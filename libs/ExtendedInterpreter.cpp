// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedInterpreter.h>

#include <formula/Node.h>
#include <formula/Parser.h>
#include <formula/ReferenceCollector.h>
#include <formula/Visitor.h>

#include "functions.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>

namespace formula
{

namespace
{

std::string reference_diagnostic_message(const FormulaFileDiagnostic &diagnostic)
{
    switch (diagnostic.code)
    {
    case FormulaFileDiagnosticCode::MISSING_IMPORT:
        return "missing import: " + diagnostic.filename;
    case FormulaFileDiagnosticCode::IMPORT_CYCLE:
        return "import cycle: " + diagnostic.filename;
    case FormulaFileDiagnosticCode::PREPROCESS_ERROR:
        return "preprocess error: " + diagnostic.detail;
    case FormulaFileDiagnosticCode::PARSE_ERROR:
        return "parse error: " + diagnostic.filename;
    case FormulaFileDiagnosticCode::UNRESOLVED_CLASS:
        return "unresolved class: " + diagnostic.detail;
    }
    throw std::runtime_error("unknown formula file diagnostic code");
}

[[noreturn]] void unsupported_runtime_node(std::string_view name)
{
    throw std::runtime_error(std::string{name} + " is not supported by the extended interpreter");
}

double real_part(const Value &value)
{
    switch (value.kind())
    {
    case ValueKind::BOOL:
        return std::get<bool>(value.storage()) ? 1.0 : 0.0;
    case ValueKind::INT:
        return static_cast<double>(std::get<int>(value.storage()));
    case ValueKind::FLOAT:
        return std::get<double>(value.storage());
    case ValueKind::COMPLEX:
        return std::get<Complex>(value.storage()).re;
    default:
        throw std::runtime_error("expected numeric value, got " + std::string{type_name(value)});
    }
}

Complex complex_value(const Value &value)
{
    if (value.kind() == ValueKind::COMPLEX)
    {
        return std::get<Complex>(value.storage());
    }
    return {real_part(value), 0.0};
}

ValueKind arithmetic_kind(ValueKind lhs, ValueKind rhs)
{
    ValueKind kind{promote_numeric_kind(lhs, rhs)};
    return kind == ValueKind::BOOL ? ValueKind::INT : kind;
}

Value numeric_value(double value, ValueKind kind)
{
    switch (kind)
    {
    case ValueKind::INT:
        return Value{static_cast<int>(value)};
    case ValueKind::FLOAT:
        return Value{value};
    default:
        return Value{Complex{value, 0.0}};
    }
}

Value numeric_binary(const Value &lhs, const Value &rhs, const std::string &op)
{
    const ValueKind kind{arithmetic_kind(lhs.kind(), rhs.kind())};
    if (kind == ValueKind::COMPLEX)
    {
        const Complex left{complex_value(lhs)};
        const Complex right{complex_value(rhs)};
        if (op == "+")
        {
            return Value{left + right};
        }
        if (op == "-")
        {
            return Value{left - right};
        }
        if (op == "*")
        {
            return Value{left * right};
        }
        if (op == "/")
        {
            return Value{left / right};
        }
        if (op == "^")
        {
            return Value{pow(left, right)};
        }
        throw std::runtime_error("invalid complex operator: " + op);
    }

    const double left{real_part(lhs)};
    const double right{real_part(rhs)};
    if (op == "+")
    {
        return numeric_value(left + right, kind);
    }
    if (op == "-")
    {
        return numeric_value(left - right, kind);
    }
    if (op == "*")
    {
        return numeric_value(left * right, kind);
    }
    if (op == "/")
    {
        return Value{left / right};
    }
    if (op == "%")
    {
        return numeric_value(std::fmod(left, right), kind);
    }
    if (op == "^")
    {
        return Value{std::pow(left, right)};
    }
    throw std::runtime_error("invalid numeric operator: " + op);
}

ColorValue color_value(const Value &value)
{
    if (value.kind() != ValueKind::COLOR)
    {
        throw std::runtime_error("expected color value, got " + std::string{type_name(value)});
    }
    return std::get<ColorValue>(value.storage());
}

Value::ImagePtr image_value(const Value &value)
{
    if (value.kind() != ValueKind::IMAGE)
    {
        throw std::runtime_error("expected image value, got " + std::string{type_name(value)});
    }
    Value::ImagePtr image{std::get<Value::ImagePtr>(value.storage())};
    if (!image)
    {
        image = std::make_shared<ImageValue>();
    }
    return image;
}

void check_arity(std::string_view name, std::size_t actual, std::size_t expected)
{
    if (actual != expected)
    {
        throw std::runtime_error("invalid call arity: " + std::string{name});
    }
}

double hue_to_rgb(double p, double q, double hue)
{
    if (hue < 0.0)
    {
        hue += 1.0;
    }
    if (hue > 1.0)
    {
        hue -= 1.0;
    }
    if (hue < 1.0 / 6.0)
    {
        return p + (q - p) * 6.0 * hue;
    }
    if (hue < 0.5)
    {
        return q;
    }
    if (hue < 2.0 / 3.0)
    {
        return p + (q - p) * (2.0 / 3.0 - hue) * 6.0;
    }
    return p;
}

ColorValue hsl_to_color(double hue, double saturation, double luminance, double alpha)
{
    hue = std::fmod(hue, 6.0) / 6.0;
    if (hue < 0.0)
    {
        hue += 1.0;
    }
    if (saturation == 0.0)
    {
        return {luminance, luminance, luminance, alpha};
    }
    const double q{luminance < 0.5 ? luminance * (1.0 + saturation) : luminance + saturation - luminance * saturation};
    const double p{2.0 * luminance - q};
    return {hue_to_rgb(p, q, hue + 1.0 / 3.0), hue_to_rgb(p, q, hue), hue_to_rgb(p, q, hue - 1.0 / 3.0), alpha};
}

struct HslValue
{
    double hue{};
    double saturation{};
    double luminance{};
};

struct RuntimeParameterInfo
{
    std::string type;
    std::string name;
    std::vector<std::string> enum_values;
    std::optional<std::string> default_selector;
    bool is_plugin{};
    bool selectable{true};
};

struct RuntimeParameterForward
{
    std::string old_name;
    std::vector<std::string> target_path;
};

struct RuntimeParameterMetadata
{
    std::vector<RuntimeParameterInfo> params;
    std::vector<RuntimeParameterInfo> functions;
    std::vector<RuntimeParameterForward> forwards;
};

class RuntimeParameterMetadataCollector : public ast::NullVisitor
{
public:
    RuntimeParameterMetadata collect(const ast::Expr &node)
    {
        visit_node(node);
        return std::move(m_metadata);
    }

    void visit(const ast::FunctionBlockNode &node) override
    {
        m_metadata.functions.push_back({node.type(), node.name(), {}, {}, false});
    }

    void visit(const ast::ParamBlockNode &node) override
    {
        RuntimeParameterInfo param{node.type(), node.name(), {}, {}, false};
        m_current_param = &param;
        visit_node(node.block());
        m_current_param = nullptr;
        m_metadata.params.push_back(std::move(param));
    }

    void visit(const ast::SettingNode &node) override
    {
        if (node.key() == "default" && m_current_param != nullptr)
        {
            if (const auto *selector = std::get_if<ast::EnumName>(&node.value()))
            {
                m_current_param->default_selector = selector->name;
            }
            return;
        }
        if (node.key() == "enum" && m_current_param != nullptr)
        {
            if (const auto *values = std::get_if<std::vector<std::string>>(&node.value()))
            {
                m_current_param->enum_values = *values;
            }
            return;
        }
        if (node.key() == "selectable" && m_current_param != nullptr)
        {
            if (const auto *selectable = std::get_if<bool>(&node.value()))
            {
                m_current_param->selectable = *selectable;
            }
            return;
        }
        if (node.key() != "param_forward")
        {
            return;
        }
        if (const auto *values = std::get_if<std::vector<std::string>>(&node.value());
            values != nullptr && values->size() >= 2U)
        {
            m_metadata.forwards.push_back({values->front(), {std::next(values->begin()), values->end()}});
        }
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        for (const ast::Expr &statement : node.statements())
        {
            visit_node(statement);
        }
    }

private:
    void visit_node(const ast::Expr &node)
    {
        if (node)
        {
            node->visit(*this);
        }
    }

    RuntimeParameterMetadata m_metadata;
    RuntimeParameterInfo *m_current_param{};
};

bool starts_with(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool same_identifier(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        const char left_char{static_cast<char>(std::tolower(static_cast<unsigned char>(left[index])))};
        const char right_char{static_cast<char>(std::tolower(static_cast<unsigned char>(right[index])))};
        if (left_char != right_char)
        {
            return false;
        }
    }
    return true;
}

bool is_void_return(std::string_view type)
{
    return type.empty() || same_identifier(type, "void");
}

bool is_legacy_function_parameter_name(std::string_view name)
{
    return name == "fn1" || name == "fn2" || name == "fn3" || name == "fn4";
}

RuntimeParameterMetadata collect_runtime_parameter_metadata(const ast::FormulaSections &ast)
{
    return RuntimeParameterMetadataCollector{}.collect(ast.defaults);
}

const RuntimeParameterInfo *find_runtime_parameter(const RuntimeParameterMetadata &metadata, std::string_view name)
{
    for (const RuntimeParameterInfo &param : metadata.params)
    {
        if (param.name == name)
        {
            return &param;
        }
    }
    return nullptr;
}

const RuntimeParameterInfo *find_runtime_function(const RuntimeParameterMetadata &metadata, std::string_view name)
{
    for (const RuntimeParameterInfo &function : metadata.functions)
    {
        if (function.name == name)
        {
            return &function;
        }
    }
    return nullptr;
}

bool is_runtime_plugin_parameter(const std::vector<semantic::FormulaParameterInfo> &parameters, std::string_view name)
{
    const auto found = std::find_if(parameters.begin(), parameters.end(),
        [name](const semantic::FormulaParameterInfo &parameter) { return parameter.name == name; });
    return found != parameters.end() && found->is_plugin;
}

std::optional<int> parse_enum_index(std::string_view value)
{
    int result{};
    const char *first{value.data()};
    const char *last{value.data() + value.size()};
    const std::from_chars_result parsed{std::from_chars(first, last, result)};
    if (parsed.ec == std::errc{} && parsed.ptr == last)
    {
        return result;
    }
    return std::nullopt;
}

struct RuntimeEntrySelector
{
    std::string filename;
    std::string class_name;
};

std::optional<RuntimeEntrySelector> parse_runtime_entry_selector(std::string_view value)
{
    const std::size_t separator{value.find(':')};
    if (separator == std::string_view::npos)
    {
        if (value.empty())
        {
            return std::nullopt;
        }
        return RuntimeEntrySelector{{}, std::string{value}};
    }
    if (separator == 0 || separator + 1 == value.size())
    {
        return std::nullopt;
    }
    return RuntimeEntrySelector{std::string{value.substr(0, separator)}, std::string{value.substr(separator + 1)}};
}

const RetainedFormulaClass *find_retained_runtime_class(
    const std::vector<RetainedFormulaClass> &classes, const RuntimeEntrySelector &selector)
{
    for (const RetainedFormulaClass &klass : classes)
    {
        if (!klass.ast || !same_identifier(klass.reference.class_name, selector.class_name))
        {
            continue;
        }
        if (selector.filename.empty() || klass.reference.filename == selector.filename)
        {
            return &klass;
        }
    }
    return nullptr;
}

const FormulaClassReference *find_runtime_class_reference(
    const FormulaFileSet &files, const RuntimeEntrySelector &selector)
{
    for (const FormulaClassReference &klass : files.class_index)
    {
        if (!same_identifier(klass.class_name, selector.class_name))
        {
            continue;
        }
        if (selector.filename.empty() || klass.filename == selector.filename)
        {
            return &klass;
        }
    }
    return nullptr;
}

const RetainedFormulaClass *find_or_retain_runtime_class(FormulaFileSet &files, const RuntimeEntrySelector &selector)
{
    if (const RetainedFormulaClass *klass{find_retained_runtime_class(files.retained_classes, selector)})
    {
        return klass;
    }
    if (const FormulaClassReference *reference{find_runtime_class_reference(files, selector)})
    {
        retain_formula_class(files, *reference);
        retain_resolved_imported_classes(files);
        return find_retained_runtime_class(files.retained_classes, selector);
    }
    return nullptr;
}

const std::string *find_runtime_class_base(
    const std::vector<RetainedFormulaClass> &classes, std::string_view class_name)
{
    for (const RetainedFormulaClass &klass : classes)
    {
        if (same_identifier(klass.reference.class_name, class_name))
        {
            return &klass.base_class;
        }
    }
    return nullptr;
}

bool runtime_class_matches_parameter_type(
    const std::vector<RetainedFormulaClass> &classes, std::string_view class_name, std::string_view parameter_type)
{
    if (same_identifier(class_name, parameter_type))
    {
        return true;
    }

    std::vector<std::string> seen;
    std::string current{class_name};
    while (const std::string *base = find_runtime_class_base(classes, current))
    {
        if (base->empty() ||
            std::find_if(seen.begin(), seen.end(),
                [base](const std::string &item) { return same_identifier(item, *base); }) != seen.end())
        {
            return false;
        }
        if (same_identifier(*base, parameter_type))
        {
            return true;
        }
        seen.push_back(*base);
        current = *base;
    }
    return false;
}

std::vector<const RetainedFormulaClass *> retained_class_ptrs(const std::vector<RetainedFormulaClass> &classes)
{
    std::vector<const RetainedFormulaClass *> result;
    result.reserve(classes.size());
    for (const RetainedFormulaClass &klass : classes)
    {
        result.push_back(&klass);
    }
    return result;
}

std::optional<Value> make_enum_parameter_value(const RuntimeParameterInfo &parameter, const Value &value)
{
    if (parameter.enum_values.empty())
    {
        return std::nullopt;
    }
    int index{};
    if (value.kind() == ValueKind::STRING)
    {
        const std::string &text{std::get<std::string>(value.storage())};
        const auto found{std::find(parameter.enum_values.begin(), parameter.enum_values.end(), text)};
        if (found == parameter.enum_values.end())
        {
            if (const std::optional<int> parsed{parse_enum_index(text)})
            {
                index = *parsed;
            }
            else
            {
                throw std::runtime_error("invalid enum value: " + text);
            }
        }
        else
        {
            index = static_cast<int>(std::distance(parameter.enum_values.begin(), found));
        }
    }
    else
    {
        const Value converted{convert_value(value, ValueKind::INT)};
        index = std::get<int>(converted.storage());
    }
    if (index < 0 || static_cast<std::size_t>(index) >= parameter.enum_values.size())
    {
        throw std::runtime_error("invalid enum value: " + std::to_string(index));
    }
    return make_enum_value(
        EnumValue{index, parameter.enum_values[static_cast<std::size_t>(index)], parameter.enum_values});
}

Value parse_saved_parameter_value(std::string_view type, std::string_view value);
std::optional<ValueKind> parameter_value_kind(std::string_view type);
ValueKind value_kind_for_type(std::string_view type);

std::string forwarded_runtime_parameter_name(const RuntimeParameterMetadata &metadata, std::string_view name)
{
    if (find_runtime_parameter(metadata, name) != nullptr)
    {
        return std::string{name};
    }
    for (const RuntimeParameterForward &forward : metadata.forwards)
    {
        if (forward.old_name == name && !forward.target_path.empty())
        {
            std::string target{forward.target_path.front()};
            for (auto part = std::next(forward.target_path.begin()); part != forward.target_path.end(); ++part)
            {
                target += ".";
                target += *part;
            }
            return target;
        }
    }
    return std::string{name};
}

bool bind_forwarded_runtime_parameter(ExtendedInterpreter &interpreter, const RuntimeParameterMetadata &metadata,
    const std::string &name, const parameter::Parameter &parameter)
{
    const std::size_t dot{name.find('.')};
    if (dot != std::string::npos)
    {
        const std::string plugin{name.substr(0, dot)};
        std::string nested{name.substr(dot + 1)};
        if (starts_with(nested, "p_"))
        {
            nested.erase(0, 2);
        }
        interpreter.set_plugin_parameter_value(plugin, nested, Value{parameter.value});
        return true;
    }

    const RuntimeParameterInfo *info{find_runtime_parameter(metadata, name)};
    if (info == nullptr)
    {
        return false;
    }
    if (is_runtime_plugin_parameter(interpreter.parameters(), name))
    {
        interpreter.set_plugin_parameter(name, parameter.value);
        return true;
    }

    Value value{parse_saved_parameter_value(info->type, parameter.value)};
    if (const std::optional<Value> enum_value{make_enum_parameter_value(*info, value)})
    {
        value = *enum_value;
    }
    interpreter.set_parameter(name, std::move(value));
    return true;
}

bool plugin_has_nested_runtime_parameter(const PluginValue &plugin, std::string_view name)
{
    if (!plugin.ast)
    {
        return false;
    }
    const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*plugin.ast)};
    return find_runtime_parameter(metadata, name) != nullptr;
}

std::optional<int> literal_int(const ast::Expr &expr)
{
    if (!expr)
    {
        return std::nullopt;
    }
    const auto *literal = dynamic_cast<const ast::LiteralNode *>(expr.get());
    if (literal == nullptr)
    {
        return std::nullopt;
    }
    if (const auto *value = std::get_if<int>(&literal->value()))
    {
        return *value;
    }
    return std::nullopt;
}

Value default_field_value(const ast::DeclarationNode &node)
{
    if (node.is_array())
    {
        ArrayValue array;
        array.element_kind = value_kind_for_type(node.type());
        if (node.is_dynamic_array())
        {
            array.dynamic = true;
            array.dimensions = {0};
        }
        else
        {
            array.dimensions.reserve(node.dimensions().size());
            for (const ast::Expr &dimension : node.dimensions())
            {
                array.dimensions.push_back(literal_int(dimension).value_or(0));
            }
            const int count{std::accumulate(
                array.dimensions.begin(), array.dimensions.end(), 1, [](int left, int right) { return left * right; })};
            array.elements.assign(static_cast<std::size_t>(count), default_value(array.element_kind));
        }
        return make_array_value(std::move(array));
    }
    if (const std::optional<ValueKind> kind{parameter_value_kind(node.type())})
    {
        return default_value(*kind);
    }
    return Value{};
}

class ObjectFieldCollector : public ast::NullVisitor
{
public:
    std::vector<std::pair<std::string, Value>> collect(const ast::FormulaSections &ast)
    {
        collect(ast.public_members);
        collect(ast.protected_members);
        collect(ast.private_members);
        return std::move(m_fields);
    }

    void visit(const ast::DeclarationNode &node) override
    {
        m_fields.push_back({node.name(), default_field_value(node)});
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        for (const ast::Expr &statement : node.statements())
        {
            collect(statement);
        }
    }

private:
    void collect(const ast::Expr &node)
    {
        if (node)
        {
            node->visit(*this);
        }
    }

    std::vector<std::pair<std::string, Value>> m_fields;
};

Value make_object_value(const PluginValue &plugin)
{
    if (!plugin.ast)
    {
        throw std::runtime_error("missing plug-in object state");
    }
    PluginValue object{plugin};
    object.object_initialized = true;
    object.object_fields = ObjectFieldCollector{}.collect(*plugin.ast);
    return make_plugin_value(std::move(object));
}

bool runtime_parameter_is_plugin(
    const std::vector<RetainedFormulaClass> &classes, const RuntimeParameterInfo &parameter)
{
    return !parameter_value_kind(parameter.type).has_value() &&
        find_retained_runtime_class(classes, RuntimeEntrySelector{{}, parameter.type}) != nullptr;
}

Value make_plugin_parameter_value(const RetainedFormulaClass &klass, FormulaFileSet &files)
{
    PluginValue value;
    value.filename = klass.reference.filename;
    value.class_name = klass.reference.class_name;
    value.base_class = klass.base_class;
    value.ast = klass.ast;

    if (klass.ast)
    {
        const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*klass.ast)};
        for (const RuntimeParameterInfo &parameter : metadata.params)
        {
            if (parameter_value_kind(parameter.type).has_value())
            {
                continue;
            }
            const std::string selector{parameter.default_selector.value_or(parameter.type)};
            const std::optional<RuntimeEntrySelector> parsed{parse_runtime_entry_selector(selector)};
            if (!parsed)
            {
                continue;
            }
            if (const RetainedFormulaClass *nested{find_or_retain_runtime_class(files, *parsed)})
            {
                value.nested_values.push_back({parameter.name, make_plugin_parameter_value(*nested, files)});
            }
        }
    }

    return make_plugin_value(std::move(value));
}

semantic::SemanticTypeKind semantic_type_kind(std::string_view type)
{
    if (type.empty() || type == "complex")
    {
        return semantic::SemanticTypeKind::COMPLEX;
    }
    if (type == "color")
    {
        return semantic::SemanticTypeKind::COLOR;
    }
    if (type == "bool")
    {
        return semantic::SemanticTypeKind::BOOL;
    }
    if (type == "int")
    {
        return semantic::SemanticTypeKind::INT;
    }
    if (type == "float")
    {
        return semantic::SemanticTypeKind::FLOAT;
    }
    return semantic::SemanticTypeKind::ERROR;
}

int parameter_conversion_rank(semantic::SemanticTypeKind type)
{
    switch (type)
    {
    case semantic::SemanticTypeKind::BOOL:
        return 0;
    case semantic::SemanticTypeKind::INT:
        return 1;
    case semantic::SemanticTypeKind::FLOAT:
        return 2;
    case semantic::SemanticTypeKind::COMPLEX:
        return 3;
    default:
        return -1;
    }
}

bool can_convert_parameter_type(semantic::SemanticTypeKind from, semantic::SemanticTypeKind to)
{
    if (from == semantic::SemanticTypeKind::ERROR || to == semantic::SemanticTypeKind::ERROR || from == to)
    {
        return true;
    }
    return parameter_conversion_rank(from) >= 0 && parameter_conversion_rank(to) >= 0 &&
        parameter_conversion_rank(from) <= parameter_conversion_rank(to);
}

bool function_target_matches_type(const semantic::SemanticFunctionDescriptor &target, semantic::SemanticTypeKind type)
{
    if (!can_convert_parameter_type(target.return_type.kind, type))
    {
        return false;
    }
    if (type == semantic::SemanticTypeKind::COLOR)
    {
        return target.argument_types.size() == 2U &&
            target.argument_types[0].kind == semantic::SemanticTypeKind::COLOR &&
            target.argument_types[1].kind == semantic::SemanticTypeKind::COLOR;
    }
    return type == semantic::SemanticTypeKind::COMPLEX && target.argument_types.size() == 1U &&
        can_convert_parameter_type(target.argument_types.front().kind, semantic::SemanticTypeKind::COMPLEX);
}

bool function_target_matches_type(const ast::FunctionDeclNode &target, semantic::SemanticTypeKind type)
{
    const semantic::SemanticTypeKind return_type{semantic_type_kind(target.return_type())};
    if (!can_convert_parameter_type(return_type, type))
    {
        return false;
    }
    if (type == semantic::SemanticTypeKind::COLOR)
    {
        return target.args().size() == 2U &&
            semantic_type_kind(target.args()[0].type) == semantic::SemanticTypeKind::COLOR &&
            semantic_type_kind(target.args()[1].type) == semantic::SemanticTypeKind::COLOR;
    }
    return type == semantic::SemanticTypeKind::COMPLEX && target.args().size() == 1U &&
        can_convert_parameter_type(semantic_type_kind(target.args().front().type), semantic::SemanticTypeKind::COMPLEX);
}

int parse_saved_int(std::string_view value)
{
    int result{};
    const char *first{value.data()};
    const char *last{value.data() + value.size()};
    const std::from_chars_result parsed{std::from_chars(first, last, result)};
    if (parsed.ec != std::errc{} || parsed.ptr != last)
    {
        throw std::runtime_error("invalid saved int parameter");
    }
    return result;
}

double parse_saved_float(std::string_view value)
{
    std::size_t pos{};
    const double result{std::stod(std::string{value}, &pos)};
    if (pos != value.size())
    {
        throw std::runtime_error("invalid saved float parameter");
    }
    return result;
}

Value parse_saved_parameter_value(std::string_view type, std::string_view value)
{
    if (type == "bool")
    {
        if (same_identifier(value, "yes") || same_identifier(value, "on") || same_identifier(value, "true"))
        {
            return Value{true};
        }
        if (same_identifier(value, "no") || same_identifier(value, "off") || same_identifier(value, "false"))
        {
            return Value{false};
        }
        throw std::runtime_error("invalid saved bool parameter");
    }
    if (type == "int")
    {
        return Value{parse_saved_int(value)};
    }
    if (type == "float")
    {
        return Value{parse_saved_float(value)};
    }
    if (type == "complex")
    {
        const std::size_t separator{value.find('/')};
        if (separator == std::string_view::npos)
        {
            throw std::runtime_error("invalid saved complex parameter");
        }
        return Value{
            Complex{parse_saved_float(value.substr(0, separator)), parse_saved_float(value.substr(separator + 1))}};
    }
    if (type == "color")
    {
        const std::uint32_t packed{static_cast<std::uint32_t>(parse_saved_int(value))};
        return Value{ColorValue{static_cast<double>((packed >> 16U) & 0xffU) / 255.0,
            static_cast<double>((packed >> 8U) & 0xffU) / 255.0, static_cast<double>(packed & 0xffU) / 255.0,
            static_cast<double>((packed >> 24U) & 0xffU) / 255.0}};
    }
    if (type == "Image")
    {
        return make_image_value(ImageValue{std::string{value}, false});
    }
    return Value{std::string{value}};
}

std::optional<bool> compare_enum_to_string(const Value &lhs, const Value &rhs, const std::string &op)
{
    const Value *enum_value{nullptr};
    const Value *string_value{nullptr};
    if (lhs.kind() == ValueKind::ENUM && rhs.kind() == ValueKind::STRING)
    {
        enum_value = &lhs;
        string_value = &rhs;
    }
    else if (lhs.kind() == ValueKind::STRING && rhs.kind() == ValueKind::ENUM)
    {
        enum_value = &rhs;
        string_value = &lhs;
    }
    if (enum_value == nullptr)
    {
        return std::nullopt;
    }
    const Value::EnumPtr enum_ptr{std::get<Value::EnumPtr>(enum_value->storage())};
    const bool matches{enum_ptr && enum_ptr->label == std::get<std::string>(string_value->storage())};
    if (op == "==")
    {
        return matches;
    }
    if (op == "!=")
    {
        return !matches;
    }
    return std::nullopt;
}

std::string parameter_reference_message(const parameter::ParameterReferenceDiagnostic &diagnostic)
{
    switch (diagnostic.code)
    {
    case parameter::ParameterReferenceErrorCode::MISSING_FILENAME:
        return "missing parameter reference filename";
    case parameter::ParameterReferenceErrorCode::MISSING_ENTRY:
        return "missing parameter reference entry";
    case parameter::ParameterReferenceErrorCode::UNRESOLVED_ENTRY:
        return "unresolved parameter reference: " + diagnostic.detail;
    case parameter::ParameterReferenceErrorCode::PARSE_ERROR:
        return "parameter reference parse error: " + diagnostic.detail;
    case parameter::ParameterReferenceErrorCode::UNKNOWN_PARAMETER:
        return "unknown parameter binding: " + diagnostic.detail;
    case parameter::ParameterReferenceErrorCode::MISSING_REQUIRED_PARAMETER:
        return "missing required parameter binding: " + diagnostic.detail;
    case parameter::ParameterReferenceErrorCode::TYPE_MISMATCH:
        return "parameter binding type mismatch: " + diagnostic.detail;
    case parameter::ParameterReferenceErrorCode::INVALID_PARAMETER_FORWARD:
        return "invalid parameter forward: " + diagnostic.detail;
    case parameter::ParameterReferenceErrorCode::NONE:
        return {};
    }
    throw std::runtime_error("unknown parameter reference diagnostic code");
}

HslValue color_to_hsl(ColorValue color)
{
    const double maximum{std::max({color.red, color.green, color.blue})};
    const double minimum{std::min({color.red, color.green, color.blue})};
    const double chroma{maximum - minimum};
    const double luminance{(maximum + minimum) / 2.0};
    if (chroma == 0.0)
    {
        return {0.0, 0.0, luminance};
    }
    double hue{};
    if (maximum == color.red)
    {
        hue = std::fmod((color.green - color.blue) / chroma, 6.0);
    }
    else if (maximum == color.green)
    {
        hue = (color.blue - color.red) / chroma + 2.0;
    }
    else
    {
        hue = (color.red - color.green) / chroma + 4.0;
    }
    if (hue < 0.0)
    {
        hue += 6.0;
    }
    const double saturation{chroma / (1.0 - std::abs(2.0 * luminance - 1.0))};
    return {hue, saturation, luminance};
}

int random_seed(int seed)
{
    const std::uint32_t next{static_cast<std::uint32_t>(seed) * 1103515245U + 12345U};
    if (next <= 0x7fffffffU)
    {
        return static_cast<int>(next);
    }
    return -static_cast<int>((~next) + 1U);
}

std::size_t image_pixel_index(const ImageValue &image, int x, int y)
{
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(x);
}

ColorValue transparent_color()
{
    return {};
}

ColorValue image_pixel(const ImageValue &image, int x, int y)
{
    if (image.empty || x < 0 || y < 0 || x >= image.width || y >= image.height)
    {
        return transparent_color();
    }
    return image.pixels[image_pixel_index(image, x, y)];
}

void resize_image(ImageValue &image, int width, int height)
{
    if (width < 0 || height < 0)
    {
        throw std::runtime_error("invalid image size");
    }
    image.empty = width == 0 || height == 0;
    image.width = width;
    image.height = height;
    image.pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), transparent_color());
}

Value compare_values(const Value &lhs, const Value &rhs, const std::string &op)
{
    if (const std::optional<bool> result{compare_enum_to_string(lhs, rhs, op)})
    {
        return Value{*result};
    }
    if (op == "==")
    {
        if (is_numeric(lhs.kind()) && is_numeric(rhs.kind()))
        {
            return Value{complex_value(lhs) == complex_value(rhs)};
        }
        return Value{lhs == rhs};
    }
    if (op == "!=")
    {
        if (is_numeric(lhs.kind()) && is_numeric(rhs.kind()))
        {
            return Value{complex_value(lhs) != complex_value(rhs)};
        }
        return Value{lhs != rhs};
    }

    const double left{real_part(lhs)};
    const double right{real_part(rhs)};
    if (op == "<")
    {
        return Value{left < right};
    }
    if (op == "<=")
    {
        return Value{left <= right};
    }
    if (op == ">")
    {
        return Value{left > right};
    }
    if (op == ">=")
    {
        return Value{left >= right};
    }
    throw std::runtime_error("invalid comparison operator: " + op);
}

ValueKind value_kind_for_type(std::string_view type)
{
    if (type == "bool")
    {
        return ValueKind::BOOL;
    }
    if (type == "int")
    {
        return ValueKind::INT;
    }
    if (type == "float")
    {
        return ValueKind::FLOAT;
    }
    if (type == "complex")
    {
        return ValueKind::COMPLEX;
    }
    if (type == "color")
    {
        return ValueKind::COLOR;
    }
    if (type == "string")
    {
        return ValueKind::STRING;
    }
    if (type == "Image")
    {
        return ValueKind::IMAGE;
    }
    throw std::runtime_error("unsupported declaration type: " + std::string{type});
}

ValueKind value_kind_for_type(semantic::SemanticTypeKind type)
{
    switch (type)
    {
    case semantic::SemanticTypeKind::BOOL:
        return ValueKind::BOOL;
    case semantic::SemanticTypeKind::INT:
        return ValueKind::INT;
    case semantic::SemanticTypeKind::FLOAT:
        return ValueKind::FLOAT;
    case semantic::SemanticTypeKind::COMPLEX:
        return ValueKind::COMPLEX;
    case semantic::SemanticTypeKind::COLOR:
        return ValueKind::COLOR;
    case semantic::SemanticTypeKind::STRING:
        return ValueKind::STRING;
    case semantic::SemanticTypeKind::BUILTIN_OBJECT:
    case semantic::SemanticTypeKind::CLASS_OBJECT:
        return ValueKind::IMAGE;
    default:
        return ValueKind::EMPTY;
    }
}

std::optional<ValueKind> parameter_value_kind(std::string_view type)
{
    if (type == "bool" || type == "int" || type == "float" || type == "complex" || type == "color" ||
        type == "string" || type == "Image")
    {
        return value_kind_for_type(type);
    }
    return std::nullopt;
}

Value convert_parameter_default(const Value &value, std::string_view type)
{
    if (type.empty())
    {
        return value.kind() == ValueKind::EMPTY ? default_value(ValueKind::COMPLEX) : value;
    }
    if (const std::optional<ValueKind> kind{parameter_value_kind(type)})
    {
        return convert_value(value, *kind);
    }
    return value;
}

Value implicit_parameter_default(std::string_view type)
{
    if (type.empty())
    {
        return default_value(ValueKind::COMPLEX);
    }
    if (const std::optional<ValueKind> kind{parameter_value_kind(type)})
    {
        return default_value(*kind);
    }
    return Value{std::string{type}};
}

std::optional<std::string> function_name_value(const ast::SettingNode::ValueType &value)
{
    if (const auto *expr = std::get_if<ast::Expr>(&value))
    {
        if (const auto *call = dynamic_cast<const ast::FunctionCallNode *>(expr->get());
            call != nullptr && !call->has_target() && call->args().empty())
        {
            return call->name();
        }
    }
    if (const auto *name = std::get_if<ast::EnumName>(&value))
    {
        return name->name;
    }
    if (const auto *name = std::get_if<std::string>(&value))
    {
        return *name;
    }
    return std::nullopt;
}

Value value_from_setting(const ast::SettingNode::ValueType &value)
{
    switch (value.index())
    {
    case 0:
        return Value{std::get<double>(value)};
    case 1:
        return Value{std::get<Complex>(value)};
    case 2:
        return Value{std::get<std::string>(value)};
    case 3:
        return Value{std::get<int>(value)};
    case 4:
        return Value{std::get<ast::EnumName>(value).name};
    case 5:
        return Value{std::get<bool>(value)};
    default:
        return {};
    }
}

const semantic::BuiltinRegistry &builtins_or_default(const semantic::BuiltinRegistry *builtins)
{
    return builtins != nullptr ? *builtins : semantic::default_builtin_registry();
}

void check_array_copy(const Value &target, const Value &source)
{
    if (target.kind() != ValueKind::ARRAY || source.kind() != ValueKind::ARRAY)
    {
        return;
    }
    const Value::ArrayPtr target_array{std::get<Value::ArrayPtr>(target.storage())};
    const Value::ArrayPtr source_array{std::get<Value::ArrayPtr>(source.storage())};
    if (!target_array || !source_array || target_array->dynamic || source_array->dynamic ||
        target_array->element_kind != source_array->element_kind ||
        target_array->dimensions != source_array->dimensions)
    {
        throw std::runtime_error("invalid array assignment");
    }
}

using FunctionMap = std::unordered_map<std::string, const ast::FunctionDeclNode *>;

void collect_function_declarations(const ast::Expr &node, FunctionMap &functions)
{
    if (!node)
    {
        return;
    }
    if (const auto *function = dynamic_cast<const ast::FunctionDeclNode *>(node.get()); function)
    {
        functions[function->name()] = function;
        return;
    }
    if (const auto *sequence = dynamic_cast<const ast::StatementSeqNode *>(node.get()); sequence)
    {
        for (const ast::Expr &statement : sequence->statements())
        {
            collect_function_declarations(statement, functions);
        }
        return;
    }
    if (const auto *branch = dynamic_cast<const ast::IfStatementNode *>(node.get()); branch)
    {
        if (branch->has_then_block())
        {
            collect_function_declarations(branch->then_block(), functions);
        }
        if (branch->has_else_block())
        {
            collect_function_declarations(branch->else_block(), functions);
        }
        return;
    }
    if (const auto *loop = dynamic_cast<const ast::WhileNode *>(node.get()); loop)
    {
        collect_function_declarations(loop->body(), functions);
        return;
    }
    if (const auto *loop = dynamic_cast<const ast::RepeatUntilNode *>(node.get()); loop)
    {
        collect_function_declarations(loop->body(), functions);
    }
}

FunctionMap collect_function_declarations(const ast::FormulaSections &formula)
{
    FunctionMap functions;
    collect_function_declarations(formula.per_image, functions);
    collect_function_declarations(formula.builtin, functions);
    collect_function_declarations(formula.initialize, functions);
    collect_function_declarations(formula.iterate, functions);
    collect_function_declarations(formula.bailout, functions);
    collect_function_declarations(formula.perturb_initialize, functions);
    collect_function_declarations(formula.perturb_iterate, functions);
    collect_function_declarations(formula.defaults, functions);
    collect_function_declarations(formula.type_switch, functions);
    collect_function_declarations(formula.final, functions);
    collect_function_declarations(formula.transform, functions);
    collect_function_declarations(formula.public_members, functions);
    collect_function_declarations(formula.protected_members, functions);
    collect_function_declarations(formula.private_members, functions);
    return functions;
}

const ast::FunctionDeclNode *find_class_method(const ast::FormulaSections &ast, std::string_view name);
const ast::FunctionDeclNode *find_static_class_method(const ast::FormulaSections &ast, std::string_view name);
const ast::FunctionDeclNode *find_class_constructor(const ast::FormulaSections &ast, std::string_view name);

const ast::FunctionDeclNode *find_method_in_expr(const ast::Expr &node, std::string_view name, bool is_static)
{
    if (!node)
    {
        return nullptr;
    }
    if (const auto *function = dynamic_cast<const ast::FunctionDeclNode *>(node.get()))
    {
        return same_identifier(function->name(), name) && function->is_static() == is_static ? function : nullptr;
    }
    if (const auto *sequence = dynamic_cast<const ast::StatementSeqNode *>(node.get()))
    {
        for (const ast::Expr &statement : sequence->statements())
        {
            if (const ast::FunctionDeclNode *function{find_method_in_expr(statement, name, is_static)})
            {
                return function;
            }
        }
    }
    return nullptr;
}

const ast::FunctionDeclNode *find_class_method(const ast::FormulaSections &ast, std::string_view name)
{
    if (const ast::FunctionDeclNode *function{find_method_in_expr(ast.public_members, name, false)})
    {
        return function;
    }
    if (const ast::FunctionDeclNode *function{find_method_in_expr(ast.protected_members, name, false)})
    {
        return function;
    }
    return find_method_in_expr(ast.private_members, name, false);
}

const ast::FunctionDeclNode *find_static_class_method(const ast::FormulaSections &ast, std::string_view name)
{
    if (const ast::FunctionDeclNode *function{find_method_in_expr(ast.public_members, name, true)})
    {
        return function;
    }
    if (const ast::FunctionDeclNode *function{find_method_in_expr(ast.protected_members, name, true)})
    {
        return function;
    }
    return find_method_in_expr(ast.private_members, name, true);
}

const ast::FunctionDeclNode *find_class_constructor(const ast::FormulaSections &ast, std::string_view name)
{
    if (const ast::FunctionDeclNode *function{find_method_in_expr(ast.public_members, name, false)})
    {
        return function;
    }
    if (const ast::FunctionDeclNode *function{find_method_in_expr(ast.protected_members, name, false)})
    {
        return function;
    }
    return find_method_in_expr(ast.private_members, name, false);
}

const ast::DeclarationNode *find_declaration_in_expr(const ast::Expr &node, std::string_view name)
{
    if (!node)
    {
        return nullptr;
    }
    if (const auto *declaration = dynamic_cast<const ast::DeclarationNode *>(node.get()))
    {
        return same_identifier(declaration->name(), name) ? declaration : nullptr;
    }
    if (const auto *sequence = dynamic_cast<const ast::StatementSeqNode *>(node.get()))
    {
        for (const ast::Expr &statement : sequence->statements())
        {
            if (const ast::DeclarationNode *declaration{find_declaration_in_expr(statement, name)})
            {
                return declaration;
            }
        }
    }
    return nullptr;
}

const ast::DeclarationNode *find_class_declaration_in_ast(const ast::FormulaSections &ast, std::string_view name)
{
    if (const ast::DeclarationNode *declaration{find_declaration_in_expr(ast.public_members, name)})
    {
        return declaration;
    }
    if (const ast::DeclarationNode *declaration{find_declaration_in_expr(ast.protected_members, name)})
    {
        return declaration;
    }
    return find_declaration_in_expr(ast.private_members, name);
}

const RetainedFormulaClass *find_retained_class_by_name(
    const std::vector<RetainedFormulaClass> &classes, std::string_view class_name)
{
    for (const RetainedFormulaClass &klass : classes)
    {
        if (same_identifier(klass.reference.class_name, class_name))
        {
            return &klass;
        }
    }
    return nullptr;
}

const std::string *class_name_from_identifier(const ast::Expr &expr)
{
    if (const auto *identifier = dynamic_cast<const ast::IdentifierNode *>(expr.get()))
    {
        const std::string &name{identifier->name()};
        if (!name.empty() && std::isupper(static_cast<unsigned char>(name.front())) != 0)
        {
            return &name;
        }
    }
    return nullptr;
}

class ExpressionInterpreter : public ast::NullVisitor
{
public:
    ExpressionInterpreter(ExtendedRuntimeState &state, const FunctionMap &functions, const FormulaFileSet &files,
        std::size_t max_loop_iterations) :
        m_state(state),
        m_functions(functions),
        m_files(files),
        m_max_loop_iterations(max_loop_iterations)
    {
    }

    Value interpret(const ast::Expr &node)
    {
        if (!node)
        {
            return {};
        }
        node->visit(*this);
        return m_result;
    }

    void visit(const ast::AssignmentNode &node) override
    {
        Value value{interpret(node.expression())};
        const RuntimeLValue target{lvalue(node.target())};
        check_array_copy(target.get(), value);
        target.set(value);
        m_result = std::move(value);
    }

    void visit(const ast::BinaryOpNode &node) override
    {
        const std::string &op{node.op()};
        const Value left{interpret(node.left())};
        if (op == "&&")
        {
            m_result = is_truthy(left) ? Value{is_truthy(interpret(node.right()))} : Value{false};
            return;
        }
        if (op == "||")
        {
            m_result = is_truthy(left) ? Value{true} : Value{is_truthy(interpret(node.right()))};
            return;
        }

        const Value right{interpret(node.right())};
        if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=")
        {
            m_result = compare_values(left, right, op);
            return;
        }
        m_result = numeric_binary(left, right, op);
    }

    void visit(const ast::ConstantRefNode &node) override
    {
        m_result = m_state.predefined_value(node.name());
    }

    void visit(const ast::DeclarationNode &node) override
    {
        if (node.is_array())
        {
            if (node.is_dynamic_array())
            {
                const Value value{make_dynamic_array(node)};
                m_state.declare_local_value(node.name(), value);
                m_result = value;
                return;
            }
            const Value value{make_static_array(node)};
            m_state.declare_local_value(node.name(), value);
            m_result = value;
            return;
        }
        Value value;
        if (const std::optional<ValueKind> kind{parameter_value_kind(node.type())})
        {
            value = node.initializer() ? convert_value(interpret(node.initializer()), *kind) : default_value(*kind);
        }
        else if (node.initializer())
        {
            value = interpret(node.initializer());
        }
        else
        {
            value = make_plugin_value(PluginValue{{}, std::string{node.type()}});
        }
        m_state.declare_local_value(node.name(), value);
        m_result = std::move(value);
    }

    void visit(const ast::FunctionBlockNode &) override
    {
        unsupported_runtime_node("FunctionBlockNode");
    }

    void visit(const ast::FunctionDeclNode &node) override
    {
        m_functions[node.name()] = &node;
    }

    void visit(const ast::FunctionCallNode &node) override
    {
        if (node.has_target())
        {
            if (const auto result = call_static_member(node))
            {
                m_result = *result;
                return;
            }
            if (const auto result = call_member(node))
            {
                m_result = *result;
                return;
            }
            unsupported_runtime_node("FunctionCallNode");
        }
        if (!node.name().empty() && node.name().front() == '@')
        {
            m_result = call_parameter_function(node.name().substr(1), node.args());
            return;
        }
        if (is_legacy_function_parameter_name(node.name()) && m_state.has_parameter_value(node.name()))
        {
            m_result = call_parameter_function(node.name(), node.args());
            return;
        }
        if (const auto builtin = call_builtin(node.name(), node.args()))
        {
            m_result = *builtin;
            return;
        }
        if (const RetainedFormulaClass *klass{find_retained_class_by_name(m_files.retained_classes, node.name())})
        {
            m_result = cast_object(*klass, node.args());
            return;
        }
        const auto function = m_functions.find(node.name());
        if (function == m_functions.end())
        {
            unsupported_runtime_node("FunctionCallNode");
        }
        m_result = call_function(*function->second, node.args());
    }

    void visit(const ast::HeadingBlockNode &) override
    {
        unsupported_runtime_node("HeadingBlockNode");
    }

    void visit(const ast::IdentifierNode &node) override
    {
        m_result = m_state.value(node.name());
    }

    void visit(const ast::IfStatementNode &node) override
    {
        if (is_truthy(interpret(node.condition())))
        {
            if (node.has_then_block())
            {
                m_result = interpret_block(node.then_block());
            }
            return;
        }
        if (node.has_else_block())
        {
            m_result = interpret_block(node.else_block());
        }
    }

    void visit(const ast::IndexNode &node) override
    {
        m_result = array_lvalue(node).get();
    }

    void visit(const ast::LiteralNode &node) override
    {
        const ast::LiteralNode::ValueType value{node.value()};
        switch (value.index())
        {
        case 0:
            m_result = Value{std::get<int>(value)};
            return;
        case 1:
            m_result = Value{std::get<double>(value)};
            return;
        case 2:
            m_result = Value{std::get<Complex>(value)};
            return;
        case 3:
            m_result = Value{std::get<bool>(value)};
            return;
        case 4:
            m_result = Value{std::get<std::string>(value)};
            return;
        case 5:
        {
            const ast::LiteralNode::Color color{std::get<ast::LiteralNode::Color>(value)};
            m_result = Value{ColorValue{color.red, color.green, color.blue, color.alpha}};
            return;
        }
        default:
            throw std::runtime_error("unknown literal variant index");
        }
    }

    void visit(const ast::MemberAccessNode &node) override
    {
        if (const auto result = class_constant_value(node))
        {
            m_result = *result;
            return;
        }
        m_result = member_lvalue(node).get();
    }

    void visit(const ast::NewNode &node) override
    {
        if (node.type() == "Image")
        {
            check_arity("Image", node.args().size(), 0U);
            m_result = make_image_value(ImageValue{});
            return;
        }
        if (!node.type().empty() && node.type().front() == '@')
        {
            m_result = m_state.parameter_value(node.type());
            if (m_result.kind() == ValueKind::PLUGIN)
            {
                const Value::PluginPtr plugin{std::get<Value::PluginPtr>(m_result.storage())};
                if (!plugin)
                {
                    throw std::runtime_error("missing plug-in binding: " + node.type().substr(1));
                }
                m_result = construct_object(*plugin, node.args());
                return;
            }
            if (m_result.kind() == ValueKind::EMPTY)
            {
                m_result = default_value(ValueKind::IMAGE);
            }
            else if (m_result.kind() == ValueKind::STRING)
            {
                throw std::runtime_error("missing plug-in binding: " + node.type().substr(1));
            }
            return;
        }
        if (m_state.has_parameter_value(node.type()))
        {
            m_result = m_state.parameter_value(node.type());
            if (m_result.kind() == ValueKind::PLUGIN)
            {
                const Value::PluginPtr plugin{std::get<Value::PluginPtr>(m_result.storage())};
                if (!plugin)
                {
                    throw std::runtime_error("missing plug-in binding: " + node.type());
                }
                m_result = construct_object(*plugin, node.args());
                return;
            }
            if (m_result.kind() == ValueKind::EMPTY)
            {
                m_result = default_value(ValueKind::IMAGE);
            }
            else if (m_result.kind() == ValueKind::STRING)
            {
                throw std::runtime_error("missing plug-in binding: " + node.type());
            }
            return;
        }
        if (const RetainedFormulaClass *klass{find_retained_class_by_name(m_files.retained_classes, node.type())})
        {
            m_result = construct_object(*klass, node.args());
            return;
        }
        unsupported_runtime_node("NewNode");
    }

    void visit(const ast::ParameterRefNode &node) override
    {
        m_result = m_state.parameter_value(node.name());
    }

    void visit(const ast::RepeatUntilNode &node) override
    {
        std::size_t iterations{};
        do
        {
            check_loop_iterations(++iterations);
            m_result = interpret_block(node.body());
            if (m_returning)
            {
                return;
            }
        } while (!is_truthy(interpret(node.condition())));
    }

    void visit(const ast::ReturnNode &node) override
    {
        m_result = node.expression() ? interpret(node.expression()) : Value{};
        m_returning = true;
    }

    void visit(const ast::SettingNode &) override
    {
        unsupported_runtime_node("SettingNode");
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        m_result = {};
        for (const ast::Expr &statement : node.statements())
        {
            m_result = interpret(statement);
            if (m_returning)
            {
                return;
            }
        }
    }

    void visit(const ast::UnaryOpNode &node) override
    {
        const Value value{interpret(node.operand())};
        switch (node.op())
        {
        case '+':
            m_result = value;
            return;
        case '-':
            m_result = numeric_binary(Value{0}, value, "-");
            return;
        case '!':
            m_result = Value{!is_truthy(value)};
            return;
        case '|':
        {
            const Complex complex{complex_value(value)};
            m_result = Value{complex.re * complex.re + complex.im * complex.im};
            return;
        }
        default:
            throw std::runtime_error("invalid unary operator");
        }
    }

    void visit(const ast::WhileNode &node) override
    {
        std::size_t iterations{};
        while (is_truthy(interpret(node.condition())))
        {
            check_loop_iterations(++iterations);
            m_result = interpret_block(node.body());
            if (m_returning)
            {
                return;
            }
        }
    }

private:
    Value make_static_array(const ast::DeclarationNode &node)
    {
        ArrayValue array;
        array.element_kind = value_kind_for_type(node.type());
        array.dimensions.reserve(node.dimensions().size());
        for (const ast::Expr &dimension : node.dimensions())
        {
            const Value value{convert_value(interpret(dimension), ValueKind::INT)};
            const int size{std::get<int>(value.storage())};
            if (size < 0)
            {
                throw std::runtime_error("invalid array dimension");
            }
            array.dimensions.push_back(size);
        }
        const int count{std::accumulate(
            array.dimensions.begin(), array.dimensions.end(), 1, [](int left, int right) { return left * right; })};
        array.elements.assign(static_cast<std::size_t>(count), default_value(array.element_kind));
        return make_array_value(std::move(array));
    }

    Value make_dynamic_array(const ast::DeclarationNode &node)
    {
        ArrayValue array;
        array.element_kind = value_kind_for_type(node.type());
        array.dynamic = true;
        array.dimensions = {0};
        return make_array_value(std::move(array));
    }

    Value call_set_length(const std::vector<ast::Expr> &args)
    {
        if (args.size() != 2U)
        {
            throw std::runtime_error("invalid call arity: setLength");
        }
        const RuntimeLValue value{lvalue(args.front())};
        Value array_value{value.get()};
        if (array_value.kind() != ValueKind::ARRAY)
        {
            throw std::runtime_error("invalid dynamic array argument: setLength");
        }
        const Value::ArrayPtr array{std::get<Value::ArrayPtr>(array_value.storage())};
        if (!array || !array->dynamic)
        {
            throw std::runtime_error("invalid dynamic array argument: setLength");
        }
        const int size{std::get<int>(convert_value(interpret(args[1]), ValueKind::INT).storage())};
        if (size < 0)
        {
            throw std::runtime_error("invalid array length");
        }
        array->dimensions = {size};
        array->elements.resize(static_cast<std::size_t>(size), default_value(array->element_kind));
        return {};
    }

    Value call_length(const std::vector<ast::Expr> &args)
    {
        if (args.size() != 1U)
        {
            throw std::runtime_error("invalid call arity: length");
        }
        const Value array_value{interpret(args.front())};
        if (array_value.kind() != ValueKind::ARRAY)
        {
            throw std::runtime_error("invalid dynamic array argument: length");
        }
        const Value::ArrayPtr array{std::get<Value::ArrayPtr>(array_value.storage())};
        if (!array || !array->dynamic)
        {
            throw std::runtime_error("invalid dynamic array argument: length");
        }
        return Value{static_cast<int>(array->elements.size())};
    }

    std::optional<Value> call_builtin(const std::string &name, const std::vector<ast::Expr> &args)
    {
        if (name == "setLength")
        {
            return call_set_length(args);
        }
        if (name == "length")
        {
            return call_length(args);
        }
        if (name == "print")
        {
            std::ostringstream out;
            for (const ast::Expr &arg : args)
            {
                out << format_value(interpret(arg));
            }
            m_state.add_message(out.str());
            return Value{};
        }
        if (name == "rgb" || name == "hsl")
        {
            check_arity(name, args.size(), 3U);
            const double first{real_part(convert_value(interpret(args[0]), ValueKind::FLOAT))};
            const double second{real_part(convert_value(interpret(args[1]), ValueKind::FLOAT))};
            const double third{real_part(convert_value(interpret(args[2]), ValueKind::FLOAT))};
            return Value{
                name == "rgb" ? ColorValue{first, second, third, 1.0} : hsl_to_color(first, second, third, 1.0)};
        }
        if (name == "rgba" || name == "hsla")
        {
            check_arity(name, args.size(), 4U);
            const double first{real_part(convert_value(interpret(args[0]), ValueKind::FLOAT))};
            const double second{real_part(convert_value(interpret(args[1]), ValueKind::FLOAT))};
            const double third{real_part(convert_value(interpret(args[2]), ValueKind::FLOAT))};
            const double fourth{real_part(convert_value(interpret(args[3]), ValueKind::FLOAT))};
            return Value{
                name == "rgba" ? ColorValue{first, second, third, fourth} : hsl_to_color(first, second, third, fourth)};
        }
        if (name == "red" || name == "green" || name == "blue" || name == "alpha" || name == "hue" || name == "sat" ||
            name == "lum")
        {
            check_arity(name, args.size(), 1U);
            const ColorValue color{color_value(interpret(args.front()))};
            if (name == "red")
            {
                return Value{color.red};
            }
            if (name == "green")
            {
                return Value{color.green};
            }
            if (name == "blue")
            {
                return Value{color.blue};
            }
            if (name == "alpha")
            {
                return Value{color.alpha};
            }
            const HslValue hsl{color_to_hsl(color)};
            if (name == "hue")
            {
                return Value{hsl.hue};
            }
            if (name == "sat")
            {
                return Value{hsl.saturation};
            }
            return Value{hsl.luminance};
        }
        if (name == "random")
        {
            check_arity(name, args.size(), 1U);
            return Value{random_seed(std::get<int>(convert_value(interpret(args.front()), ValueKind::INT).storage()))};
        }
        if (name == "atan2")
        {
            check_arity(name, args.size(), 1U);
            const Complex value{complex_value(interpret(args.front()))};
            return Value{std::atan2(value.im, value.re)};
        }
        if (name == "isNaN" || name == "isInf")
        {
            check_arity(name, args.size(), 1U);
            const double value{real_part(convert_value(interpret(args.front()), ValueKind::FLOAT))};
            return Value{name == "isNaN" ? std::isnan(value) : std::isinf(value)};
        }
        if (lookup_complex(name) != nullptr || lookup_real(name) != nullptr)
        {
            check_arity(name, args.size(), 1U);
            return Value{evaluate(name, complex_value(interpret(args.front())))};
        }
        return std::nullopt;
    }

    Value call_parameter_function(std::string_view name, const std::vector<ast::Expr> &args)
    {
        const Value target{m_state.parameter_value(name)};
        if (target.kind() != ValueKind::STRING)
        {
            throw std::runtime_error("invalid function parameter: " + std::string{name});
        }
        const std::string &target_name{std::get<std::string>(target.storage())};
        if (const std::optional<Value> builtin{call_builtin(target_name, args)})
        {
            return *builtin;
        }
        const auto function = m_functions.find(target_name);
        if (function == m_functions.end())
        {
            throw std::runtime_error("invalid function parameter target: " + target_name);
        }
        return call_function(*function->second, args);
    }

    std::optional<Value> call_member(const ast::FunctionCallNode &node)
    {
        Value target_value{interpret(node.target())};
        if (target_value.kind() == ValueKind::PLUGIN)
        {
            return call_object_method(target_value, node);
        }
        if (target_value.kind() != ValueKind::IMAGE)
        {
            return std::nullopt;
        }
        const std::string &name{node.name()};
        const std::vector<ast::Expr> &args{node.args()};
        Value::ImagePtr image{image_value(target_value)};
        if (name == "assign")
        {
            check_arity(name, args.size(), 1U);
            const Value source{interpret(args.front())};
            RuntimeLValue target{lvalue(node.target())};
            target.set(source.kind() == ValueKind::EMPTY ? default_value(ValueKind::IMAGE) : source);
            return Value{};
        }
        if (name == "getEmpty")
        {
            check_arity(name, args.size(), 0U);
            return Value{!image || image->empty};
        }
        if (name == "getWidth")
        {
            check_arity(name, args.size(), 0U);
            return Value{image && !image->empty ? image->width : 0};
        }
        if (name == "getHeight")
        {
            check_arity(name, args.size(), 0U);
            return Value{image && !image->empty ? image->height : 0};
        }
        if (name == "getPixel")
        {
            check_arity(name, args.size(), 2U);
            const int x{std::get<int>(convert_value(interpret(args[0]), ValueKind::INT).storage())};
            const int y{std::get<int>(convert_value(interpret(args[1]), ValueKind::INT).storage())};
            return Value{image ? image_pixel(*image, x, y) : transparent_color()};
        }
        if (name == "setPixel")
        {
            check_arity(name, args.size(), 3U);
            const int x{std::get<int>(convert_value(interpret(args[0]), ValueKind::INT).storage())};
            const int y{std::get<int>(convert_value(interpret(args[1]), ValueKind::INT).storage())};
            const ColorValue color{color_value(interpret(args[2]))};
            if (image && !image->empty && x >= 0 && y >= 0 && x < image->width && y < image->height)
            {
                image->pixels[image_pixel_index(*image, x, y)] = color;
            }
            return Value{};
        }
        if (name == "resize")
        {
            check_arity(name, args.size(), 2U);
            const int width{std::get<int>(convert_value(interpret(args[0]), ValueKind::INT).storage())};
            const int height{std::get<int>(convert_value(interpret(args[1]), ValueKind::INT).storage())};
            resize_image(*image, width, height);
            return Value{};
        }
        if (name == "getColor")
        {
            check_arity(name, args.size(), 1U);
            const Complex point{complex_value(interpret(args.front()))};
            if (!image || image->empty)
            {
                return Value{transparent_color()};
            }
            const int x{static_cast<int>(((point.re + 1.0) / 2.0) * image->width)};
            const int y{static_cast<int>(((1.0 - point.im) / 2.0) * image->height)};
            return Value{image_pixel(*image, x, y)};
        }
        return std::nullopt;
    }

    std::optional<Value> call_object_method(const Value &target_value, const ast::FunctionCallNode &node)
    {
        const Value::PluginPtr object{std::get<Value::PluginPtr>(target_value.storage())};
        if (!object || !object->object_initialized)
        {
            throw std::runtime_error("null object reference");
        }
        const ast::FunctionDeclNode *method{};
        if (object->ast)
        {
            method = find_class_method(*object->ast, node.name());
        }
        std::vector<std::string> seen;
        std::string base{object->base_class};
        while (method == nullptr && !base.empty())
        {
            if (std::find_if(seen.begin(), seen.end(),
                    [&base](const std::string &item) { return same_identifier(item, base); }) != seen.end())
            {
                break;
            }
            seen.push_back(base);
            const RetainedFormulaClass *klass{find_retained_class_by_name(m_files.retained_classes, base)};
            if (klass == nullptr)
            {
                break;
            }
            if (klass->ast)
            {
                method = find_class_method(*klass->ast, node.name());
            }
            base = klass->base_class;
        }
        if (method == nullptr)
        {
            const ast::FunctionDeclNode *static_method{find_static_method(*object, node.name())};
            if (static_method == nullptr)
            {
                return std::nullopt;
            }
            return call_function(*static_method, node.args());
        }
        return call_method(target_value, *method, node.args());
    }

    const ast::FunctionDeclNode *find_static_method(const PluginValue &object, std::string_view name) const
    {
        const ast::FunctionDeclNode *method{};
        if (object.ast)
        {
            method = find_static_class_method(*object.ast, name);
        }
        std::vector<std::string> seen;
        std::string base{object.base_class};
        while (method == nullptr && !base.empty())
        {
            if (std::find_if(seen.begin(), seen.end(),
                    [&base](const std::string &item) { return same_identifier(item, base); }) != seen.end())
            {
                break;
            }
            seen.push_back(base);
            const RetainedFormulaClass *klass{find_retained_class_by_name(m_files.retained_classes, base)};
            if (klass == nullptr)
            {
                break;
            }
            if (klass->ast)
            {
                method = find_static_class_method(*klass->ast, name);
            }
            base = klass->base_class;
        }
        return method;
    }

    std::optional<Value> call_static_member(const ast::FunctionCallNode &node)
    {
        const std::string *class_name{class_name_from_identifier(node.target())};
        if (class_name == nullptr)
        {
            return std::nullopt;
        }
        const ast::FunctionDeclNode *method{find_static_method(*class_name, node.name())};
        if (method == nullptr)
        {
            return std::nullopt;
        }
        return call_function(*method, node.args());
    }

    const ast::FunctionDeclNode *find_static_method(std::string_view class_name, std::string_view name) const
    {
        const RetainedFormulaClass *klass{find_retained_class_by_name(m_files.retained_classes, class_name)};
        if (klass == nullptr)
        {
            return nullptr;
        }
        const ast::FunctionDeclNode *method{};
        if (klass->ast)
        {
            method = find_static_class_method(*klass->ast, name);
        }
        std::vector<std::string> seen;
        std::string base{klass->base_class};
        while (method == nullptr && !base.empty())
        {
            if (std::find_if(seen.begin(), seen.end(),
                    [&base](const std::string &item) { return same_identifier(item, base); }) != seen.end())
            {
                break;
            }
            seen.push_back(base);
            klass = find_retained_class_by_name(m_files.retained_classes, base);
            if (klass == nullptr)
            {
                break;
            }
            if (klass->ast)
            {
                method = find_static_class_method(*klass->ast, name);
            }
            base = klass->base_class;
        }
        return method;
    }

    std::optional<Value> class_constant_value(const ast::MemberAccessNode &node)
    {
        const std::string *class_name{class_name_from_identifier(node.target())};
        if (class_name == nullptr)
        {
            return std::nullopt;
        }
        const ast::DeclarationNode *declaration{find_class_declaration(*class_name, node.member())};
        if (declaration == nullptr)
        {
            return std::nullopt;
        }
        if (declaration->is_array())
        {
            return declaration->is_dynamic_array() ? make_dynamic_array(*declaration) : make_static_array(*declaration);
        }
        if (const std::optional<ValueKind> kind{parameter_value_kind(declaration->type())})
        {
            return declaration->initializer() ? convert_value(interpret(declaration->initializer()), *kind)
                                              : default_value(*kind);
        }
        return declaration->initializer() ? interpret(declaration->initializer())
                                          : make_plugin_value(PluginValue{{}, std::string{declaration->type()}});
    }

    Value construct_object(const RetainedFormulaClass &klass, const std::vector<ast::Expr> &args)
    {
        PluginValue plugin;
        plugin.filename = klass.reference.filename;
        plugin.class_name = klass.reference.class_name;
        plugin.base_class = klass.base_class;
        plugin.ast = klass.ast;
        return construct_object(plugin, args);
    }

    Value construct_object(const PluginValue &plugin, const std::vector<ast::Expr> &args)
    {
        Value object{make_object_value(plugin)};
        const Value::PluginPtr object_plugin{std::get<Value::PluginPtr>(object.storage())};
        const ast::FunctionDeclNode *constructor{};
        if (object_plugin && object_plugin->ast)
        {
            constructor = find_class_constructor(*object_plugin->ast, object_plugin->class_name);
        }
        if (constructor == nullptr)
        {
            check_arity(object_plugin ? object_plugin->class_name : std::string{"constructor"}, args.size(), 0U);
            return object;
        }
        call_method(object, *constructor, args);
        return object;
    }

    Value cast_object(const RetainedFormulaClass &klass, const std::vector<ast::Expr> &args)
    {
        check_arity(klass.reference.class_name, args.size(), 1U);
        const Value value{interpret(args.front())};
        if (value.kind() != ValueKind::PLUGIN)
        {
            return {};
        }
        const Value::PluginPtr plugin{std::get<Value::PluginPtr>(value.storage())};
        if (!plugin || !plugin->object_initialized)
        {
            return {};
        }
        if (!runtime_class_matches_parameter_type(
                m_files.retained_classes, plugin->class_name, klass.reference.class_name))
        {
            return {};
        }
        return value;
    }

    const ast::DeclarationNode *find_class_declaration(std::string_view class_name, std::string_view name) const
    {
        const RetainedFormulaClass *klass{find_retained_class_by_name(m_files.retained_classes, class_name)};
        if (klass == nullptr)
        {
            return nullptr;
        }
        const ast::DeclarationNode *declaration{};
        if (klass->ast)
        {
            declaration = find_class_declaration_in_ast(*klass->ast, name);
        }
        std::vector<std::string> seen;
        std::string base{klass->base_class};
        while (declaration == nullptr && !base.empty())
        {
            if (std::find_if(seen.begin(), seen.end(),
                    [&base](const std::string &item) { return same_identifier(item, base); }) != seen.end())
            {
                break;
            }
            seen.push_back(base);
            klass = find_retained_class_by_name(m_files.retained_classes, base);
            if (klass == nullptr)
            {
                break;
            }
            if (klass->ast)
            {
                declaration = find_class_declaration_in_ast(*klass->ast, name);
            }
            base = klass->base_class;
        }
        return declaration;
    }

    RuntimeLValue array_lvalue(const ast::IndexNode &node)
    {
        const Value array_value{interpret(node.target())};
        if (array_value.kind() != ValueKind::ARRAY)
        {
            throw std::runtime_error("expected array value");
        }
        const Value::ArrayPtr array{std::get<Value::ArrayPtr>(array_value.storage())};
        if (!array)
        {
            throw std::runtime_error("expected array value");
        }
        if (node.indices().size() != array->dimensions.size())
        {
            throw std::runtime_error("invalid array index count");
        }
        std::size_t flat{};
        for (std::size_t i = 0; i < node.indices().size(); ++i)
        {
            const Value value{convert_value(interpret(node.indices()[i]), ValueKind::INT)};
            const int index{std::get<int>(value.storage())};
            if (index < 0 || index >= array->dimensions[i])
            {
                throw std::runtime_error("array index out of range");
            }
            flat = flat * static_cast<std::size_t>(array->dimensions[i]) + static_cast<std::size_t>(index);
        }
        return RuntimeLValue::array_element(array, flat);
    }

    Value call_function(const ast::FunctionDeclNode &function, const std::vector<ast::Expr> &args)
    {
        if (args.size() != function.args().size())
        {
            throw std::runtime_error("invalid call arity: " + function.name());
        }
        const bool caller_returning{m_returning};
        const Value caller_result{m_result};
        bool frame_active{true};
        m_returning = false;
        m_state.push_function_frame();
        try
        {
            bind_function_arguments(function, args);
            const Value body_result{interpret(function.body())};
            Value result{m_returning ? m_result : body_result};
            m_state.pop_function_frame();
            frame_active = false;
            if (!m_returning && !is_void_return(function.return_type()))
            {
                throw std::runtime_error("missing return: " + function.name());
            }
            if (!is_void_return(function.return_type()))
            {
                result = convert_value(result, value_kind_for_type(function.return_type()));
            }
            else
            {
                result = {};
            }
            m_returning = caller_returning;
            m_result = caller_returning ? caller_result : result;
            return result;
        }
        catch (...)
        {
            if (frame_active)
            {
                m_state.pop_function_frame();
            }
            m_returning = caller_returning;
            m_result = caller_result;
            throw;
        }
    }

    Value call_method(const Value &object, const ast::FunctionDeclNode &function, const std::vector<ast::Expr> &args)
    {
        if (args.size() != function.args().size())
        {
            throw std::runtime_error("invalid call arity: " + function.name());
        }
        const bool caller_returning{m_returning};
        const Value caller_result{m_result};
        bool frame_active{true};
        m_returning = false;
        m_state.push_function_frame();
        try
        {
            m_state.declare_local_value("this", object);
            bind_function_arguments(function, args);
            const Value body_result{interpret(function.body())};
            Value result{m_returning ? m_result : body_result};
            m_state.pop_function_frame();
            frame_active = false;
            if (!m_returning && !is_void_return(function.return_type()))
            {
                throw std::runtime_error("missing return: " + function.name());
            }
            if (!is_void_return(function.return_type()))
            {
                result = convert_value(result, value_kind_for_type(function.return_type()));
            }
            else
            {
                result = {};
            }
            m_returning = caller_returning;
            m_result = caller_returning ? caller_result : result;
            return result;
        }
        catch (...)
        {
            if (frame_active)
            {
                m_state.pop_function_frame();
            }
            m_returning = caller_returning;
            m_result = caller_result;
            throw;
        }
    }

    void bind_function_arguments(const ast::FunctionDeclNode &function, const std::vector<ast::Expr> &args)
    {
        for (std::size_t i = 0; i < args.size(); ++i)
        {
            const ast::FunctionArgument &arg{function.args()[i]};
            const ValueKind kind{value_kind_for_type(arg.type)};
            if (arg.is_by_ref)
            {
                RuntimeLValue value{lvalue(args[i])};
                m_state.bind_local_reference(arg.name, arg.is_const ? value.read_only() : value);
            }
            else
            {
                m_state.declare_local_value(arg.name, convert_value(interpret(args[i]), kind), !arg.is_const);
            }
        }
    }

    Value interpret_block(const ast::Expr &node)
    {
        m_state.push_local_scope();
        try
        {
            Value result{interpret(node)};
            m_state.pop_local_scope();
            return result;
        }
        catch (...)
        {
            m_state.pop_local_scope();
            throw;
        }
    }

    void check_loop_iterations(std::size_t iterations) const
    {
        if (iterations > m_max_loop_iterations)
        {
            throw std::runtime_error("loop iteration limit exceeded");
        }
    }

    RuntimeLValue lvalue(const ast::Expr &node)
    {
        if (const auto *identifier = dynamic_cast<const ast::IdentifierNode *>(node.get()); identifier)
        {
            return m_state.lvalue(identifier->name());
        }
        if (const auto *predefined = dynamic_cast<const ast::ConstantRefNode *>(node.get()); predefined)
        {
            return m_state.predefined_lvalue(predefined->name());
        }
        if (const auto *parameter = dynamic_cast<const ast::ParameterRefNode *>(node.get()); parameter)
        {
            return m_state.parameter_lvalue(parameter->name());
        }
        if (const auto *index = dynamic_cast<const ast::IndexNode *>(node.get()); index)
        {
            return array_lvalue(*index);
        }
        if (const auto *member = dynamic_cast<const ast::MemberAccessNode *>(node.get()); member)
        {
            return member_lvalue(*member);
        }
        throw std::runtime_error("invalid assignment target");
    }

    RuntimeLValue member_lvalue(const ast::MemberAccessNode &node)
    {
        const Value target{interpret(node.target())};
        if (target.kind() != ValueKind::PLUGIN)
        {
            throw std::runtime_error("expected object value");
        }
        const Value::PluginPtr object{std::get<Value::PluginPtr>(target.storage())};
        if (!object || !object->object_initialized)
        {
            throw std::runtime_error("null object reference");
        }
        const std::string member{node.member()};
        return RuntimeLValue{[object, member]()
            {
                const auto found{std::find_if(object->object_fields.begin(), object->object_fields.end(),
                    [&member](const auto &field) { return field.first == member; })};
                if (found == object->object_fields.end())
                {
                    throw std::runtime_error("unknown object field: " + member);
                }
                return found->second;
            },
            [object, member](Value value)
            {
                const auto found{std::find_if(object->object_fields.begin(), object->object_fields.end(),
                    [&member](const auto &field) { return field.first == member; })};
                if (found == object->object_fields.end())
                {
                    throw std::runtime_error("unknown object field: " + member);
                }
                found->second = std::move(value);
            },
            true};
    }

    ExtendedRuntimeState &m_state;
    FunctionMap m_functions;
    const FormulaFileSet &m_files;
    std::size_t m_max_loop_iterations{};
    Value m_result;
    bool m_returning{};
};

class ParameterDefaultCollector : public ast::NullVisitor
{
public:
    ParameterDefaultCollector(ExtendedRuntimeState &state, const FunctionMap &functions, const FormulaFileSet &files,
        std::size_t max_loop_iterations) :
        m_state(state),
        m_functions(functions),
        m_files(files),
        m_max_loop_iterations(max_loop_iterations)
    {
    }

    void collect(const ast::Expr &node)
    {
        if (node)
        {
            node->visit(*this);
        }
    }

    void visit(const ast::ParamBlockNode &node) override
    {
        m_current_parameter = &node;
        m_current_enum_values.clear();
        collect(node.block());
        if (!m_state.has_parameter_value(node.name()))
        {
            Value value{implicit_parameter_default(node.type())};
            if (!m_current_enum_values.empty())
            {
                value = *make_enum_parameter_value(
                    RuntimeParameterInfo{node.type(), node.name(), m_current_enum_values, {}, false}, value);
            }
            m_state.set_parameter_value(node.name(), std::move(value));
        }
        m_current_parameter = nullptr;
        m_current_enum_values.clear();
    }

    void visit(const ast::FunctionBlockNode &node) override
    {
        m_current_function = &node;
        collect(node.block());
        if (!m_state.has_parameter_value(node.name()))
        {
            m_state.set_parameter_value(
                node.name(), Value{std::string{node.type() == "color" ? "mergenormal" : "sin"}});
        }
        m_current_function = nullptr;
    }

    void visit(const ast::SettingNode &node) override
    {
        if (node.key() != "default")
        {
            if (node.key() == "enum" && m_current_parameter != nullptr)
            {
                if (const auto *values = std::get_if<std::vector<std::string>>(&node.value()))
                {
                    m_current_enum_values = *values;
                }
            }
            return;
        }
        if (m_current_function != nullptr && !m_state.has_parameter_value(m_current_function->name()))
        {
            if (const std::optional<std::string> name{function_name_value(node.value())})
            {
                m_state.set_parameter_value(m_current_function->name(), Value{*name});
            }
            return;
        }
        if (m_current_parameter == nullptr || m_state.has_parameter_value(m_current_parameter->name()))
        {
            return;
        }
        Value value;
        if (const auto *expr = std::get_if<ast::Expr>(&node.value()))
        {
            value = ExpressionInterpreter{m_state, m_functions, m_files, m_max_loop_iterations}.interpret(*expr);
        }
        else
        {
            value = value_from_setting(node.value());
        }
        value = convert_parameter_default(value, m_current_parameter->type());
        if (!m_current_enum_values.empty())
        {
            value = *make_enum_parameter_value(RuntimeParameterInfo{m_current_parameter->type(),
                                                   m_current_parameter->name(), m_current_enum_values, {}, false},
                value);
        }
        m_state.set_parameter_value(m_current_parameter->name(), std::move(value));
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        for (const ast::Expr &statement : node.statements())
        {
            collect(statement);
        }
    }

private:
    ExtendedRuntimeState &m_state;
    FunctionMap m_functions;
    const FormulaFileSet &m_files;
    std::size_t m_max_loop_iterations{};
    const ast::ParamBlockNode *m_current_parameter{};
    const ast::FunctionBlockNode *m_current_function{};
    std::vector<std::string> m_current_enum_values;
};

const ast::Expr &section_expr(const ast::FormulaSections &formula, Section section)
{
    switch (section)
    {
    case Section::PER_IMAGE:
        return formula.per_image;
    case Section::BUILTIN:
        return formula.builtin;
    case Section::INITIALIZE:
        return formula.initialize;
    case Section::ITERATE:
        return formula.iterate;
    case Section::BAILOUT:
        return formula.bailout;
    case Section::PERTURB_INITIALIZE:
        return formula.perturb_initialize;
    case Section::PERTURB_ITERATE:
        return formula.perturb_iterate;
    case Section::DEFAULT:
        return formula.defaults;
    case Section::SWITCH:
        return formula.type_switch;
    case Section::FINAL:
        return formula.final;
    case Section::TRANSFORM:
        return formula.transform;
    default:
        throw std::runtime_error("invalid section");
    }
}

bool section_allowed(parser::EntryKind kind, Section section)
{
    switch (kind)
    {
    case parser::EntryKind::FRACTAL:
        return section == Section::PER_IMAGE || section == Section::INITIALIZE || section == Section::ITERATE ||
            section == Section::BAILOUT || section == Section::PERTURB_INITIALIZE ||
            section == Section::PERTURB_ITERATE;
    case parser::EntryKind::COLORING:
        return section == Section::PER_IMAGE || section == Section::INITIALIZE || section == Section::ITERATE ||
            section == Section::FINAL;
    case parser::EntryKind::TRANSFORMATION:
        return section == Section::PER_IMAGE || section == Section::TRANSFORM;
    case parser::EntryKind::CLASS:
        return false;
    }
    throw std::runtime_error("unknown entry kind");
}

Value section_result(Section section, const Value &value)
{
    switch (section)
    {
    case Section::BAILOUT:
        return Value{is_truthy(value)};
    case Section::FINAL:
        if (value.kind() == ValueKind::EMPTY || value.kind() == ValueKind::COLOR || is_numeric(value.kind()))
        {
            return value;
        }
        throw std::runtime_error("invalid final section result");
    case Section::TRANSFORM:
        return {};
    default:
        return value;
    }
}

} // namespace

ExtendedInterpreter::ExtendedInterpreter(FileEntry entry, ExtendedInterpreterOptions options) :
    m_entry(std::move(entry)),
    m_options(std::move(options))
{
    parse();
    resolve_references();
    analyze();
}

const std::vector<ExtendedInterpreterDiagnostic> &ExtendedInterpreter::diagnostics() const
{
    return m_diagnostics;
}

bool ExtendedInterpreter::ok() const
{
    return m_construction_diagnostics.empty() && m_binding_diagnostics.empty();
}

const std::vector<semantic::FormulaParameterInfo> &ExtendedInterpreter::parameters() const
{
    return m_parameters;
}

void ExtendedInterpreter::set_value(std::string_view name, Value value)
{
    const semantic::BuiltinRegistry &builtins{builtins_or_default(m_options.builtins)};
    if (!name.empty() && name.front() == '@')
    {
        std::string_view parameter_name{name};
        parameter_name.remove_prefix(1);
        clear_binding_diagnostics(parameter_name);
        if (m_ast)
        {
            const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*m_ast)};
            if (const RuntimeParameterInfo *parameter{find_runtime_parameter(metadata, parameter_name)})
            {
                try
                {
                    if (const std::optional<Value> enum_value{make_enum_parameter_value(*parameter, value)})
                    {
                        value = *enum_value;
                    }
                }
                catch (const std::runtime_error &error)
                {
                    add_binding_diagnostic(parameter_name, error.what());
                    return;
                }
            }
        }
        if (value.kind() != ValueKind::ENUM)
        {
            if (const auto found = std::find_if(m_parameters.begin(), m_parameters.end(),
                    [parameter_name](const auto &parameter) { return parameter.name == parameter_name; });
                found != m_parameters.end())
            {
                if (const std::optional<ValueKind> kind{parameter_value_kind(found->type)})
                {
                    try
                    {
                        value = convert_value(value, *kind);
                    }
                    catch (const std::runtime_error &error)
                    {
                        add_binding_diagnostic(parameter_name, error.what());
                        return;
                    }
                }
            }
        }
        m_state.set_parameter_value(name, std::move(value));
        return;
    }
    if (!name.empty() && name.front() == '#')
    {
        std::string_view symbol{name};
        symbol.remove_prefix(1);
        const semantic::SemanticPredefinedSymbolDescriptor *descriptor{builtins.find_predefined_symbol(symbol)};
        m_state.set_predefined_value(name, std::move(value), descriptor == nullptr || descriptor->writable);
        return;
    }
    m_state.set_formula_value(name, std::move(value));
}

void ExtendedInterpreter::set_parameter(std::string_view name, Value value)
{
    if (set_nonselectable_plugin_parameter_value(name, value))
    {
        return;
    }
    set_value("@" + std::string{name}, std::move(value));
}

bool ExtendedInterpreter::set_nonselectable_plugin_parameter_value(std::string_view name, Value value)
{
    if (!m_ast || find_runtime_parameter(collect_runtime_parameter_metadata(*m_ast), name) != nullptr)
    {
        return false;
    }

    const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*m_ast)};
    for (const RuntimeParameterInfo &parameter : metadata.params)
    {
        if (parameter.selectable || !is_runtime_plugin_parameter(m_parameters, parameter.name))
        {
            continue;
        }
        const Value current{m_state.parameter_value(parameter.name)};
        if (current.kind() != ValueKind::PLUGIN)
        {
            continue;
        }
        const Value::PluginPtr plugin{std::get<Value::PluginPtr>(current.storage())};
        if (!plugin || !plugin_has_nested_runtime_parameter(*plugin, name))
        {
            continue;
        }
        set_plugin_parameter_value(parameter.name, name, std::move(value));
        return true;
    }
    return false;
}

void ExtendedInterpreter::set_function_parameter(std::string_view name, std::string_view target)
{
    clear_binding_diagnostics(name);
    if (!m_ast)
    {
        add_binding_diagnostic(name, "invalid function target: " + std::string{target});
        return;
    }
    const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*m_ast)};
    const RuntimeParameterInfo *function{find_runtime_function(metadata, name)};
    if (function == nullptr)
    {
        add_binding_diagnostic(name, "invalid function parameter: " + std::string{name});
        return;
    }

    const semantic::SemanticTypeKind type{semantic_type_kind(function->type)};
    const semantic::BuiltinRegistry &builtins{builtins_or_default(m_options.builtins)};
    for (const semantic::SemanticFunctionDescriptor &builtin : builtins.functions)
    {
        if (same_identifier(builtin.name, target) && function_target_matches_type(builtin, type))
        {
            m_state.set_parameter_value(name, Value{builtin.name});
            return;
        }
    }

    const FunctionMap functions{collect_function_declarations(*m_ast)};
    const auto user_function{functions.find(std::string{target})};
    if (user_function != functions.end() && function_target_matches_type(*user_function->second, type))
    {
        m_state.set_parameter_value(name, Value{std::string{target}});
        return;
    }

    add_binding_diagnostic(name, "invalid function target: " + std::string{target});
}

void ExtendedInterpreter::set_plugin_parameter(std::string_view name, std::string_view selector)
{
    clear_binding_diagnostics(name);
    const auto parameter = std::find_if(m_parameters.begin(), m_parameters.end(),
        [name](const semantic::FormulaParameterInfo &info) { return info.name == name; });
    if (!m_ast || parameter == m_parameters.end() || !parameter->is_plugin)
    {
        set_parameter(name, Value{std::string{selector}});
        return;
    }
    const std::optional<RuntimeEntrySelector> parsed{parse_runtime_entry_selector(selector)};
    if (!parsed)
    {
        add_binding_diagnostic(name, "invalid plug-in target: " + std::string{selector});
        return;
    }
    const RetainedFormulaClass *klass{};
    FormulaFileSet selector_files;
    FormulaFileSet *files{&m_files};
    if (const FormulaClassReference *reference{find_runtime_class_reference(*files, *parsed)})
    {
        const std::size_t diagnostics_size{files->diagnostics.size()};
        retain_formula_class(*files, *reference);
        retain_resolved_imported_classes(*files);
        if (files->diagnostics.size() != diagnostics_size)
        {
            add_binding_diagnostic(name, reference_diagnostic_message(files->diagnostics.back()));
            return;
        }
        klass = find_retained_runtime_class(files->retained_classes, *parsed);
    }
    else if (m_options.parser.file_importer && !parsed->filename.empty())
    {
        selector_files = load_formula_file_tree(parsed->filename, m_options.parser.file_importer);
        files = &selector_files;
        if (!files->diagnostics.empty())
        {
            add_binding_diagnostic(name, reference_diagnostic_message(files->diagnostics.front()));
            return;
        }
        const FormulaClassReference *reference{find_runtime_class_reference(*files, *parsed)};
        if (reference != nullptr)
        {
            retain_formula_class(*files, *reference);
            retain_resolved_imported_classes(*files);
            if (!files->diagnostics.empty())
            {
                add_binding_diagnostic(name, reference_diagnostic_message(files->diagnostics.back()));
                return;
            }
            klass = find_retained_runtime_class(files->retained_classes, *parsed);
        }
    }
    if (klass == nullptr)
    {
        add_binding_diagnostic(name, "missing plug-in class: " + std::string{selector});
        return;
    }
    if (!runtime_class_matches_parameter_type(files->retained_classes, klass->reference.class_name, parameter->type))
    {
        add_binding_diagnostic(name, "invalid plug-in target: " + std::string{selector});
        return;
    }
    semantic::FormulaSemanticContext context;
    context.entry_kind = parser::EntryKind::CLASS;
    context.source_name = klass->reference.filename;
    context.retained_classes = retained_class_ptrs(files->retained_classes);
    context.builtins = m_options.builtins;
    const std::vector<semantic::SemanticDiagnostic> diagnostics{semantic::analyze_formula(*klass->ast, context)};
    for (const semantic::SemanticDiagnostic &diagnostic : diagnostics)
    {
        if (diagnostic.severity == semantic::SemanticDiagnosticSeverity::ERROR)
        {
            add_binding_diagnostic(name, diagnostic.message);
            return;
        }
    }
    set_parameter(name, make_plugin_parameter_value(*klass, *files));
}

void ExtendedInterpreter::set_plugin_parameter_value(
    std::string_view plugin_name, std::string_view nested_name, Value value)
{
    const std::string diagnostic_name{std::string{plugin_name} + "." + std::string{nested_name}};
    clear_binding_diagnostics(diagnostic_name);
    const Value current{m_state.parameter_value(plugin_name)};
    if (current.kind() == ValueKind::PLUGIN)
    {
        const Value::PluginPtr plugin{std::get<Value::PluginPtr>(current.storage())};
        PluginValue updated{plugin ? *plugin : PluginValue{}};
        const std::string key{nested_name};
        if (plugin && plugin->ast)
        {
            const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*plugin->ast)};
            if (const RuntimeParameterInfo *parameter{find_runtime_parameter(metadata, nested_name)};
                parameter != nullptr && !parameter_value_kind(parameter->type).has_value() &&
                value.kind() == ValueKind::STRING)
            {
                (void) find_or_retain_runtime_class(m_files, RuntimeEntrySelector{{}, parameter->type});
                const std::string selector{std::get<std::string>(value.storage())};
                const std::optional<RuntimeEntrySelector> parsed{parse_runtime_entry_selector(selector)};
                if (!parsed)
                {
                    add_binding_diagnostic(diagnostic_name, "invalid plug-in target: " + selector);
                    return;
                }
                const RetainedFormulaClass *klass{find_or_retain_runtime_class(m_files, *parsed)};
                FormulaFileSet selector_files;
                FormulaFileSet *files{&m_files};
                if (klass == nullptr && m_options.parser.file_importer && !parsed->filename.empty())
                {
                    selector_files = load_formula_file_tree(parsed->filename, m_options.parser.file_importer);
                    files = &selector_files;
                    if (!files->diagnostics.empty())
                    {
                        add_binding_diagnostic(
                            diagnostic_name, reference_diagnostic_message(files->diagnostics.front()));
                        return;
                    }
                    klass = find_or_retain_runtime_class(*files, *parsed);
                    if (!files->diagnostics.empty())
                    {
                        add_binding_diagnostic(
                            diagnostic_name, reference_diagnostic_message(files->diagnostics.back()));
                        return;
                    }
                }
                if (klass == nullptr)
                {
                    add_binding_diagnostic(diagnostic_name, "missing plug-in class: " + selector);
                    return;
                }
                if (!runtime_class_matches_parameter_type(
                        files->retained_classes, klass->reference.class_name, parameter->type))
                {
                    add_binding_diagnostic(diagnostic_name, "invalid plug-in target: " + selector);
                    return;
                }
                value = make_plugin_parameter_value(*klass, *files);
            }
        }
        const auto found{std::find_if(updated.nested_values.begin(), updated.nested_values.end(),
            [&key](const auto &nested) { return nested.first == key; })};
        if (found == updated.nested_values.end())
        {
            updated.nested_values.push_back({key, std::move(value)});
        }
        else
        {
            found->second = std::move(value);
        }
        m_state.set_parameter_value(plugin_name, make_plugin_value(std::move(updated)));
        return;
    }
    set_parameter(std::string{plugin_name} + "." + std::string{nested_name}, std::move(value));
}

Value ExtendedInterpreter::value(std::string_view name) const
{
    if (!name.empty() && name.front() == '@')
    {
        return m_state.parameter_value(name);
    }
    if (!name.empty() && name.front() == '#')
    {
        return m_state.predefined_value(name);
    }
    return m_state.value(name);
}

const std::vector<std::string> &ExtendedInterpreter::messages() const
{
    return m_state.messages();
}

Value ExtendedInterpreter::interpret(Section section)
{
    if (!ok())
    {
        throw std::runtime_error("extended interpreter has diagnostics");
    }
    if (!m_ast)
    {
        throw std::runtime_error("extended interpreter has no AST");
    }
    if (!section_allowed(m_options.parser.entry_kind, section))
    {
        throw std::runtime_error("invalid section for entry kind");
    }
    if (!section_expr(*m_ast, section))
    {
        return {};
    }
    FunctionMap functions{collect_function_declarations(*m_ast)};
    const Value result{ExpressionInterpreter{m_state, functions, m_files, m_options.max_loop_iterations}.interpret(
        section_expr(*m_ast, section))};
    return section_result(section, result);
}

void ExtendedInterpreter::parse()
{
    const parser::ParserPtr parser{parser::create_parser(m_entry.body, m_options.parser)};
    m_ast = parser->parse();
    add_parse_diagnostics(*parser);
    rebuild_diagnostics();
}

void ExtendedInterpreter::resolve_references()
{
    if (!m_ast || !m_options.parser.file_importer || m_options.parser.source_filename.empty())
    {
        return;
    }
    m_files = load_formula_file_tree(m_options.parser.source_filename, m_options.parser.file_importer);
    resolve_root_formula_file_references(m_files);
    retain_resolved_imported_classes(m_files);
    add_reference_diagnostics();
    rebuild_diagnostics();
}

void ExtendedInterpreter::analyze()
{
    if (!m_ast || !m_construction_diagnostics.empty())
    {
        return;
    }
    std::vector<const RetainedFormulaClass *> retained_classes;
    retained_classes.reserve(m_files.retained_classes.size());
    for (const RetainedFormulaClass &retained : m_files.retained_classes)
    {
        retained_classes.push_back(&retained);
    }
    semantic::FormulaSemanticContext context;
    context.entry_kind = m_options.parser.entry_kind;
    context.source_name = m_options.parser.source_filename.empty() ? m_entry.name : m_options.parser.source_filename;
    context.retained_classes = std::move(retained_classes);
    context.builtins = m_options.builtins;
    m_parameters = semantic::collect_formula_parameters(*m_ast, context);
    add_semantic_diagnostics(semantic::analyze_formula(*m_ast, context));
    initialize_runtime_state();
    rebuild_diagnostics();
}

void ExtendedInterpreter::initialize_runtime_state()
{
    if (!m_ast || !m_construction_diagnostics.empty())
    {
        return;
    }
    const semantic::BuiltinRegistry &builtins{builtins_or_default(m_options.builtins)};
    for (const semantic::SemanticPredefinedSymbolDescriptor &symbol : builtins.predefined_symbols)
    {
        if (m_state.has_predefined_value(symbol.name))
        {
            continue;
        }
        Value value{default_value(value_kind_for_type(symbol.type.kind))};
        if (symbol.name == "pi")
        {
            value = Value{std::atan2(0.0, -1.0)};
        }
        else if (symbol.name == "e")
        {
            value = Value{std::exp(1.0)};
        }
        else if (symbol.name == "randomrange")
        {
            value = Value{2147483648.0};
        }
        m_state.set_predefined_value(symbol.name, std::move(value), symbol.writable);
    }
    FunctionMap functions{collect_function_declarations(*m_ast)};
    ParameterDefaultCollector{m_state, functions, m_files, m_options.max_loop_iterations}.collect(m_ast->defaults);
    const RuntimeParameterMetadata metadata{collect_runtime_parameter_metadata(*m_ast)};
    for (const RuntimeParameterInfo &parameter : metadata.params)
    {
        if (!is_runtime_plugin_parameter(m_parameters, parameter.name))
        {
            continue;
        }
        std::string selector{parameter.default_selector.value_or(parameter.type)};
        const Value current{m_state.parameter_value(parameter.name)};
        if (current.kind() == ValueKind::STRING)
        {
            selector = std::get<std::string>(current.storage());
        }
        const std::optional<RuntimeEntrySelector> parsed{parse_runtime_entry_selector(selector)};
        if (!parsed)
        {
            continue;
        }
        if (const RetainedFormulaClass *klass{find_retained_runtime_class(m_files.retained_classes, *parsed)})
        {
            m_state.set_parameter_value(parameter.name, make_plugin_parameter_value(*klass, m_files));
        }
    }
}

void ExtendedInterpreter::add_parse_diagnostics(const parser::Parser &parser)
{
    for (const parser::Diagnostic &diagnostic : parser.get_errors())
    {
        m_construction_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::PARSE,
            diagnostic.position, m_options.parser.source_filename, parser::to_string(diagnostic.code)});
    }
}

void ExtendedInterpreter::add_reference_diagnostics()
{
    for (const FormulaFileDiagnostic &diagnostic : m_files.diagnostics)
    {
        m_construction_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::REFERENCE,
            diagnostic.location, diagnostic.filename, reference_diagnostic_message(diagnostic)});
    }
}

void ExtendedInterpreter::add_semantic_diagnostics(const std::vector<semantic::SemanticDiagnostic> &diagnostics)
{
    for (const semantic::SemanticDiagnostic &diagnostic : diagnostics)
    {
        if (diagnostic.severity != semantic::SemanticDiagnosticSeverity::ERROR)
        {
            continue;
        }
        m_construction_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::SEMANTIC,
            diagnostic.location, diagnostic.entry_name, diagnostic.message});
    }
}

void ExtendedInterpreter::add_binding_diagnostic(std::string_view name, std::string message)
{
    m_binding_diagnostics.push_back({std::string{name},
        ExtendedInterpreterDiagnostic{
            ExtendedInterpreterDiagnosticKind::BINDING, {}, m_entry.name, std::move(message)}});
    rebuild_diagnostics();
}

void ExtendedInterpreter::clear_binding_diagnostics(std::string_view name)
{
    const std::string key{name};
    m_binding_diagnostics.erase(std::remove_if(m_binding_diagnostics.begin(), m_binding_diagnostics.end(),
                                    [&key](const auto &diagnostic) { return diagnostic.first == key; }),
        m_binding_diagnostics.end());
    rebuild_diagnostics();
}

void ExtendedInterpreter::rebuild_diagnostics()
{
    m_diagnostics = m_construction_diagnostics;
    for (const auto &diagnostic : m_binding_diagnostics)
    {
        m_diagnostics.push_back(diagnostic.second);
    }
}

bool PreparedParameterSet::ok() const
{
    return diagnostics.empty();
}

PreparedParameterSet prepare_parameter_interpreters(
    const parameter::ParameterReferenceSet &references, ExtendedInterpreterOptions options)
{
    PreparedParameterSet result;
    for (const parameter::ParameterReferenceDiagnostic &diagnostic : references.diagnostics)
    {
        result.diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::REFERENCE,
            diagnostic.location, {}, parameter_reference_message(diagnostic)});
    }
    if (!result.diagnostics.empty())
    {
        return result;
    }

    for (const parameter::ParameterResolvedReference &resolved : references.resolved)
    {
        ExtendedInterpreterOptions interpreter_options{options};
        interpreter_options.parser.entry_kind = resolved.entry_kind;
        if (interpreter_options.parser.source_filename.empty())
        {
            interpreter_options.parser.source_filename = resolved.reference.filename;
        }

        ExtendedInterpreter interpreter{resolved.file_entry, interpreter_options};
        for (const ExtendedInterpreterDiagnostic &diagnostic : interpreter.diagnostics())
        {
            result.diagnostics.push_back(diagnostic);
        }
        if (!interpreter.ok())
        {
            continue;
        }

        const RuntimeParameterMetadata metadata{
            resolved.ast ? collect_runtime_parameter_metadata(*resolved.ast) : RuntimeParameterMetadata{}};
        for (const parameter::Parameter &parameter : resolved.reference.parameters)
        {
            if (starts_with(parameter.key, "p_") && parameter.key.find('.') == std::string::npos)
            {
                const std::string name{
                    forwarded_runtime_parameter_name(metadata, std::string_view{parameter.key}.substr(2))};
                if (!bind_forwarded_runtime_parameter(interpreter, metadata, name, parameter))
                {
                    interpreter.set_parameter(name, Value{parameter.value});
                }
                continue;
            }
            if (starts_with(parameter.key, "f_"))
            {
                interpreter.set_function_parameter(std::string_view{parameter.key}.substr(2), parameter.value);
                continue;
            }
            if (starts_with(parameter.key, "p_"))
            {
                std::string_view path{parameter.key};
                path.remove_prefix(2);
                const std::size_t dot{path.find('.')};
                if (dot == std::string_view::npos)
                {
                    interpreter.set_plugin_parameter(path, parameter.value);
                    continue;
                }
                std::string_view nested_name{path.substr(dot + 1)};
                if (starts_with(nested_name, "p_") || starts_with(nested_name, "f_"))
                {
                    nested_name.remove_prefix(2);
                }
                interpreter.set_plugin_parameter_value(path.substr(0, dot), nested_name, Value{parameter.value});
            }
        }
        if (!interpreter.ok())
        {
            for (const ExtendedInterpreterDiagnostic &diagnostic : interpreter.diagnostics())
            {
                result.diagnostics.push_back(diagnostic);
            }
            continue;
        }

        result.formulas.push_back(PreparedParameterFormula{
            resolved.reference.site, resolved.reference.filename, resolved.reference.entry, std::move(interpreter)});
    }
    return result;
}

} // namespace formula
