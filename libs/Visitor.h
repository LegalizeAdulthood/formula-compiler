// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <map>
#include <string>

namespace formula::ast
{

struct FormulaDefinition;
class AssignmentNode;
class BinaryOpNode;
class Node;
class FunctionCallNode;
class IdentifierNode;
class IfStatementNode;
class NumberNode;
class StatementSeqNode;
class UnaryOpNode;

class Visitor
{
public:
    Visitor() = default;
    virtual ~Visitor() = default;

    virtual void visit(const FormulaDefinition &definition) = 0;
    virtual void visit(const AssignmentNode &node) = 0;
    virtual void visit(const BinaryOpNode &node) = 0;
    virtual void visit(const FunctionCallNode &node) = 0;
    virtual void visit(const IdentifierNode &node) = 0;
    virtual void visit(const IfStatementNode &node) = 0;
    virtual void visit(const NumberNode &node) = 0;
    virtual void visit(const StatementSeqNode &node) = 0;
    virtual void visit(const UnaryOpNode &node) = 0;
};

Complex interpret(const std::shared_ptr<Node> &expr, std::map<std::string, Complex> &symbols);

} // namespace formula::ast
