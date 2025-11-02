// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Node.h>

#include <formula/Visitor.h>

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

void DefaultNode::visit(Visitor &visitor) const
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

void NumberNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void StatementSeqNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void TypeNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

void UnaryOpNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace formula::ast
