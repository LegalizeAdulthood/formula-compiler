// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

namespace formula::ast
{

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

    virtual void visit(const AssignmentNode &node) = 0;
    virtual void visit(const BinaryOpNode &node) = 0;
    virtual void visit(const FunctionCallNode &node) = 0;
    virtual void visit(const IdentifierNode &node) = 0;
    virtual void visit(const IfStatementNode &node) = 0;
    virtual void visit(const NumberNode &node) = 0;
    virtual void visit(const StatementSeqNode &node) = 0;
    virtual void visit(const UnaryOpNode &node) = 0;
};

} // namespace formula::ast
