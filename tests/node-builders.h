// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>
#include <formula/Visitor.h>

#include <memory>
#include <string>
#include <vector>

namespace formula::ast
{

inline std::shared_ptr<NumberNode> number(double value)
{
    return std::make_shared<NumberNode>(value);
}

inline std::shared_ptr<IdentifierNode> identifier(const std::string &name)
{
    return std::make_shared<IdentifierNode>(name);
}

inline std::shared_ptr<FunctionCallNode> function_call(const std::string &name, const Expr &arg)
{
    return std::make_shared<FunctionCallNode>(name, arg);
}

inline std::shared_ptr<StatementSeqNode> statements(const std::vector<Expr> &statements)
{
    return std::make_shared<StatementSeqNode>(statements);
}

inline std::shared_ptr<UnaryOpNode> unary(char op, const Expr &operand)
{
    return std::make_shared<UnaryOpNode>(op, operand);
}

inline std::shared_ptr<BinaryOpNode> binary(const Expr &left, const char op, const Expr &right)
{
    return std::make_shared<BinaryOpNode>(left, op, right);
}

inline std::shared_ptr<BinaryOpNode> binary(const Expr &left, const std::string &op, const Expr &right)
{
    return std::make_shared<BinaryOpNode>(left, op, right);
}

} // namespace formula::ast
