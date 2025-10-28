// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "ast.h"

#include "Visitor.h"
#include "functions.h"

#include <algorithm>

//
// An Xmm register is 128 bits wide, holding two 64-bit doubles,
// bit enough to hold a complex double.
//
// The formula compiler stores the real part in the low 64-bits
// and the imaginary part in the high 64-bits.
//
namespace formula::ast
{

static asmjit::Label get_constant_label(asmjit::x86::Compiler &comp, ConstantBindings &labels, const Complex &value)
{
    if (const auto it = labels.find(value); it != labels.end())
    {
        return it->second.label;
    }

    // Create a new label for the constant
    asmjit::Label label = comp.newLabel();
    labels[value] = LabelBinding{label, false};
    return label;
}

static asmjit::Label get_symbol_label(asmjit::x86::Compiler &comp, SymbolBindings &labels, std::string name)
{
    if (const auto it = labels.find(name); it != labels.end())
    {
        return it->second.label;
    }

    // Create a new label for the symbol
    asmjit::Label label = comp.newNamedLabel(name.c_str());
    labels[name] = LabelBinding{label, false};
    return label;
}

CompileError NumberNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_constant_label(comp, state.data.constants, {m_value, 0.0});
    ASMJIT_CHECK(comp.movlpd(result, asmjit::x86::ptr(label)));
    ASMJIT_CHECK(comp.movhpd(result, asmjit::x86::ptr(label, sizeof(double))));
    return {};
}

void NumberNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

CompileError IdentifierNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label{get_symbol_label(comp, state.data.symbols, m_name)};
    ASMJIT_CHECK(comp.movlpd(result, asmjit::x86::ptr(label)));
    ASMJIT_CHECK(comp.movhpd(result, asmjit::x86::ptr(label, sizeof(double))));
    return {};
}

void IdentifierNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

static CompileError call(asmjit::x86::Compiler &comp, double (*fn)(double), asmjit::x86::Xmm result)
{
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    ASMJIT_CHECK(comp.invoke(&call, target, asmjit::FuncSignature::build<double, double>()));
    call->setArg(0, result);
    call->setRet(0, result);
    return {};
}

CompileError FunctionCallNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    if (const CompileError err = m_arg->compile(comp, state, result); err)
    {
        return err;
    }
    if (m_name == "conj")
    {
        asmjit::x86::Xmm xmm1{comp.newXmm()};
        ASMJIT_CHECK(comp.xorpd(xmm1, xmm1));       // xmm1 = 0.0
        ASMJIT_CHECK(comp.subpd(xmm1, result));     // xmm1 -= result       [-re, -im]
        ASMJIT_CHECK(comp.shufpd(result, xmm1, 2)); // result.y = xmm1.y    [re, -im]
        return {};
    }
    if (m_name == "flip")
    {
        ASMJIT_CHECK(comp.shufpd(result, result, 1)); // result = result.yx
        return {};
    }
    if (m_name == "ident")
    {
        // identity does nothing
        return {};
    }
    // if (ComplexFunction *fn = lookup_complex(m_name))
    //{
    //     m_arg->compile(comp, state, result);
    //     return call(comp, fn, result);
    // }
    if (RealFunction *fn = lookup_real(m_name))
    {
        return call(comp, fn, result);
    }
    return asmjit::kErrorInvalidArgument;
}

void FunctionCallNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

CompileError UnaryOpNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    if (m_op == '+')
    {
        return m_operand->compile(comp, state, result);
    }
    if (m_op == '-')
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        if (const CompileError err = m_operand->compile(comp, state, operand); err)
        {
            return err;
        }
        asmjit::x86::Xmm tmp = comp.newXmm();
        ASMJIT_CHECK(comp.xorpd(tmp, tmp));     // tmp = 0.0          [0.0, 0.0]
        ASMJIT_CHECK(comp.subpd(tmp, operand)); // tmp -= operand     [-re, -im]      negate operand
        ASMJIT_CHECK(comp.movapd(result, tmp)); // result = tmp       [-re, -im]
        return {};
    }
    if (m_op == '|') // modulus operator |x + yi| returns x^2 + y^2
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        if (const CompileError err = m_operand->compile(comp, state, operand); err)
        {
            return err;
        }
        ASMJIT_CHECK(comp.mulpd(operand, operand));     // op *= op           [x^2, y^2]
        ASMJIT_CHECK(comp.xorpd(result, result));       // result = 0         [0.0, 0.0]
        ASMJIT_CHECK(comp.movsd(result, operand));      // result.x = op.x    [x^2, 0.0]
        ASMJIT_CHECK(comp.shufpd(operand, operand, 1)); // op = op.yx         [y^2, x^2]
        ASMJIT_CHECK(comp.addsd(result, operand));      // result.x += op.x   [x^2 + y^2, 0.0]
        return {};
    }

    return asmjit::kErrorInvalidArgument;
}

void UnaryOpNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

static CompileError call(
    asmjit::x86::Compiler &comp, double (*fn)(double, double), asmjit::x86::Xmm result, asmjit::x86::Xmm right)
{
    // For exponentiation, we can use the pow intrinsic
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    ASMJIT_CHECK(comp.invoke(&call, target, asmjit::FuncSignature::build<double, double, double>()));
    call->setArg(0, result);
    call->setArg(1, right);
    call->setRet(0, result);
    return {};
}

CompileError BinaryOpNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    if (const CompileError err = m_left->compile(comp, state, result); err)
    {
        return err;
    }
    asmjit::Label skip_right = comp.newLabel();
    asmjit::x86::Xmm right{comp.newXmm()};
    if (m_op == "&&")
    {
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_CHECK(comp.xorpd(zero, zero));       // xmm = 0.0
        ASMJIT_CHECK(comp.ucomisd(result, zero));   // xmm0 <=> 0.0?
        asmjit::Label eval_right = comp.newLabel(); //
        ASMJIT_CHECK(comp.jne(eval_right));         // xmm0 != 0.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_CHECK(comp.movsd(right, asmjit::x86::ptr(one)));
        ASMJIT_CHECK(comp.jmp(skip_right));
        ASMJIT_CHECK(comp.bind(eval_right));
        ASMJIT_CHECK(comp.movsd(result, asmjit::x86::ptr(one)));
    }
    if (m_op == "||")
    {
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_CHECK(comp.xorpd(zero, zero));       // xmm = 0.0
        ASMJIT_CHECK(comp.ucomisd(result, zero));   // result <=> 0.0?
        asmjit::Label eval_right = comp.newLabel(); //
        ASMJIT_CHECK(comp.je(eval_right));          // result == 0.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_CHECK(comp.movsd(result, asmjit::x86::ptr(one)));
        ASMJIT_CHECK(comp.jmp(skip_right));
        ASMJIT_CHECK(comp.bind(eval_right));
        ASMJIT_CHECK(comp.movsd(result, zero));
    }
    if (const auto err = m_right->compile(comp, state, right); err)
    {
        return err;
    }
    ASMJIT_CHECK(comp.bind(skip_right));
    if (m_op == "+")
    {
        ASMJIT_CHECK(comp.addpd(result, right)); // result += right
        return {};
    }
    if (m_op == "-")
    {
        ASMJIT_CHECK(comp.subpd(result, right)); // result -= right
        return {};
    }
    if (m_op == "*")
    {
        // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
        asmjit::x86::Xmm xmm0{result};            // xmm0 = [a, b]
        asmjit::x86::Xmm xmm1{right};             // xmm1 = [c, d]
        asmjit::x86::Xmm xmm2{comp.newXmm()};     //
        asmjit::x86::Xmm xmm3{comp.newXmm()};     //
        ASMJIT_CHECK(comp.movapd(xmm2, xmm0));    // xmm2 = xmm0            [a, b]
        ASMJIT_CHECK(comp.mulpd(xmm2, xmm1));     // xmm2 *= xmm1           [ac, bd]
        ASMJIT_CHECK(comp.movapd(xmm3, xmm1));    // xmm3 = xmm1            [c, d]
        ASMJIT_CHECK(comp.shufpd(xmm3, xmm3, 1)); // xmm3 = xmm3.yx         [d, c]
        ASMJIT_CHECK(comp.mulpd(xmm3, xmm0));     // xmm3 *= xmm0           [ad, bc]
        ASMJIT_CHECK(comp.movapd(xmm0, xmm2));    // xmm0 = xmm2            [ac, bd]
        ASMJIT_CHECK(comp.shufpd(xmm2, xmm2, 1)); // xmm2 = xmm2.yx         [bd, ac]
        ASMJIT_CHECK(comp.subsd(xmm0, xmm2));     // xmm0.x -= xmm2.x       [ac - bd, bd]
        ASMJIT_CHECK(comp.movapd(xmm1, xmm3));    // xmm1 = xmm3            [ad, bc]
        ASMJIT_CHECK(comp.shufpd(xmm3, xmm3, 1)); // xmm3 = xmm3.yx         [bc, ad]
        ASMJIT_CHECK(comp.addsd(xmm1, xmm3));     // xmm1.x += xmm3.x       [ad + bc, ad]
        ASMJIT_CHECK(comp.unpcklpd(xmm0, xmm1));  // xmm0 = xmm0.x, xmm1.x  [ac - bd, ad + bc]
        return {};
    }
    if (m_op == "/")
    {
        // (u + vi) / (x + yi) = ((ux + vy) + (vx - uy)i) / (x^2 + y^2)
        // (1 + 2i) / (3 + 4i) = ((1*3 + 2*4) + (2*3 - 1*4)i) / (3^2 + 4^2)
        //                     = ((3 + 8) + (6 - 4)i) / (9 + 16)
        //                     = (11 + 2i) / 25
        asmjit::x86::Xmm xmm0{result};            // xmm0 = [u, v]
        asmjit::x86::Xmm xmm1{right};             // xmm1 = [x, y]
        asmjit::x86::Xmm xmm2{comp.newXmm()};     //
        asmjit::x86::Xmm xmm3{comp.newXmm()};     //
        asmjit::x86::Xmm xmm4{comp.newXmm()};     //
        ASMJIT_CHECK(comp.movapd(xmm2, xmm1));    // xmm2 = [x, y]
        ASMJIT_CHECK(comp.mulpd(xmm2, xmm2));     // xmm2 *= xmm1      [x^2, y^2]              squares
        ASMJIT_CHECK(comp.movapd(xmm3, xmm2));    // xmm3 = xmm2       [x^2, y^2]
        ASMJIT_CHECK(comp.shufpd(xmm3, xmm3, 1)); // xmm3 = xmm2.yx    [y^2, x^2]              swap lanes
        ASMJIT_CHECK(comp.addpd(xmm2, xmm3));     // xmm2 += xmm3      [x^2 + y^2, x^2 + y^2]  denominator in both lanes
        ASMJIT_CHECK(comp.movapd(xmm3, xmm0));    // xmm3 = xmm0       [u, v]
        ASMJIT_CHECK(comp.mulpd(xmm3, xmm1));     // xmm3 *= xmm1      [ux, vy]         real part products
        ASMJIT_CHECK(comp.shufpd(xmm0, xmm0, 1)); // xmm0 = xmm0.yx    [v, u]           swap lanes
        ASMJIT_CHECK(comp.movapd(xmm4, xmm1));    // xmm4 = xmm1       [x, y]
        ASMJIT_CHECK(comp.mulpd(xmm4, xmm0));     // xmm4 *= xmm0      [vx, uy]         imaginary part products
                                                  //
                                                  // at this point:
                                                  //   xmm0 = [v, u]
                                                  //   xmm1 = [x, y]
                                                  //   xmm2 = [x^2 + y^2, x^2 + y^2]        real, imaginary denominator
                                                  //   xmm3 = [ux, vy]                      real part products
                                                  //   xmm4 = [vx, uy]                      imaginary part products
                                                  //
        ASMJIT_CHECK(comp.movapd(xmm0, xmm3));    // xmm0 = xmm3       [ux, vy]
        ASMJIT_CHECK(comp.shufpd(xmm0, xmm0, 1)); // xmm0 = xmm0.yx    [vy, ux]         swap lanes
        ASMJIT_CHECK(comp.addsd(xmm0, xmm3));     // xmm0.x += xmm3.x  [ux + vy, vy]    add real parts
                                                  //                                    xmm0.x is real part of numerator
        ASMJIT_CHECK(comp.movapd(xmm1, xmm4));    // xmm1 = xmm4       [vx, uy]
        ASMJIT_CHECK(comp.shufpd(xmm1, xmm1, 1)); // xmm1 = xmm1.yx    [uy, vx]         swap lanes
        ASMJIT_CHECK(comp.movapd(xmm3, xmm4));    // xmm3 = xmm4       [vx, uy]
        ASMJIT_CHECK(comp.subsd(xmm4, xmm1));    // xmm4.x -= xmm1.x  [vx - uy, uy]        xmm4.x is numerator imaginary
        ASMJIT_CHECK(comp.unpcklpd(xmm0, xmm4)); // xmm0.y = xmm4.x   [ux + vy, vx - uy] swizzle lanes
        ASMJIT_CHECK(comp.divpd(xmm0, xmm2));    // xmm0 /= xmm2      [ux + vy, vx - uy]/(x^2 + y^2)
        return {};
    }
    if (m_op == "^")
    {
        // For exponentiation, we can use the pow intrinsic
        return call(comp, std::pow, result, right);
    }
    if (m_op == "<" || m_op == "<=" || m_op == ">" || m_op == ">=" || m_op == "==" || m_op == "!=")
    {
        // Compare left and right, set result to 1.0 on true, else 0.0
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        ASMJIT_CHECK(comp.ucomisd(result, right)); // xmm0 <=> xmm1
        if (m_op == "<")
        {
            ASMJIT_CHECK(comp.jb(success)); // xmm0 < xmm1?
        }
        else if (m_op == "<=")
        {
            ASMJIT_CHECK(comp.jbe(success)); // xmm0 <= xmm1?
        }
        else if (m_op == ">")
        {
            ASMJIT_CHECK(comp.ja(success)); // xmm0 > xmm1?
        }
        else if (m_op == ">=")
        {
            ASMJIT_CHECK(comp.jae(success)); // xmm0 >= xmm1?
        }
        else if (m_op == "==")
        {
            ASMJIT_CHECK(comp.je(success)); // xmm0 == xmm1?
        }
        else if (m_op == "!=")
        {
            ASMJIT_CHECK(comp.jne(success)); // xmm0 != xmm1?
        }
        else
        {
            return asmjit::kErrorInvalidArgument; // Unsupported operator
        }
        ASMJIT_CHECK(comp.xorpd(result, result)); // result = 0.0
        ASMJIT_CHECK(comp.jmp(end));
        ASMJIT_CHECK(comp.bind(success));
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_CHECK(comp.movsd(result, asmjit::x86::ptr(one)));
        ASMJIT_CHECK(comp.bind(end));
        return {};
    }
    if (m_op == "&&")
    {
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_CHECK(comp.xorpd(zero, zero));     // xmm = 0.0
        ASMJIT_CHECK(comp.ucomisd(result, zero)); // result <=> 0.0?
        ASMJIT_CHECK(comp.je(end));               // result == 0.0?
        ASMJIT_CHECK(comp.ucomisd(right, zero));  // right <=> 0.0?
        ASMJIT_CHECK(comp.jne(success));
        ASMJIT_CHECK(comp.movsd(result, zero));
        ASMJIT_CHECK(comp.jmp(end));
        ASMJIT_CHECK(comp.bind(success));
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_CHECK(comp.movsd(result, asmjit::x86::ptr(one)));
        ASMJIT_CHECK(comp.bind(end));
        return {};
    }
    if (m_op == "||")
    {
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_CHECK(comp.xorpd(zero, zero));     // xmm = 0.0
        ASMJIT_CHECK(comp.ucomisd(result, zero)); // result <=> 0.0?
        ASMJIT_CHECK(comp.jne(end));              // result != 0.0
        ASMJIT_CHECK(comp.ucomisd(right, zero));  // right <=> 0.0?
        ASMJIT_CHECK(comp.jne(success));
        ASMJIT_CHECK(comp.movsd(result, zero));
        ASMJIT_CHECK(comp.jmp(end));
        ASMJIT_CHECK(comp.bind(success));
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_CHECK(comp.movsd(result, asmjit::x86::ptr(one)));
        ASMJIT_CHECK(comp.bind(end));
        return {};
    }

    return asmjit::kErrorInvalidArgument;
}

void BinaryOpNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

CompileError AssignmentNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_symbol_label(comp, state.data.symbols, m_variable);
    if (const CompileError err = m_expression->compile(comp, state, result); err)
    {
        return err;
    }
    ASMJIT_CHECK(comp.movsd(asmjit::x86::ptr(label), result));
    return {};
}

void AssignmentNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

CompileError StatementSeqNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    CompileError err;
    const auto it = std::find_if(m_statements.begin(), m_statements.end(),
        [&comp, &state, &result, &err](const Expr &statement)
        {
            err = statement->compile(comp, state, result);
            return err;
        });
    return err;
}

void StatementSeqNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

CompileError IfStatementNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label else_label = comp.newLabel();
    asmjit::Label end_label = comp.newLabel();
    if (const CompileError err = m_condition->compile(comp, state, result); err)
    {
        return err;
    }
    asmjit::x86::Xmm zero{comp.newXmm()};
    ASMJIT_CHECK(comp.xorpd(zero, zero));     // xmm = 0.0
    ASMJIT_CHECK(comp.ucomisd(result, zero)); // result <=> 0.0?
    ASMJIT_CHECK(comp.jz(else_label));        // if result == 0.0, jump to else block
    if (m_then_block)
    {
        if (const CompileError err = m_then_block->compile(comp, state, result); err)
        {
            return err;
        }
    }
    else
    {
        // If no then block, just set result to 1.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_CHECK(comp.movsd(result, asmjit::x86::ptr(one)));
    }
    ASMJIT_CHECK(comp.jmp(end_label)); // Jump over else block
    ASMJIT_CHECK(comp.bind(else_label));
    if (m_else_block)
    {
        if (const CompileError err = m_else_block->compile(comp, state, result); err)
        {
            return err;
        }
    }
    else
    {
        // If no else block, set result to 0.0
        ASMJIT_CHECK(comp.movsd(result, zero));
    }
    ASMJIT_CHECK(comp.bind(end_label));
    return {};
}

void IfStatementNode::visit(Visitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace formula::ast
