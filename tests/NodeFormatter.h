// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/core/Visitor.h>

#include <memory>
#include <ostream>

namespace formula::ast
{
class Node;
using Expr = std::shared_ptr<Node>;
} // namespace formula::ast

namespace formula::test
{

class NodeFormatter : public ast::NullVisitor
{
public:
    NodeFormatter(std::ostream &str) :
        m_str(str)
    {
    }
    NodeFormatter(const NodeFormatter &rhs) = delete;
    NodeFormatter(NodeFormatter &&rhs) = delete;
    ~NodeFormatter() override = default;
    NodeFormatter &operator=(const NodeFormatter &rhs) = delete;
    NodeFormatter &operator=(NodeFormatter &&rhs) = delete;

    void visit(const ast::AssignmentNode &node) override;
    void visit(const ast::BinaryOpNode &node) override;
    void visit(const ast::ConstantRefNode &node) override;
    void visit(const ast::DeclarationNode &node) override;
    void visit(const ast::FunctionBlockNode &node) override;
    void visit(const ast::FunctionDeclNode &node) override;
    void visit(const ast::FunctionCallNode &node) override;
    void visit(const ast::HeadingBlockNode &node) override;
    void visit(const ast::IdentifierNode &node) override;
    void visit(const ast::IfStatementNode &node) override;
    void visit(const ast::IndexNode &node) override;
    void visit(const ast::LiteralNode &node) override;
    void visit(const ast::MemberAccessNode &node) override;
    void visit(const ast::NewNode &node) override;
    void visit(const ast::ParamBlockNode &node) override;
    void visit(const ast::ParameterRefNode &node) override;
    void visit(const ast::RepeatUntilNode &node) override;
    void visit(const ast::ReturnNode &node) override;
    void visit(const ast::SettingNode &node) override;
    void visit(const ast::StatementSeqNode &node) override;
    void visit(const ast::UnaryOpNode &node) override;
    void visit(const ast::WhileNode &node) override;

private:
    std::ostream &m_str;
};

std::string to_string(const ast::Expr &expr);

} // namespace formula::test
