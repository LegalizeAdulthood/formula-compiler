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
#include <cmath>
#include <cstdint>
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

Value compare_values(const Value &lhs, const Value &rhs, const std::string &op)
{
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

class ExpressionInterpreter : public ast::NullVisitor
{
public:
    ExpressionInterpreter(ExtendedRuntimeState &state, const FunctionMap &functions, std::size_t max_loop_iterations) :
        m_state(state),
        m_functions(functions),
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
        const ValueKind kind{value_kind_for_type(node.type())};
        Value value{node.initializer() ? convert_value(interpret(node.initializer()), kind) : default_value(kind)};
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
            unsupported_runtime_node("FunctionCallNode");
        }
        if (const auto builtin = call_builtin(node))
        {
            m_result = *builtin;
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

    void visit(const ast::MemberAccessNode &) override
    {
        unsupported_runtime_node("MemberAccessNode");
    }

    void visit(const ast::NewNode &) override
    {
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

    std::optional<Value> call_builtin(const ast::FunctionCallNode &node)
    {
        const std::string &name{node.name()};
        const std::vector<ast::Expr> &args{node.args()};
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
            if (!m_returning && !function.return_type().empty())
            {
                throw std::runtime_error("missing return: " + function.name());
            }
            if (!function.return_type().empty())
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
        throw std::runtime_error("invalid assignment target");
    }

    ExtendedRuntimeState &m_state;
    FunctionMap m_functions;
    std::size_t m_max_loop_iterations{};
    Value m_result;
    bool m_returning{};
};

class ParameterDefaultCollector : public ast::NullVisitor
{
public:
    ParameterDefaultCollector(
        ExtendedRuntimeState &state, const FunctionMap &functions, std::size_t max_loop_iterations) :
        m_state(state),
        m_functions(functions),
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
        collect(node.block());
        m_current_parameter = nullptr;
    }

    void visit(const ast::SettingNode &node) override
    {
        if (m_current_parameter == nullptr || node.key() != "default" ||
            m_state.has_parameter_value(m_current_parameter->name()))
        {
            return;
        }
        Value value;
        if (const auto *expr = std::get_if<ast::Expr>(&node.value()))
        {
            value = ExpressionInterpreter{m_state, m_functions, m_max_loop_iterations}.interpret(*expr);
        }
        else
        {
            value = value_from_setting(node.value());
        }
        m_state.set_parameter_value(
            m_current_parameter->name(), convert_value(value, value_kind_for_type(m_current_parameter->type())));
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
    std::size_t m_max_loop_iterations{};
    const ast::ParamBlockNode *m_current_parameter{};
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
    return m_diagnostics.empty();
}

void ExtendedInterpreter::set_value(std::string_view name, Value value)
{
    const semantic::BuiltinRegistry &builtins{builtins_or_default(m_options.builtins)};
    if (!name.empty() && name.front() == '@')
    {
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
    const Value result{ExpressionInterpreter{m_state, functions, m_options.max_loop_iterations}.interpret(
        section_expr(*m_ast, section))};
    return section_result(section, result);
}

void ExtendedInterpreter::parse()
{
    const parser::ParserPtr parser{parser::create_parser(m_entry.body, m_options.parser)};
    m_ast = parser->parse();
    add_parse_diagnostics(*parser);
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
}

void ExtendedInterpreter::analyze()
{
    if (!m_ast || !m_diagnostics.empty())
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
    add_semantic_diagnostics(semantic::analyze_formula(*m_ast, context));
    initialize_runtime_state();
}

void ExtendedInterpreter::initialize_runtime_state()
{
    if (!m_ast || !m_diagnostics.empty())
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
    ParameterDefaultCollector{m_state, functions, m_options.max_loop_iterations}.collect(m_ast->defaults);
}

void ExtendedInterpreter::add_parse_diagnostics(const parser::Parser &parser)
{
    for (const parser::Diagnostic &diagnostic : parser.get_errors())
    {
        m_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::PARSE,
            diagnostic.position, m_options.parser.source_filename, parser::to_string(diagnostic.code)});
    }
}

void ExtendedInterpreter::add_reference_diagnostics()
{
    for (const FormulaFileDiagnostic &diagnostic : m_files.diagnostics)
    {
        m_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::REFERENCE,
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
        m_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::SEMANTIC,
            diagnostic.location, diagnostic.entry_name, diagnostic.message});
    }
}

} // namespace formula
