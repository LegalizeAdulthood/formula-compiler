// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/SemanticAnalyzer.h>

#include <formula/Visitor.h>

#include <array>
#include <string_view>
#include <unordered_set>

namespace formula::semantic
{

namespace
{

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
    std::vector<SemanticDiagnostic> collect(const ast::FormulaSections &formula)
    {
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
        collect(node.target());
        collect(node.expression());
    }

    void visit(const ast::BinaryOpNode &node) override
    {
        collect(node.left());
        collect(node.right());
    }

    void visit(const ast::DeclarationNode &node) override
    {
        declare(node.name(), SemanticSymbolKind::LOCAL);
        for (const ast::Expr &dimension : node.dimensions())
        {
            collect(dimension);
        }
        collect(node.initializer());
    }

    void visit(const ast::FunctionDeclNode &node) override
    {
        declare(node.name(), SemanticSymbolKind::USER_FUNCTION);
        begin_scope();
        for (const ast::FunctionArgument &arg : node.args())
        {
            declare(arg.name, SemanticSymbolKind::FUNCTION_PARAMETER);
        }
        collect(node.body());
        end_scope();
    }

    void visit(const ast::FunctionCallNode &node) override
    {
        for (const ast::Expr &arg : node.args())
        {
            collect(arg);
        }
    }

    void visit(const ast::IfStatementNode &node) override
    {
        collect(node.condition());
        collect_block(node.has_then_block() ? node.then_block() : ast::Expr{});
        collect_block(node.has_else_block() ? node.else_block() : ast::Expr{});
    }

    void visit(const ast::IndexNode &node) override
    {
        collect(node.target());
        for (const ast::Expr &index : node.indices())
        {
            collect(index);
        }
    }

    void visit(const ast::MemberAccessNode &node) override
    {
        collect(node.target());
    }

    void visit(const ast::NewNode &node) override
    {
        for (const ast::Expr &arg : node.args())
        {
            collect(arg);
        }
    }

    void visit(const ast::RepeatUntilNode &node) override
    {
        collect_block(node.body());
        collect(node.condition());
    }

    void visit(const ast::ReturnNode &node) override
    {
        collect(node.expression());
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
        collect(node.condition());
        collect_block(node.body());
    }

private:
    void begin_scope()
    {
        m_scopes.emplace_back();
    }

    void end_scope()
    {
        m_scopes.pop_back();
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

    void declare(const std::string &name, SemanticSymbolKind)
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
    }

    std::vector<std::unordered_set<std::string>> m_scopes{{}};
    std::vector<SemanticDiagnostic> m_diagnostics;
};

std::vector<SemanticDiagnostic> analyze_formula(const ast::FormulaSections &formula, const FormulaSemanticContext &)
{
    return FormulaSymbolCollector{}.collect(formula);
}

std::vector<SemanticDiagnostic> analyze_parameter_set(const parameter::ExtendedParameterEntry &,
    const parameter::ParameterReferenceSet &, const ParameterSetSemanticContext &)
{
    return {};
}

} // namespace formula::semantic
