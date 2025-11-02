// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "NodeFormatter.h"

#include <formula/Node.h>

#include <iostream>
#include <sstream>

namespace formula::test
{

void NodeFormatter::visit(const ast::AssignmentNode &node)
{
    m_str << "assignment:" << node.variable() << "=(\n";
    node.expression()->visit(*this);
    m_str << ")\n";
}

void NodeFormatter::visit(const ast::BinaryOpNode &node)
{
    m_str << "binary_op:" << node.op() << '\n';
    node.left()->visit(*this);
    node.right()->visit(*this);
}

void NodeFormatter::visit(const ast::FunctionCallNode &node)
{
    m_str << "function_call:" << node.name() << "(\n";
    node.arg()->visit(*this);
    m_str << ")\n";
}

void NodeFormatter::visit(const ast::IdentifierNode &node)
{
    m_str << "identifier:" << node.name() << '\n';
}

void NodeFormatter::visit(const ast::IfStatementNode &node)
{
    m_str << "if_statement:(\n";
    node.condition()->visit(*this);
    m_str << ") {\n";
    if (node.has_then_block())
    {
        node.then_block()->visit(*this);
    }
    if (node.has_else_block())
    {
        m_str << "} else {\n";
        node.else_block()->visit(*this);
        m_str << "} endif\n";
    }
    else
    {
        m_str << "} endif\n";
    }
}

void NodeFormatter::visit(const ast::NumberNode &node)
{
    m_str << "number:" << node.value() << '\n';
}

void NodeFormatter::visit(const ast::StatementSeqNode &node)
{
    m_str << "statement_seq:" << node.statements().size() << " {\n";
    for (const ast::Expr &stmt : node.statements())
    {
        stmt->visit(*this);
    }
    m_str << "}\n";
}

void NodeFormatter::visit(const ast::TypeNode &node)
{
    m_str << "type:" << +node.type() << '\n';
}

void NodeFormatter::visit(const ast::UnaryOpNode &node)
{
    m_str << "unary_op:" << node.op() << '\n';
    node.operand()->visit(*this);
}

std::string to_string(const ast::Expr &expr)
{
    std::ostringstream str;
    NodeFormatter formatter(str);
    expr->visit(formatter);
    return str.str();
}

} // namespace formula::test
