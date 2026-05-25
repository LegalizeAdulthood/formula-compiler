// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/interpreter/Interpreter.h>

#include <formula/core/Visitor.h>

#include <formula/core/functions.h>
#include <formula/core/Node.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace formula::ast
{

namespace
{

class Interpreter : public NullVisitor
{
public:
    explicit Interpreter(Dictionary symbols, std::map<std::string, std::string> functions, std::mt19937 *random) :
        m_symbols(std::move(symbols)),
        m_functions(std::move(functions)),
        m_random(random)
    {
    }
    Interpreter(const Interpreter &rhs) = delete;
    Interpreter(Interpreter &&) = delete;
    ~Interpreter() override = default;
    Interpreter &operator=(const Interpreter &rhs) = delete;
    Interpreter &operator=(Interpreter &&) = delete;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const ConstantRefNode &node) override;
    void visit(const DeclarationNode &node) override;
    void visit(const FunctionDeclNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const IndexNode &node) override;
    void visit(const LiteralNode &node) override;
    void visit(const MemberAccessNode &node) override;
    void visit(const NewNode &node) override;
    void visit(const ParameterRefNode &node) override;
    void visit(const RepeatUntilNode &node) override;
    void visit(const ReturnNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;
    void visit(const WhileNode &node) override;

    const Complex &result() const
    {
        return m_result.back();
    }

    const Dictionary &symbols() const
    {
        return m_symbols;
    }

private:
    Complex &back()
    {
        return m_result.back();
    }
    Complex pop()
    {
        Complex val = result();
        m_result.pop_back();
        return val;
    }

    std::vector<Complex> m_result{1};
    Dictionary m_symbols;
    std::map<std::string, std::string> m_functions;
    std::mt19937 *m_random{};
};

void unsupported_node(std::string_view name)
{
    throw std::runtime_error(std::string{name} + " is not supported by the interpreter");
}

void Interpreter::visit(const AssignmentNode &node)
{
    node.expression()->visit(*this);
    m_symbols[node.variable()] = result();
}

void Interpreter::visit(const BinaryOpNode &node)
{
    node.left()->visit(*this);
    const std::string &op{node.op()};
    const auto bool_result = [](bool condition)
    {
        return Complex{condition ? 1.0 : 0.0, 0.0};
    };
    if (op == "&&")
    {
        const bool left{result().re != 0.0};
        node.right()->visit(*this);
        back() = bool_result(left && result().re != 0.0);
        return;
    }
    if (op == "||")
    {
        const bool left{result().re != 0.0};
        node.right()->visit(*this);
        back() = bool_result(left || result().re != 0.0);
        return;
    }

    m_result.push_back(Complex{});
    node.right()->visit(*this);
    const Complex right{pop()};
    const Complex &left{back()};
    if (op == "+")
    {
        back() += right;
    }
    else if (op == "-")
    {
        back() -= right;
    }
    else if (op == "*")
    {
        back() *= right;
    }
    else if (op == "/")
    {
        back() /= right;
    }
    else if (op == "^")
    {
        back() = pow(left, right);
    }
    else if (op == "<")
    {
        back() = bool_result(left.re < right.re);
    }
    else if (op == "<=")
    {
        back() = bool_result(left.re <= right.re);
    }
    else if (op == ">")
    {
        back() = bool_result(left.re > right.re);
    }
    else if (op == ">=")
    {
        back() = bool_result(left.re >= right.re);
    }
    else if (op == "==")
    {
        back() = bool_result(left == right);
    }
    else if (op == "!=")
    {
        back() = bool_result(left != right);
    }
    else
    {
        throw std::runtime_error(std::string{"Invalid binary operator '"} + op + "'");
    }
}

void Interpreter::visit(const ConstantRefNode &)
{
    unsupported_node("ConstantRefNode");
}

void Interpreter::visit(const DeclarationNode &)
{
    unsupported_node("DeclarationNode");
}

void Interpreter::visit(const FunctionDeclNode &)
{
    unsupported_node("FunctionDeclNode");
}

void Interpreter::visit(const FunctionCallNode &node)
{
    node.arg()->visit(*this);
    const std::string name{select_function(node.name(), m_functions)};
    if (name == "srand")
    {
        if (m_random != nullptr)
        {
            m_random->seed(static_cast<std::mt19937::result_type>(back().re));
        }
        back() = {};
        return;
    }
    if (name == "sqr")
    {
        const Complex arg{back()};
        m_symbols["lastsqr"] = {arg.re * arg.re + arg.im * arg.im, 0.0};
        back() = evaluate(name, arg);
        return;
    }
    back() = evaluate(name, back());
}

void Interpreter::visit(const IdentifierNode &node)
{
    if (const auto it = m_symbols.find(node.name()); it != m_symbols.end())
    {
        m_result.back() = it->second;
    }
    else
    {
        back() = Complex{};
    }
}

void Interpreter::visit(const IfStatementNode &node)
{
    node.condition()->visit(*this);
    if (result().re != 0.0) // Only check real part
    {
        if (node.has_then_block())
        {
            node.then_block()->visit(*this);
        }
        else
        {
            back() = Complex{1.0, 0.0};
        }
        return;
    }

    if (node.has_else_block())
    {
        node.else_block()->visit(*this);
    }
    else
    {
        back() = Complex{};
    }
}

void Interpreter::visit(const IndexNode &)
{
    unsupported_node("IndexNode");
}

void Interpreter::visit(const LiteralNode &node)
{
    switch (node.value().index())
    {
    case 0:
        back().re = std::get<int>(node.value());
        back().im = 0.0;
        break;

    case 1:
        back() = {std::get<double>(node.value()), 0.0};
        break;

    case 2:
        back() = std::get<Complex>(node.value());
        break;

    case 3:
        back() = {std::get<bool>(node.value()) ? 1.0 : 0.0, 0.0};
        break;

    default:
        throw std::runtime_error("Unknown LiteralNode variant index " + std::to_string(node.value().index()));
    }
}

void Interpreter::visit(const MemberAccessNode &)
{
    unsupported_node("MemberAccessNode");
}

void Interpreter::visit(const NewNode &)
{
    unsupported_node("NewNode");
}

void Interpreter::visit(const ParameterRefNode &)
{
    unsupported_node("ParameterRefNode");
}

void Interpreter::visit(const RepeatUntilNode &)
{
    unsupported_node("RepeatUntilNode");
}

void Interpreter::visit(const ReturnNode &)
{
    unsupported_node("ReturnNode");
}

void Interpreter::visit(const StatementSeqNode &node)
{
    for (const Expr &st : node.statements())
    {
        st->visit(*this);
    }
}

void Interpreter::visit(const UnaryOpNode &node)
{
    node.operand()->visit(*this);
    if (node.op() == '-')
    {
        back().re = -back().re;
        back().im = -back().im;
    }
    else if (node.op() == '|')
    {
        const double re = back().re;
        const double im = back().im;
        back() = {re * re + im * im, 0.0};
    }
    // nothing to be done for unary + since it doesn't change the value
}

void Interpreter::visit(const WhileNode &)
{
    unsupported_node("WhileNode");
}

} // namespace

Complex interpret(const std::shared_ptr<Node> &expr, Dictionary &symbols)
{
    return interpret(expr, symbols, {});
}

Complex interpret(
    const std::shared_ptr<Node> &expr, Dictionary &symbols, const std::map<std::string, std::string> &functions)
{
    return interpret(expr, symbols, functions, nullptr);
}

Complex interpret(const std::shared_ptr<Node> &expr, Dictionary &symbols,
    const std::map<std::string, std::string> &functions, std::mt19937 *random)
{
    Interpreter interp(symbols, functions, random);
    expr->visit(interp);
    symbols = interp.symbols();
    return interp.result();
}

} // namespace formula::ast
