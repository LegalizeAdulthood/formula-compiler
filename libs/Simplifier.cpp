// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include "functions.h"
#include <formula/Visitor.h>

#include <cassert>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace formula::ast;

namespace formula
{

namespace
{

enum class NodeType
{
    NONE = 0,
    ASSIGNMENT,
    BINARY_OP,
    FUNCTION_CALL,
    IDENTIFIER,
    IF_STATEMENT,
    NUMBER,
    STATEMENT_SEQ,
    UNARY_OP
};

class Typer : public Visitor
{
public:
    Typer() = default;
    ~Typer() override = default;

    NodeType result() const
    {
        return m_type;
    }

    // clang-format off
    void visit(const AssignmentNode &node) override     { m_type = NodeType::ASSIGNMENT; }
    void visit(const BinaryOpNode &node) override       { m_type = NodeType::BINARY_OP; }
    void visit(const FunctionCallNode &node) override   { m_type = NodeType::FUNCTION_CALL; }
    void visit(const IdentifierNode &node) override     { m_type = NodeType::IDENTIFIER; }
    void visit(const IfStatementNode &node) override    { m_type = NodeType::IF_STATEMENT; }
    void visit(const NumberNode &node) override         { m_type = NodeType::NUMBER; }
    void visit(const StatementSeqNode &node) override   { m_type = NodeType::STATEMENT_SEQ; }
    void visit(const UnaryOpNode &node) override        { m_type = NodeType::UNARY_OP; }
    // clang-format on

private:
    NodeType m_type{};
};

NodeType get_node_type(Expr node)
{
    Typer v;
    node->visit(v);
    return v.result();
}

class Simplifier : public Visitor
{
public:
    Simplifier() = default;
    ~Simplifier() override = default;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const NumberNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

    Expr result() const
    {
        return m_result.back();
    }

private:
    std::vector<Expr> m_result;
};

void Simplifier::visit(const AssignmentNode &node)
{
    throw std::runtime_error("AssignmentNode not implemented");
}

void Simplifier::visit(const BinaryOpNode &node)
{
    node.left()->visit(*this);
    Expr lhs{m_result.back()};
    m_result.pop_back();
    auto left = dynamic_cast<NumberNode *>(lhs.get());
    if (left)
    {
        // short-circuit and
        if (node.op() == "&&" && left->value() == 0.0)
        {
            m_result.push_back(std::make_shared<NumberNode>(0.0));
            return;
        }
        // short-circuit or
        if (node.op() == "||" && left->value() != 0.0)
        {
            m_result.push_back(std::make_shared<NumberNode>(1.0));
            return;
        }
    }

    node.right()->visit(*this);
    Expr rhs{m_result.back()};
    m_result.pop_back();
    if (auto right = dynamic_cast<NumberNode *>(rhs.get()))
    {
        const double left_value = left->value();
        const double right_value = right->value();
        const std::string &op{node.op()};
        if (op == "+")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value + right_value));
            return;
        }
        if (op == "-")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value - right_value));
            return;
        }
        if (op == "*")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value * right_value));
            return;
        }
        if (op == "/")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value / right_value));
            return;
        }
        if (op == "^")
        {
            m_result.push_back(std::make_shared<NumberNode>(std::pow(left_value, right_value)));
            return;
        }
        if (op == "&&")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value != 0.0 && right_value != 0.0 ? 1.0 : 0.0));
            return;
        }
        if (op == "||")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value != 0.0 || right_value != 0.0 ? 1.0 : 0.0));
            return;
        }
        if (op == "<")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value < right_value ? 1.0 : 0.0));
            return;
        }
        if (op == ">")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value > right_value ? 1.0 : 0.0));
            return;
        }
        if (op == "==")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value == right_value ? 1.0 : 0.0));
            return;
        }
        if (op == "<=")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value <= right_value ? 1.0 : 0.0));
            return;
        }
        if (op == ">=")
        {
            m_result.push_back(std::make_shared<NumberNode>(left_value >= right_value ? 1.0 : 0.0));
            return;
        }
    }
    m_result.push_back(std::make_shared<BinaryOpNode>(lhs, node.op(), rhs));
}

void Simplifier::visit(const FunctionCallNode &node)
{
    node.arg()->visit(*this);
    Expr arg{m_result.back()};
    m_result.pop_back();

    if (const auto number = dynamic_cast<NumberNode *>(arg.get()))
    {
        const double result_value = evaluate(node.name(), number->value());
        m_result.push_back(std::make_shared<NumberNode>(result_value));
        return;
    }

    // If we can't simplify, keep the original function call
    m_result.push_back(std::make_shared<FunctionCallNode>(node.name(), arg));
}

void Simplifier::visit(const IdentifierNode &node)
{
    m_result.push_back(std::make_shared<IdentifierNode>(node.name()));
}

void Simplifier::visit(const IfStatementNode &node)
{
    throw std::runtime_error("IfStatementNode not implemented");
}

void Simplifier::visit(const NumberNode &node)
{
    m_result.push_back(std::make_shared<NumberNode>(node.value()));
}

void Simplifier::visit(const StatementSeqNode &node)
{
    assert(!node.statements().empty());
    if (node.statements().size() == 1)
    {
        m_result.push_back(node.statements().back());
        return;
    }

    std::vector<Expr> repl;
    bool was_number{false};
    for (Expr stmt : node.statements())
    {
        stmt->visit(*this);
        stmt = m_result.back();
        m_result.pop_back();
        if (!was_number)
        {
            repl.push_back(stmt);
            was_number = get_node_type(stmt) == NodeType::NUMBER;
        }
        else if (get_node_type(stmt) == NodeType::NUMBER)
        {
            repl.back() = stmt;
        }
        else
        {
            repl.push_back(stmt);
            was_number = false;
        }
    }
    m_result.push_back(std::make_shared<StatementSeqNode>(repl));
}

void Simplifier::visit(const UnaryOpNode &node)
{
    node.operand()->visit(*this);
    Expr op{m_result.back()};
    m_result.pop_back();
    if (auto number = dynamic_cast<NumberNode *>(op.get()))
    {
        double value = number->value();
        switch (node.op())
        {
        case '+':
            // Unary plus, no change
            break;
        case '-':
            // Unary minus
            value = -value;
            break;
        case '|':
            // Unary modulus
            value *= value;
            break;
        default:
            // Unknown operator
            throw std::runtime_error("Unknown unary operator '" + std::string{1, node.op()} + "' in simplifier");
        }
        m_result.push_back(std::make_shared<NumberNode>(value));
        return;
    }
    m_result.push_back(std::make_shared<UnaryOpNode>(node.op(), node.operand()));
}

} // namespace

Expr simplify(const Expr &expr)
{
    Simplifier simplifier;
    expr->visit(simplifier);
    return simplifier.result();
}

} // namespace formula
