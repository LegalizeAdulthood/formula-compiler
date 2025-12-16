// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>
#include <formula/Visitor.h>

namespace formula::ast
{

enum class NodeType
{
    NONE = 0,
    ASSIGNMENT,
    BINARY_OP,
    SETTING,
    FUNCTION_CALL,
    IDENTIFIER,
    IF_STATEMENT,
    LITERAL,
    PARAM_BLOCK,
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
    void visit(const LiteralNode &node) override        { m_type = NodeType::LITERAL; }
    void visit(const ParamBlockNode &node) override     { m_type = NodeType::PARAM_BLOCK; }
    void visit(const SettingNode &node) override        { m_type = NodeType::SETTING; }
    void visit(const StatementSeqNode &node) override   { m_type = NodeType::STATEMENT_SEQ; }
    void visit(const UnaryOpNode &node) override        { m_type = NodeType::UNARY_OP; }
    // clang-format on

private:
    NodeType m_type{NodeType::NONE};
};

inline NodeType get_node_type(Expr node)
{
    Typer v;
    node->visit(v);
    return v.result();
}

} // namespace formula
