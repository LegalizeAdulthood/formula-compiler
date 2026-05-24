// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedInterpreter.h>

#include <formula/Node.h>
#include <formula/Parser.h>
#include <formula/ReferenceCollector.h>
#include <formula/Visitor.h>

#include <cmath>
#include <stdexcept>
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

class ExpressionInterpreter : public ast::NullVisitor
{
public:
    explicit ExpressionInterpreter(ExtendedRuntimeState &state) :
        m_state(state)
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
        lvalue(node.target()).set(value);
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

    void visit(const ast::DeclarationNode &) override
    {
        unsupported_runtime_node("DeclarationNode");
    }

    void visit(const ast::FunctionBlockNode &) override
    {
        unsupported_runtime_node("FunctionBlockNode");
    }

    void visit(const ast::FunctionDeclNode &) override
    {
        unsupported_runtime_node("FunctionDeclNode");
    }

    void visit(const ast::FunctionCallNode &) override
    {
        unsupported_runtime_node("FunctionCallNode");
    }

    void visit(const ast::HeadingBlockNode &) override
    {
        unsupported_runtime_node("HeadingBlockNode");
    }

    void visit(const ast::IdentifierNode &node) override
    {
        m_result = m_state.value(node.name());
    }

    void visit(const ast::IfStatementNode &) override
    {
        unsupported_runtime_node("IfStatementNode");
    }

    void visit(const ast::IndexNode &) override
    {
        unsupported_runtime_node("IndexNode");
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

    void visit(const ast::RepeatUntilNode &) override
    {
        unsupported_runtime_node("RepeatUntilNode");
    }

    void visit(const ast::ReturnNode &) override
    {
        unsupported_runtime_node("ReturnNode");
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

    void visit(const ast::WhileNode &) override
    {
        unsupported_runtime_node("WhileNode");
    }

private:
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
        throw std::runtime_error("invalid assignment target");
    }

    ExtendedRuntimeState &m_state;
    Value m_result;
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
    if (!name.empty() && name.front() == '@')
    {
        m_state.set_parameter_value(name, std::move(value));
        return;
    }
    if (!name.empty() && name.front() == '#')
    {
        m_state.set_predefined_value(name, std::move(value), true);
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
    return ExpressionInterpreter{m_state}.interpret(section_expr(*m_ast, section));
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
