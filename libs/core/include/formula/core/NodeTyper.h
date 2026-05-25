// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/core/Node.h>
#include <formula/core/Visitor.h>

namespace formula::ast
{

enum class NodeType
{
    NONE = 0,
    ASSIGNMENT,
    BINARY_OP,
    SETTING,
    CONSTANT_REF,
    DECLARATION,
    FUNCTION_DECL,
    FUNCTION_BLOCK,
    FUNCTION_CALL,
    HEADING_BLOCK,
    IDENTIFIER,
    IF_STATEMENT,
    INDEX,
    LITERAL,
    MEMBER_ACCESS,
    NEW_OBJECT,
    PARAM_BLOCK,
    PARAMETER_REF,
    REPEAT_UNTIL,
    RETURN,
    STATEMENT_SEQ,
    UNARY_OP,
    WHILE_LOOP
};

class Typer : public NullVisitor
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
    void visit(const ConstantRefNode &node) override    { m_type = NodeType::CONSTANT_REF; }
    void visit(const DeclarationNode &node) override    { m_type = NodeType::DECLARATION; }
    void visit(const FunctionBlockNode &node) override  { m_type = NodeType::FUNCTION_BLOCK; }
    void visit(const FunctionDeclNode &node) override   { m_type = NodeType::FUNCTION_DECL; }
    void visit(const FunctionCallNode &node) override   { m_type = NodeType::FUNCTION_CALL; }
    void visit(const HeadingBlockNode &node) override   { m_type = NodeType::HEADING_BLOCK; }
    void visit(const IdentifierNode &node) override     { m_type = NodeType::IDENTIFIER; }
    void visit(const IfStatementNode &node) override    { m_type = NodeType::IF_STATEMENT; }
    void visit(const IndexNode &node) override          { m_type = NodeType::INDEX; }
    void visit(const LiteralNode &node) override        { m_type = NodeType::LITERAL; }
    void visit(const MemberAccessNode &node) override   { m_type = NodeType::MEMBER_ACCESS; }
    void visit(const NewNode &node) override            { m_type = NodeType::NEW_OBJECT; }
    void visit(const ParamBlockNode &node) override     { m_type = NodeType::PARAM_BLOCK; }
    void visit(const ParameterRefNode &node) override   { m_type = NodeType::PARAMETER_REF; }
    void visit(const RepeatUntilNode &node) override    { m_type = NodeType::REPEAT_UNTIL; }
    void visit(const ReturnNode &node) override         { m_type = NodeType::RETURN; }
    void visit(const SettingNode &node) override        { m_type = NodeType::SETTING; }
    void visit(const StatementSeqNode &node) override   { m_type = NodeType::STATEMENT_SEQ; }
    void visit(const UnaryOpNode &node) override        { m_type = NodeType::UNARY_OP; }
    void visit(const WhileNode &node) override          { m_type = NodeType::WHILE_LOOP; }
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

} // namespace formula::ast
