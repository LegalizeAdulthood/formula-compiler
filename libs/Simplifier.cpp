// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include "functions.h"
#include <formula/NodeTyper.h>
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

class Simplifier : public NullVisitor
{
public:
    Simplifier() = default;
    ~Simplifier() override = default;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const LiteralNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

    Expr result() const
    {
        return m_result.back();
    }

private:
    Expr visit_result(Expr node)
    {
        node->visit(*this);
        Expr result{m_result.back()};
        m_result.pop_back();
        return result;
    }
    std::vector<Expr> m_result;
};

void Simplifier::visit(const AssignmentNode &node)
{
    // keep existing
    m_result.push_back(std::make_shared<AssignmentNode>(node.variable(), node.expression()));
}

void Simplifier::visit(const BinaryOpNode &node)
{
    Expr lhs{visit_result(node.left())};
    auto left = dynamic_cast<LiteralNode *>(lhs.get());
    if (left)
    {
        // short-circuit and
        if (node.op() == "&&" && std::get<double>(left->value()) == 0.0)
        {
            m_result.push_back(std::make_shared<LiteralNode>(0.0));
            return;
        }
        // short-circuit or
        if (node.op() == "||" && std::get<double>(left->value()) != 0.0)
        {
            m_result.push_back(std::make_shared<LiteralNode>(1.0));
            return;
        }
    }

    Expr rhs{visit_result(node.right())};
    if (auto right = dynamic_cast<LiteralNode *>(rhs.get()))
    {
        const double left_value = std::get<double>(left->value());
        const double right_value = std::get<double>(right->value());
        const std::string &op{node.op()};
        if (op == "+")
        {
            m_result.push_back(std::make_shared<LiteralNode>(left_value + right_value));
            return;
        }
        if (op == "-")
        {
            m_result.push_back(std::make_shared<LiteralNode>(left_value - right_value));
            return;
        }
        if (op == "*")
        {
            m_result.push_back(std::make_shared<LiteralNode>(left_value * right_value));
            return;
        }
        if (op == "/")
        {
            m_result.push_back(std::make_shared<LiteralNode>(left_value / right_value));
            return;
        }
        if (op == "^")
        {
            m_result.push_back(std::make_shared<LiteralNode>(std::pow(left_value, right_value)));
            return;
        }
        const auto bool_result{[&](bool expr)
            {
                m_result.push_back(std::make_shared<LiteralNode>(expr ? 1.0 : 0.0));
            }};
        if (op == "&&")
        {
            bool_result(left_value != 0.0 && right_value != 0.0);
            return;
        }
        if (op == "||")
        {
            bool_result(left_value != 0.0 || right_value != 0.0);
            return;
        }
        if (op == "<")
        {
            bool_result(left_value < right_value);
            return;
        }
        if (op == ">")
        {
            bool_result(left_value > right_value ? 1.0 : 0.0);
            return;
        }
        if (op == "==")
        {
            bool_result(left_value == right_value ? 1.0 : 0.0);
            return;
        }
        if (op == "<=")
        {
            bool_result(left_value <= right_value ? 1.0 : 0.0);
            return;
        }
        if (op == ">=")
        {
            bool_result(left_value >= right_value ? 1.0 : 0.0);
            return;
        }
    }

    // keep existing
    m_result.push_back(std::make_shared<BinaryOpNode>(lhs, node.op(), rhs));
}

void Simplifier::visit(const FunctionCallNode &node)
{
    Expr arg{visit_result(node.arg())};
    if (const auto number = dynamic_cast<LiteralNode *>(arg.get()))
    {
        const double result_value = evaluate(node.name(), std::get<double>(number->value()));
        m_result.push_back(std::make_shared<LiteralNode>(result_value));
        return;
    }

    // keep existing
    m_result.push_back(std::make_shared<FunctionCallNode>(node.name(), arg));
}

void Simplifier::visit(const IdentifierNode &node)
{
    const std::string &name = node.name();

    // Simplify mathematical constants
    if (name == "pi")
    {
        m_result.push_back(std::make_shared<LiteralNode>(std::atan2(0.0, -1.0)));
        return;
    }
    if (name == "e")
    {
        m_result.push_back(std::make_shared<LiteralNode>(std::exp(1.0)));
        return;
    }

    // keep existing
    m_result.push_back(std::make_shared<IdentifierNode>(node.name()));
}

void Simplifier::visit(const IfStatementNode &node)
{
    Expr condition{visit_result(node.condition())};

    // If the condition is a constant number, we can simplify the if statement
    if (auto number = dynamic_cast<LiteralNode *>(condition.get()))
    {
        // Non-zero condition: simplify to then block (or 1.0 if no then block)
        if (std::get<double>(number->value()) != 0.0)
        {
            if (node.has_then_block())
            {
                node.then_block()->visit(*this);
            }
            else
            {
                m_result.push_back(std::make_shared<LiteralNode>(1.0));
            }
        }
        // Zero condition: simplify to else block (or 0.0 if no else block)
        else
        {
            if (node.has_else_block())
            {
                node.else_block()->visit(*this);
            }
            else
            {
                m_result.push_back(std::make_shared<LiteralNode>(0.0));
            }
        }
        return;
    }

    // If condition is not a constant, keep the original if statement
    // but simplify the blocks recursively
    Expr then_block = nullptr;
    Expr else_block = nullptr;

    if (node.has_then_block())
    {
        then_block = visit_result(node.then_block());
    }

    if (node.has_else_block())
    {
        else_block = visit_result(node.else_block());
    }

    m_result.push_back(std::make_shared<IfStatementNode>(condition, then_block, else_block));
}

void Simplifier::visit(const LiteralNode &node)
{
    m_result.push_back(std::make_shared<LiteralNode>(std::get<double>(node.value())));
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
        stmt = visit_result(stmt);
        if (!was_number)
        {
            repl.push_back(stmt);
            was_number = get_node_type(stmt) == NodeType::LITERAL;
        }
        else if (get_node_type(stmt) == NodeType::LITERAL)
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
    Expr op{visit_result(node.operand())};
    if (auto number = dynamic_cast<LiteralNode *>(op.get()))
    {
        double value = std::get<double>(number->value());
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
        m_result.push_back(std::make_shared<LiteralNode>(value));
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
