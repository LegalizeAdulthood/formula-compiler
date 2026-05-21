// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include "NodeFormatter.h"

#include <formula/Node.h>

#include <iostream>
#include <sstream>

namespace formula::test
{

void NodeFormatter::visit(const ast::AssignmentNode &node)
{
    m_str << "assignment:";
    if (!node.variable().empty())
    {
        m_str << node.variable() << '\n';
    }
    else
    {
        m_str << "{\n";
        node.target()->visit(*this);
        m_str << "}\n";
    }
    node.expression()->visit(*this);
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
    for (const ast::Expr &arg : node.args())
    {
        arg->visit(*this);
    }
    m_str << ")\n";
}

void NodeFormatter::visit(const ast::FunctionBlockNode &node)
{
    m_str << "function_block:";
    if (node.type().empty())
    {
        m_str << node.name();
    }
    else
    {
        m_str << node.type() << ',' << node.name();
    }
    m_str << " {\n";
    if (node.block())
    {
        node.block()->visit(*this);
    }
    m_str << "}\n";
}

void NodeFormatter::visit(const ast::HeadingBlockNode &node)
{
    m_str << "heading_block {\n";
    if (node.block())
    {
        node.block()->visit(*this);
    }
    m_str << "}\n";
}

void NodeFormatter::visit(const ast::ConstantRefNode &node)
{
    m_str << "constant_ref:" << node.name() << '\n';
}

void NodeFormatter::visit(const ast::DeclarationNode &node)
{
    m_str << "declaration:" << node.type() << ',' << node.name();
    if (node.is_array())
    {
        m_str << '[';
        bool first = true;
        for (const ast::Expr &dimension : node.dimensions())
        {
            if (!first)
            {
                m_str << ',';
            }
            first = false;
            if (dimension)
            {
                m_str << "\n";
                dimension->visit(*this);
            }
        }
        m_str << ']';
    }
    m_str << '\n';
    if (node.initializer())
    {
        node.initializer()->visit(*this);
    }
}

void NodeFormatter::visit(const ast::FunctionDeclNode &node)
{
    m_str << "function_decl:";
    if (!node.return_type().empty())
    {
        m_str << node.return_type() << ' ';
    }
    m_str << node.name() << '(';
    bool first = true;
    for (const ast::FunctionArgument &arg : node.args())
    {
        if (!first)
        {
            m_str << ',';
        }
        first = false;
        if (arg.is_const)
        {
            m_str << "const ";
        }
        m_str << arg.type << ' ';
        if (arg.is_by_ref)
        {
            m_str << '&';
        }
        m_str << arg.name;
    }
    m_str << ')';
    if (node.is_const())
    {
        m_str << " const";
    }
    if (node.is_static())
    {
        m_str << " static";
    }
    m_str << " {\n";
    if (node.body())
    {
        node.body()->visit(*this);
    }
    m_str << "}\n";
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

void NodeFormatter::visit(const ast::IndexNode &node)
{
    m_str << "index:[\n";
    node.target()->visit(*this);
    for (const ast::Expr &index : node.indices())
    {
        index->visit(*this);
    }
    m_str << "]\n";
}

void NodeFormatter::visit(const ast::LiteralNode &node)
{
    m_str << "literal:";
    switch (node.value().index())
    {
    case 0:
        m_str << std::get<int>(node.value());
        break;
    case 1:
        m_str << std::get<double>(node.value());
        break;
    case 2:
        m_str << '(' << std::get<Complex>(node.value()).re << ',' << std::get<Complex>(node.value()).im << ')';
        break;
    case 3:
        m_str << (std::get<bool>(node.value()) ? "true" : "false");
        break;
    case 4:
        m_str << '"' << std::get<std::string>(node.value()) << '"';
        break;
    case 5:
    {
        const ast::LiteralNode::Color &value{std::get<ast::LiteralNode::Color>(node.value())};
        m_str << "rgba(" << value.red << ',' << value.green << ',' << value.blue << ',' << value.alpha << ')';
        break;
    }
    default:
        throw std::runtime_error("LiteralNode value variant index out of range");
    }
    m_str << '\n';
}

void NodeFormatter::visit(const ast::MemberAccessNode &node)
{
    m_str << "member:" << node.member() << " {\n";
    node.target()->visit(*this);
    m_str << "}\n";
}

void NodeFormatter::visit(const ast::NewNode &node)
{
    m_str << "new:" << node.type() << "(\n";
    for (const ast::Expr &arg : node.args())
    {
        arg->visit(*this);
    }
    m_str << ")\n";
}

void NodeFormatter::visit(const ast::ParamBlockNode &node)
{
    m_str << "param_block:";
    if (node.type().empty())
    {
        m_str << node.name();
    }
    else
    {
        m_str << node.type() << ',' << node.name();
    }
    m_str << " {\n";
    if (node.block())
    {
        node.block()->visit(*this);
    }
    m_str << "}\n";
}

void NodeFormatter::visit(const ast::ParameterRefNode &node)
{
    m_str << "parameter_ref:" << node.name() << '\n';
}

void NodeFormatter::visit(const ast::RepeatUntilNode &node)
{
    m_str << "repeat {\n";
    if (node.body())
    {
        node.body()->visit(*this);
    }
    m_str << "} until (\n";
    node.condition()->visit(*this);
    m_str << ")\n";
}

void NodeFormatter::visit(const ast::ReturnNode &node)
{
    m_str << "return";
    if (node.expression())
    {
        m_str << ":\n";
        node.expression()->visit(*this);
    }
    else
    {
        m_str << '\n';
    }
}

void NodeFormatter::visit(const ast::SettingNode &node)
{
    m_str << "setting:" << node.key() << "=";
    switch (node.value().index())
    {
    case 0:
        m_str << std::get<0>(node.value());
        break;

    case 1:
    {
        const Complex &value{std::get<1>(node.value())};
        m_str << '(' << value.re << ',' << value.im << ')';
        break;
    }
    case 2:
        m_str << '"' << std::get<2>(node.value()) << '"';
        break;
    case 3:
        m_str << std::get<3>(node.value());
        break;
    case 4:
        m_str << std::get<4>(node.value()).name;
        break;
    case 5:
        m_str << (std::get<5>(node.value()) ? "true" : "false");
        break;
    case 6:
        m_str << "{\n";
        std::get<ast::Expr>(node.value())->visit(*this);
        m_str << '}';
        break;
    case 7:
        m_str << "{\n";
        for (const std::string &str : std::get<7>(node.value()))
        {
            m_str << '"' << str << "\"\n";
        }
        m_str << '}';
        break;
    case 8:
        m_str << std::get<8>(node.value()).src;
        break;
    default:
        throw std::runtime_error("ValueType variant index out of range");
    }

    m_str << '\n';
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

void NodeFormatter::visit(const ast::UnaryOpNode &node)
{
    m_str << "unary_op:" << node.op() << '\n';
    node.operand()->visit(*this);
}

void NodeFormatter::visit(const ast::WhileNode &node)
{
    m_str << "while:(\n";
    node.condition()->visit(*this);
    m_str << ") {\n";
    if (node.body())
    {
        node.body()->visit(*this);
    }
    m_str << "}\n";
}

std::string to_string(const ast::Expr &expr)
{
    std::ostringstream str;
    NodeFormatter formatter(str);
    expr->visit(formatter);
    return str.str();
}

} // namespace formula::test
