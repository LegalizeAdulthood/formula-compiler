// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include <formula/Visitor.h>

#include <cassert>
#include <cmath>
#include <memory>
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

    void visit(const AssignmentNode &node) override { m_type = NodeType::ASSIGNMENT;}
    void visit(const BinaryOpNode &node) override { m_type = NodeType::BINARY_OP;}
    void visit(const FunctionCallNode &node) override{m_type = NodeType::FUNCTION_CALL;}
    void visit(const IdentifierNode &node) override{m_type = NodeType::IDENTIFIER;}
    void visit(const IfStatementNode &node) override{m_type = NodeType::IF_STATEMENT;}
    void visit(const NumberNode &node) override{m_type = NodeType::NUMBER;}
    void visit(const StatementSeqNode &node) override{m_type = NodeType::STATEMENT_SEQ;}
    void visit(const UnaryOpNode &node) override{m_type =NodeType::UNARY_OP;}

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
    }
    m_result.push_back(std::make_shared<BinaryOpNode>(lhs, node.op(), rhs));
}

void Simplifier::visit(const FunctionCallNode &node)
{
}

void Simplifier::visit(const IdentifierNode &node)
{
    m_result.push_back(std::make_shared<IdentifierNode>(node.name()));
}

void Simplifier::visit(const IfStatementNode &node)
{
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
    if (auto number = dynamic_cast<NumberNode *>(node.operand().get()))
    {
        double value = number->value();
        switch (node.op())
        {
        case '+':
            // Unary plus, no change
            m_result.push_back(std::make_shared<NumberNode>(value));
            return;
        case '-':
            // Unary minus
            m_result.push_back(std::make_shared<NumberNode>(-value));
            return;
        default:
            // Unknown operator, fall through
            break;
        }
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
