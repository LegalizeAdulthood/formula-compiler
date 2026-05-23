// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/SemanticAnalyzer.h>

#include <formula/Visitor.h>

#include <algorithm>
#include <array>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace formula::semantic
{

namespace
{

struct FunctionSignature
{
    SemanticTypeKind return_type{SemanticTypeKind::VOID};
    std::vector<SemanticTypeKind> argument_types;
    std::vector<bool> by_ref_arguments;
};

SemanticType semantic_type(SemanticTypeKind kind, std::string name)
{
    SemanticType type;
    type.kind = kind;
    type.name = std::move(name);
    return type;
}

SemanticFunctionDescriptor unary_complex_function(std::string name, const SemanticType &complex_type)
{
    SemanticFunctionDescriptor function;
    function.name = std::move(name);
    function.return_type = complex_type;
    function.argument_types.push_back(complex_type);
    return function;
}

BuiltinRegistry make_default_builtin_registry()
{
    BuiltinRegistry registry;
    registry.types.push_back(semantic_type(SemanticTypeKind::VOID, "void"));
    registry.types.push_back(semantic_type(SemanticTypeKind::BOOL, "bool"));
    registry.types.push_back(semantic_type(SemanticTypeKind::INT, "int"));
    registry.types.push_back(semantic_type(SemanticTypeKind::FLOAT, "float"));
    registry.types.push_back(semantic_type(SemanticTypeKind::COMPLEX, "complex"));
    registry.types.push_back(semantic_type(SemanticTypeKind::COLOR, "color"));
    registry.types.push_back(semantic_type(SemanticTypeKind::STRING, "string"));
    registry.types.push_back(semantic_type(SemanticTypeKind::BUILTIN_OBJECT, "Image"));

    const SemanticType *complex_type{registry.find_type("complex")};
    if (complex_type)
    {
        static constexpr std::array<std::string_view, 37> functions{
            "sin", "cos", "sinh", "cosh", "cosxx",      //
            "tan", "cotan", "tanh", "cotanh", "sqr",    //
            "log", "exp", "abs", "conj", "real",        //
            "imag", "flip", "fn1", "fn2", "fn3",        //
            "fn4", "srand", "asin", "acos", "asinh",    //
            "acosh", "atan", "atanh", "sqrt", "cabs",   //
            "floor", "ceil", "trunc", "round", "ident", //
            "zero", "one",                              //
        };
        for (std::string_view name : functions)
        {
            registry.functions.push_back(unary_complex_function(std::string{name}, *complex_type));
        }
    }

    const SemanticType *image_type{registry.find_type("Image")};
    if (image_type)
    {
        SemanticClassDescriptor image;
        image.name = "Image";
        image.type = *image_type;
        image.builtin = true;
        registry.classes.push_back(std::move(image));
    }

    return registry;
}

} // namespace

const SemanticType *BuiltinRegistry::find_type(std::string_view name) const
{
    for (const SemanticType &type : types)
    {
        if (type.name == name)
        {
            return &type;
        }
    }
    return nullptr;
}

const SemanticFunctionDescriptor *BuiltinRegistry::find_function(std::string_view name) const
{
    for (const SemanticFunctionDescriptor &function : functions)
    {
        if (function.name == name)
        {
            return &function;
        }
    }
    return nullptr;
}

const SemanticClassDescriptor *BuiltinRegistry::find_class(std::string_view name) const
{
    for (const SemanticClassDescriptor &klass : classes)
    {
        if (klass.name == name)
        {
            return &klass;
        }
    }
    return nullptr;
}

const BuiltinRegistry &default_builtin_registry()
{
    static const BuiltinRegistry registry{make_default_builtin_registry()};
    return registry;
}

class FormulaSymbolCollector : public ast::NullVisitor
{
public:
    FormulaSymbolCollector(const BuiltinRegistry &builtins, const FormulaSemanticContext &context) :
        m_builtins(builtins)
    {
        for (const RetainedFormulaClass *klass : context.retained_classes)
        {
            if (klass)
            {
                m_class_names.insert(klass->reference.class_name);
            }
        }
    }

    std::vector<SemanticDiagnostic> collect(const ast::FormulaSections &formula)
    {
        predeclare_functions(formula.per_image);
        predeclare_functions(formula.builtin);
        predeclare_functions(formula.initialize);
        predeclare_functions(formula.iterate);
        predeclare_functions(formula.bailout);
        predeclare_functions(formula.perturb_initialize);
        predeclare_functions(formula.perturb_iterate);
        predeclare_functions(formula.defaults);
        predeclare_functions(formula.type_switch);
        predeclare_functions(formula.final);
        predeclare_functions(formula.transform);
        predeclare_functions(formula.public_members);
        predeclare_functions(formula.protected_members);
        predeclare_functions(formula.private_members);

        collect(formula.per_image);
        collect(formula.builtin);
        collect(formula.initialize);
        collect(formula.iterate);
        collect(formula.bailout);
        collect(formula.perturb_initialize);
        collect(formula.perturb_iterate);
        collect(formula.defaults);
        collect(formula.type_switch);
        collect(formula.final);
        collect(formula.transform);
        collect(formula.public_members);
        collect(formula.protected_members);
        collect(formula.private_members);
        return m_diagnostics;
    }

    void visit(const ast::AssignmentNode &node) override
    {
        if (!node.variable().empty() && is_read_only_symbol(node.variable()))
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_ASSIGNMENT_TARGET;
            diagnostic.message = "invalid assignment target: " + node.variable() + " is const";
            m_diagnostics.push_back(std::move(diagnostic));
        }
        const SemanticTypeKind target_type{expression_type(node.target())};
        const SemanticTypeKind value_type{expression_type(node.expression())};
        if (!can_convert(value_type, target_type))
        {
            report_invalid_conversion(value_type, target_type);
        }
    }

    void visit(const ast::BinaryOpNode &node) override
    {
        collect(node.left());
        collect(node.right());
    }

    void visit(const ast::DeclarationNode &node) override
    {
        validate_type(node.type());
        declare(node.name(), SemanticSymbolKind::LOCAL, type_kind(node.type()));
        for (const ast::Expr &dimension : node.dimensions())
        {
            validate_index_type(expression_type(dimension));
        }
        if (node.initializer())
        {
            const SemanticTypeKind value_type{expression_type(node.initializer())};
            const SemanticTypeKind target_type{type_kind(node.type())};
            if (!can_convert(value_type, target_type))
            {
                report_invalid_conversion(value_type, target_type);
            }
        }
    }

    void visit(const ast::FunctionDeclNode &node) override
    {
        if (!node.return_type().empty())
        {
            validate_type(node.return_type());
        }
        if (m_predeclared_functions.find(&node) == m_predeclared_functions.end())
        {
            declare_function(node);
        }
        begin_scope();
        m_function_return_types.push_back(
            node.return_type().empty() ? SemanticTypeKind::VOID : type_kind(node.return_type()));
        for (const ast::FunctionArgument &arg : node.args())
        {
            validate_type(arg.type);
            declare(arg.name, SemanticSymbolKind::FUNCTION_PARAMETER, type_kind(arg.type), arg.is_const);
        }
        collect(node.body());
        m_function_return_types.pop_back();
        end_scope();
    }

    void visit(const ast::FunctionCallNode &node) override
    {
        std::size_t checked_args{};
        if (const SemanticFunctionDescriptor *function = m_builtins.find_function(node.name()))
        {
            validate_call_arity(node.name(), node.args().size(), function->argument_types.size());
            checked_args = validate_builtin_call_arguments(node, *function);
        }
        else if (const auto function = m_functions.find(node.name()); function != m_functions.end())
        {
            validate_call_arity(node.name(), node.args().size(), function->second.argument_types.size());
            checked_args = validate_user_call_arguments(node, function->second);
        }
        else if (!is_declared(node.name()))
        {
            report_unknown_symbol(node.name());
        }
        for (std::size_t index = checked_args; index < node.args().size(); ++index)
        {
            collect(node.args()[index]);
        }
    }

    void visit(const ast::ConstantRefNode &node) override
    {
        report_unknown_symbol(node.name());
    }

    void visit(const ast::IdentifierNode &node) override
    {
        if (!is_declared(node.name()) && !is_builtin_variable(node.name()))
        {
            report_unknown_symbol(node.name());
        }
    }

    void visit(const ast::IfStatementNode &node) override
    {
        validate_condition_type(expression_type(node.condition()));
        collect_block(node.has_then_block() ? node.then_block() : ast::Expr{});
        collect_block(node.has_else_block() ? node.else_block() : ast::Expr{});
    }

    void visit(const ast::IndexNode &node) override
    {
        collect(node.target());
        for (const ast::Expr &index : node.indices())
        {
            validate_index_type(expression_type(index));
        }
    }

    void visit(const ast::MemberAccessNode &node) override
    {
        collect(node.target());
    }

    void visit(const ast::NewNode &node) override
    {
        validate_new_type(node.type());
        for (const ast::Expr &arg : node.args())
        {
            collect(arg);
        }
    }

    void visit(const ast::ParameterRefNode &node) override
    {
        report_unknown_symbol(node.name());
    }

    void visit(const ast::RepeatUntilNode &node) override
    {
        collect_block(node.body());
        validate_condition_type(expression_type(node.condition()));
    }

    void visit(const ast::ReturnNode &node) override
    {
        if (m_function_return_types.empty())
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_RETURN;
            diagnostic.message = "invalid return outside function";
            m_diagnostics.push_back(std::move(diagnostic));
            collect(node.expression());
            return;
        }
        const SemanticTypeKind expected_type{m_function_return_types.back()};
        const SemanticTypeKind actual_type{
            node.expression() ? expression_type(node.expression()) : SemanticTypeKind::VOID};
        if (!can_convert(actual_type, expected_type))
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_RETURN;
            diagnostic.message = "invalid return: " + type_name(actual_type) + " to " + type_name(expected_type);
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        for (const ast::Expr &statement : node.statements())
        {
            collect(statement);
        }
    }

    void visit(const ast::UnaryOpNode &node) override
    {
        collect(node.operand());
    }

    void visit(const ast::WhileNode &node) override
    {
        validate_condition_type(expression_type(node.condition()));
        collect_block(node.body());
    }

private:
    void begin_scope()
    {
        m_scopes.emplace_back();
        m_symbol_types.emplace_back();
        m_read_only_symbols.emplace_back();
    }

    void end_scope()
    {
        m_scopes.pop_back();
        m_symbol_types.pop_back();
        m_read_only_symbols.pop_back();
    }

    void collect(const ast::Expr &expr)
    {
        if (expr)
        {
            expr->visit(*this);
        }
    }

    void collect_block(const ast::Expr &expr)
    {
        begin_scope();
        collect(expr);
        end_scope();
    }

    void predeclare_functions(const ast::Expr &expr)
    {
        if (const auto *function = dynamic_cast<const ast::FunctionDeclNode *>(expr.get()))
        {
            declare_function(*function);
            m_predeclared_functions.insert(function);
        }
        else if (const auto *sequence = dynamic_cast<const ast::StatementSeqNode *>(expr.get()))
        {
            for (const ast::Expr &statement : sequence->statements())
            {
                predeclare_functions(statement);
            }
        }
    }

    void declare(const std::string &name, SemanticSymbolKind, SemanticTypeKind type = SemanticTypeKind::ERROR,
        bool is_read_only = false)
    {
        if (m_scopes.empty())
        {
            begin_scope();
        }
        if (!m_scopes.back().insert(name).second)
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::DUPLICATE_SYMBOL;
            diagnostic.message = "duplicate symbol: " + name;
            m_diagnostics.push_back(std::move(diagnostic));
        }
        m_symbol_types.back().emplace(name, type);
        if (is_read_only)
        {
            m_read_only_symbols.back().insert(name);
        }
    }

    void validate_type(const std::string &name)
    {
        if (!m_builtins.find_type(name) && !m_builtins.find_class(name) && !is_class(name))
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::UNKNOWN_TYPE;
            diagnostic.message = "unknown type: " + name;
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    void validate_new_type(const std::string &name)
    {
        if (!m_builtins.find_class(name) && !is_class(name))
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = m_builtins.find_type(name) ? SemanticDiagnosticCode::INVALID_BUILTIN_USAGE
                                                         : SemanticDiagnosticCode::UNKNOWN_TYPE;
            diagnostic.message = "invalid new type: " + name;
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    void validate_call_arity(const std::string &name, std::size_t actual, std::size_t expected)
    {
        if (actual != expected)
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_CALL_ARITY;
            diagnostic.message = "invalid call arity: " + name + " expects " + std::to_string(expected) + " argument";
            if (expected != 1U)
            {
                diagnostic.message += "s";
            }
            diagnostic.message += ", got " + std::to_string(actual);
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    std::size_t validate_builtin_call_arguments(
        const ast::FunctionCallNode &node, const SemanticFunctionDescriptor &function)
    {
        const std::size_t count{std::min(node.args().size(), function.argument_types.size())};
        for (std::size_t index = 0; index < count; ++index)
        {
            validate_argument_type(
                node.name(), expression_type(node.args()[index]), function.argument_types[index].kind);
        }
        return count;
    }

    std::size_t validate_user_call_arguments(const ast::FunctionCallNode &node, const FunctionSignature &function)
    {
        const std::size_t count{std::min(node.args().size(), function.argument_types.size())};
        for (std::size_t index = 0; index < count; ++index)
        {
            validate_argument_type(node.name(), expression_type(node.args()[index]), function.argument_types[index]);
            if (function.by_ref_arguments[index] && !is_assignment_target(node.args()[index]))
            {
                SemanticDiagnostic diagnostic;
                diagnostic.code = SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE;
                diagnostic.message = "invalid by-ref argument: " + node.name();
                m_diagnostics.push_back(std::move(diagnostic));
            }
        }
        return count;
    }

    void validate_argument_type(const std::string &name, SemanticTypeKind actual, SemanticTypeKind expected)
    {
        if (!can_convert(actual, expected))
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE;
            diagnostic.message =
                "invalid argument type: " + name + " got " + type_name(actual) + ", expected " + type_name(expected);
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    void declare_function(const ast::FunctionDeclNode &node)
    {
        declare(node.name(), SemanticSymbolKind::USER_FUNCTION, SemanticTypeKind::FUNCTION);
        FunctionSignature signature;
        signature.return_type = node.return_type().empty() ? SemanticTypeKind::VOID : type_kind(node.return_type());
        for (const ast::FunctionArgument &arg : node.args())
        {
            signature.argument_types.push_back(type_kind(arg.type));
            signature.by_ref_arguments.push_back(arg.is_by_ref);
        }
        m_functions.emplace(node.name(), std::move(signature));
    }

    SemanticTypeKind expression_type(const ast::Expr &expr)
    {
        if (!expr)
        {
            return SemanticTypeKind::VOID;
        }
        if (const auto *literal = dynamic_cast<const ast::LiteralNode *>(expr.get()))
        {
            switch (literal->value().index())
            {
            case 0:
                return SemanticTypeKind::INT;
            case 1:
                return SemanticTypeKind::FLOAT;
            case 2:
                return SemanticTypeKind::COMPLEX;
            case 3:
                return SemanticTypeKind::BOOL;
            case 4:
                return SemanticTypeKind::STRING;
            case 5:
                return SemanticTypeKind::COLOR;
            default:
                return SemanticTypeKind::ERROR;
            }
        }
        if (const auto *identifier = dynamic_cast<const ast::IdentifierNode *>(expr.get()))
        {
            if (!is_declared(identifier->name()) && !is_builtin_variable(identifier->name()))
            {
                report_unknown_symbol(identifier->name());
                return SemanticTypeKind::ERROR;
            }
            return symbol_type(identifier->name());
        }
        if (const auto *call = dynamic_cast<const ast::FunctionCallNode *>(expr.get()))
        {
            visit(*call);
            if (const SemanticFunctionDescriptor *function = m_builtins.find_function(call->name()))
            {
                return function->return_type.kind;
            }
            if (const auto function = m_functions.find(call->name()); function != m_functions.end())
            {
                return function->second.return_type;
            }
            return SemanticTypeKind::ERROR;
        }
        if (const auto *new_object = dynamic_cast<const ast::NewNode *>(expr.get()))
        {
            visit(*new_object);
            if (m_builtins.find_class(new_object->type()))
            {
                return SemanticTypeKind::BUILTIN_OBJECT;
            }
            return is_class(new_object->type()) ? SemanticTypeKind::CLASS_OBJECT : SemanticTypeKind::ERROR;
        }
        collect(expr);
        return SemanticTypeKind::ERROR;
    }

    SemanticTypeKind symbol_type(const std::string &name) const
    {
        for (auto scope = m_symbol_types.rbegin(); scope != m_symbol_types.rend(); ++scope)
        {
            const auto found = scope->find(name);
            if (found != scope->end())
            {
                return found->second;
            }
        }
        return is_builtin_variable(name) ? SemanticTypeKind::COMPLEX : SemanticTypeKind::ERROR;
    }

    SemanticTypeKind type_kind(const std::string &name) const
    {
        if (const SemanticType *type = m_builtins.find_type(name))
        {
            return type->kind;
        }
        if (m_builtins.find_class(name) || is_class(name))
        {
            return SemanticTypeKind::CLASS_OBJECT;
        }
        return SemanticTypeKind::ERROR;
    }

    bool can_convert(SemanticTypeKind from, SemanticTypeKind to) const
    {
        if (from == SemanticTypeKind::ERROR || to == SemanticTypeKind::ERROR || from == to)
        {
            return true;
        }
        return conversion_rank(from) >= 0 && conversion_rank(to) >= 0 && conversion_rank(from) <= conversion_rank(to);
    }

    int conversion_rank(SemanticTypeKind type) const
    {
        switch (type)
        {
        case SemanticTypeKind::BOOL:
            return 0;
        case SemanticTypeKind::INT:
            return 1;
        case SemanticTypeKind::FLOAT:
            return 2;
        case SemanticTypeKind::COMPLEX:
            return 3;
        default:
            return -1;
        }
    }

    std::string type_name(SemanticTypeKind type) const
    {
        switch (type)
        {
        case SemanticTypeKind::BOOL:
            return "bool";
        case SemanticTypeKind::INT:
            return "int";
        case SemanticTypeKind::FLOAT:
            return "float";
        case SemanticTypeKind::COMPLEX:
            return "complex";
        case SemanticTypeKind::COLOR:
            return "color";
        case SemanticTypeKind::STRING:
            return "string";
        case SemanticTypeKind::BUILTIN_OBJECT:
            return "builtin object";
        case SemanticTypeKind::CLASS_OBJECT:
            return "class object";
        case SemanticTypeKind::FUNCTION:
            return "function";
        case SemanticTypeKind::VOID:
            return "void";
        case SemanticTypeKind::ARRAY:
            return "array";
        case SemanticTypeKind::ERROR:
            return "error";
        }
        return "error";
    }

    void report_invalid_conversion(SemanticTypeKind from, SemanticTypeKind to)
    {
        SemanticDiagnostic diagnostic;
        diagnostic.code = SemanticDiagnosticCode::INVALID_TYPE_CONVERSION;
        diagnostic.message = "invalid conversion: " + type_name(from) + " to " + type_name(to);
        m_diagnostics.push_back(std::move(diagnostic));
    }

    bool is_assignment_target(const ast::Expr &expr) const
    {
        return dynamic_cast<const ast::IdentifierNode *>(expr.get()) != nullptr ||
            dynamic_cast<const ast::IndexNode *>(expr.get()) != nullptr ||
            dynamic_cast<const ast::MemberAccessNode *>(expr.get()) != nullptr;
    }

    void validate_condition_type(SemanticTypeKind type)
    {
        if (type != SemanticTypeKind::ERROR && conversion_rank(type) < 0)
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE;
            diagnostic.message = "invalid condition type: " + type_name(type);
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    void validate_index_type(SemanticTypeKind type)
    {
        if (!can_convert(type, SemanticTypeKind::INT))
        {
            SemanticDiagnostic diagnostic;
            diagnostic.code = SemanticDiagnosticCode::INVALID_ARRAY_ACCESS;
            diagnostic.message = "invalid array index type: " + type_name(type);
            m_diagnostics.push_back(std::move(diagnostic));
        }
    }

    bool is_declared(const std::string &name) const
    {
        for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); ++scope)
        {
            if (scope->find(name) != scope->end())
            {
                return true;
            }
        }
        return false;
    }

    bool is_class(const std::string &name) const
    {
        return m_class_names.find(name) != m_class_names.end();
    }

    bool is_read_only_symbol(const std::string &name) const
    {
        for (auto scope = m_read_only_symbols.rbegin(); scope != m_read_only_symbols.rend(); ++scope)
        {
            if (scope->find(name) != scope->end())
            {
                return true;
            }
        }
        return false;
    }

    bool is_builtin_variable(const std::string &name) const
    {
        static constexpr std::array<std::string_view, 19> variables{
            "z",
            "p1",
            "p2",
            "p3",
            "p4",
            "p5",
            "pixel",
            "lastsqr",
            "rand",
            "pi",
            "e",
            "maxit",
            "scrnmax",
            "scrnpix",
            "whitesq",
            "ismand",
            "center",
            "magxmag",
            "rotskew",
        };
        for (std::string_view variable : variables)
        {
            if (name == variable)
            {
                return true;
            }
        }
        return false;
    }

    void report_unknown_symbol(const std::string &name)
    {
        SemanticDiagnostic diagnostic;
        diagnostic.code = SemanticDiagnosticCode::UNKNOWN_SYMBOL;
        diagnostic.message = "unknown symbol: " + name;
        m_diagnostics.push_back(std::move(diagnostic));
    }

    const BuiltinRegistry &m_builtins;
    std::vector<std::unordered_set<std::string>> m_scopes{{}};
    std::vector<std::unordered_map<std::string, SemanticTypeKind>> m_symbol_types{{}};
    std::vector<std::unordered_set<std::string>> m_read_only_symbols{{}};
    std::unordered_map<std::string, FunctionSignature> m_functions;
    std::unordered_set<std::string> m_class_names;
    std::unordered_set<const ast::FunctionDeclNode *> m_predeclared_functions;
    std::vector<SemanticTypeKind> m_function_return_types;
    std::vector<SemanticDiagnostic> m_diagnostics;
};

std::vector<SemanticDiagnostic> analyze_formula(
    const ast::FormulaSections &formula, const FormulaSemanticContext &context)
{
    const BuiltinRegistry &builtins{context.builtins ? *context.builtins : default_builtin_registry()};
    return FormulaSymbolCollector{builtins, context}.collect(formula);
}

std::vector<SemanticDiagnostic> analyze_parameter_set(const parameter::ExtendedParameterEntry &,
    const parameter::ParameterReferenceSet &, const ParameterSetSemanticContext &)
{
    return {};
}

} // namespace formula::semantic
