// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/core/Node.h>

#include <formula/core/Visitor.h>

//
// An Xmm register is 128 bits wide, holding two 64-bit doubles,
// bit enough to hold a complex double.
//
// The formula compiler stores the real part in the low 64-bits
// and the imaginary part in the high 64-bits.
//
namespace formula::ast
{

void AssignmentNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void BinaryOpNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void ConstantRefNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void DeclarationNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void FunctionDeclNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void FunctionBlockNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void HeadingBlockNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void FunctionCallNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void IdentifierNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void IfStatementNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void IndexNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void LiteralNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void MemberAccessNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void NewNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void ParamBlockNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void ParameterRefNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void RepeatUntilNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void ReturnNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void SettingNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void StatementSeqNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void UnaryOpNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void WhileNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace formula::ast
