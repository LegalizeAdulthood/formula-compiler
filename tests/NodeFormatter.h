// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include "formula/Formula.h"

#include <formula/Visitor.h>

#include <ostream>

namespace formula::test
{

class NodeFormatter : public ast::Visitor
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
    void visit(const ast::FunctionCallNode &node) override;
    void visit(const ast::IdentifierNode &node) override;
    void visit(const ast::IfStatementNode &node) override;
    void visit(const ast::NumberNode &node) override;
    void visit(const ast::ParamBlockNode &node) override;
    void visit(const ast::SettingNode &node) override;
    void visit(const ast::StatementSeqNode &node) override;
    void visit(const ast::UnaryOpNode &node) override;

private:
    std::ostream &m_str;
};

std::string to_string(const ast::Expr &expr);

} // namespace formula::test
