// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

namespace formula::ast
{

class AssignmentNode;
class BinaryOpNode;
class DefaultNode;
class FunctionCallNode;
class IdentifierNode;
class IfStatementNode;
class NumberNode;
class StatementSeqNode;
class TypeNode;
class UnaryOpNode;

class Visitor
{
public:
    Visitor() = default;
    virtual ~Visitor() = default;

    virtual void visit(const AssignmentNode &node) = 0;
    virtual void visit(const BinaryOpNode &node) = 0;
    virtual void visit(const DefaultNode &node) = 0;
    virtual void visit(const FunctionCallNode &node) = 0;
    virtual void visit(const IdentifierNode &node) = 0;
    virtual void visit(const IfStatementNode &node) = 0;
    virtual void visit(const NumberNode &node) = 0;
    virtual void visit(const StatementSeqNode &node) = 0;
    virtual void visit(const TypeNode &node) = 0;
    virtual void visit(const UnaryOpNode &node) = 0;
};

class NullVisitor : public Visitor
{
public:
    NullVisitor() = default;
    ~NullVisitor() override = default;

    void visit(const AssignmentNode &node) override
    {
    }
    void visit(const BinaryOpNode &node) override
    {
    }
    void visit(const DefaultNode &node) override
    {
    }
    void visit(const FunctionCallNode &node) override
    {
    }
    void visit(const IdentifierNode &node) override
    {
    }
    void visit(const IfStatementNode &node) override
    {
    }
    void visit(const NumberNode &node) override
    {
    }
    void visit(const StatementSeqNode &node) override
    {
    }
    void visit(const TypeNode &node) override
    {
    }
    void visit(const UnaryOpNode &node) override
    {
    }
};

} // namespace formula::ast
