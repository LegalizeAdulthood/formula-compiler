// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

namespace formula::ast
{

class AssignmentNode;
class BinaryOpNode;
class ConstantRefNode;
class DeclarationNode;
class SettingNode;
class FunctionDeclNode;
class FunctionCallNode;
class IdentifierNode;
class IfStatementNode;
class IndexNode;
class LiteralNode;
class MemberAccessNode;
class NewNode;
class ParamBlockNode;
class ParameterRefNode;
class RepeatUntilNode;
class ReturnNode;
class StatementSeqNode;
class TypeNode;
class UnaryOpNode;
class WhileNode;

class Visitor
{
public:
    Visitor() = default;
    virtual ~Visitor() = default;

    virtual void visit(const AssignmentNode &node) = 0;
    virtual void visit(const BinaryOpNode &node) = 0;
    virtual void visit(const ConstantRefNode &node) = 0;
    virtual void visit(const DeclarationNode &node) = 0;
    virtual void visit(const FunctionDeclNode &node) = 0;
    virtual void visit(const FunctionCallNode &node) = 0;
    virtual void visit(const IdentifierNode &node) = 0;
    virtual void visit(const IfStatementNode &node) = 0;
    virtual void visit(const IndexNode &node) = 0;
    virtual void visit(const LiteralNode &node) = 0;
    virtual void visit(const MemberAccessNode &node) = 0;
    virtual void visit(const NewNode &node) = 0;
    virtual void visit(const ParamBlockNode &node) = 0;
    virtual void visit(const ParameterRefNode &node) = 0;
    virtual void visit(const RepeatUntilNode &node) = 0;
    virtual void visit(const ReturnNode &node) = 0;
    virtual void visit(const SettingNode &node) = 0;
    virtual void visit(const StatementSeqNode &node) = 0;
    virtual void visit(const UnaryOpNode &node) = 0;
    virtual void visit(const WhileNode &node) = 0;
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
    void visit(const ConstantRefNode &node) override
    {
    }
    void visit(const DeclarationNode &node) override
    {
    }
    void visit(const FunctionDeclNode &node) override
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
    void visit(const IndexNode &node) override
    {
    }
    void visit(const LiteralNode &node) override
    {
    }
    void visit(const MemberAccessNode &node) override
    {
    }
    void visit(const NewNode &node) override
    {
    }
    void visit(const ParamBlockNode &node) override
    {
    }
    void visit(const ParameterRefNode &node) override
    {
    }
    void visit(const RepeatUntilNode &node) override
    {
    }
    void visit(const ReturnNode &node) override
    {
    }
    void visit(const SettingNode &node) override
    {
    }
    void visit(const StatementSeqNode &node) override
    {
    }
    void visit(const UnaryOpNode &node) override
    {
    }
    void visit(const WhileNode &node) override
    {
    }
};

} // namespace formula::ast
