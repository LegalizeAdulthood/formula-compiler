// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Interpreter.h>

#include <formula/Visitor.h>

#include "functions.h"
#include <formula/Node.h>

#include <map>
#include <string>
#include <vector>

namespace formula::ast
{

namespace
{

class Interpreter : public NullVisitor
{
public:
    explicit Interpreter(Dictionary symbols) :
        m_symbols(std::move(symbols))
    {
    }
    Interpreter(const Interpreter &rhs) = delete;
    Interpreter(Interpreter &&) = delete;
    ~Interpreter() override = default;
    Interpreter &operator=(const Interpreter &rhs) = delete;
    Interpreter &operator=(Interpreter &&) = delete;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const LiteralNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

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
};

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
    if (op == "&&") // short-circuit AND - only check real part
    {
        if (result().re == 0.0)
        {
            back() = {0.0, 0.0};
            return;
        }
        node.right()->visit(*this);
        back() = bool_result(result().re != 0.0);
        return;
    }
    if (op == "||") // short-circuit OR - only check real part
    {
        if (result().re != 0.0)
        {
            back() = {1.0, 0.0};
            return;
        }
        back() = {};
        node.right()->visit(*this);
        back() = bool_result(result().re != 0.0);
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

void Interpreter::visit(const FunctionCallNode &node)
{
    node.arg()->visit(*this);
    back() = evaluate(node.name(), back());
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

    default:
        throw std::runtime_error("Unknown LiteralNode variant index " + std::to_string(node.value().index()));
    }
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

} // namespace

Complex interpret(const std::shared_ptr<Node> &expr, Dictionary &symbols)
{
    Interpreter interp(symbols);
    expr->visit(interp);
    symbols = interp.symbols();
    return interp.result();
}

} // namespace formula::ast
