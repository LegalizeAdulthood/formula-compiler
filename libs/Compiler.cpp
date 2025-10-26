// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "Compiler.h"

#include "ast.h"
#include "functions.h"
#include "Visitor.h"

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

class Compiler : public Visitor
{
public:
    Compiler(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) :
        comp(comp),
        state(state),
        result(result)
    {
    }
    ~Compiler() override = default;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const NumberNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

    bool success() const
    {
        return m_success;
    }

private:
    asmjit::x86::Compiler &comp;
    EmitterState &state;
    asmjit::x86::Xmm result;
    bool m_success{true};
};

void Compiler::visit(const NumberNode &node)
{
    asmjit::Label label = get_constant_label(comp, state.data.constants, {node.value(), 0.0});
    comp.movlpd(result, asmjit::x86::ptr(label));
    comp.movhpd(result, asmjit::x86::ptr(label, sizeof(double)));
}

void Compiler::visit(const IdentifierNode &node)
{
    asmjit::Label label{get_symbol_label(comp, state.data.symbols, node.name())};
    comp.movlpd(result, asmjit::x86::ptr(label));
    comp.movhpd(result, asmjit::x86::ptr(label, sizeof(double)));
}

static void call(asmjit::x86::Compiler &comp, double (*fn)(double), asmjit::x86::Xmm result)
{
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    comp.invoke(&call, target, asmjit::FuncSignature::build<double, double>());
    call->setArg(0, result);
    call->setRet(0, result);
}

void Compiler::visit(const FunctionCallNode &node)
{
    node.arg().visit(*this);
    if (!success())
    {
        return;
    }
    m_success = true;
    const std::string &name{node.name()};
    if (name == "conj")
    {
        asmjit::x86::Xmm xmm1{comp.newXmm()};
        comp.xorpd(xmm1, xmm1);       // xmm1 = 0.0
        comp.subpd(xmm1, result);     // xmm1 -= result       [-re, -im]
        comp.shufpd(result, xmm1, 2); // result.y = xmm1.y    [re, -im]
        return;
    }
    if (name == "flip")
    {
        comp.shufpd(result, result, 1); // result = result.yx
        return;
    }
    if (name == "ident")
    {
        // identity does nothing
        return;
    }
    // if (ComplexFunction *fn = lookup_complex(m_name))
    //{
    //     m_arg->compile(comp, state, result);
    //     return call(comp, fn, result);
    // }
    if (RealFunction *fn = lookup_real(name))
    {
        call(comp, fn, result);
    }
}

void Compiler::visit(const UnaryOpNode &node)
{
    const char op{node.op()};
    if (op == '+')
    {
        node.operand().visit(*this);
    }
    else if (op == '-')
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        node.operand().visit(*this);
        if (!success())
        {
            return;
        }
        asmjit::x86::Xmm tmp = comp.newXmm();
        comp.xorpd(tmp, tmp);     // tmp = 0.0          [0.0, 0.0]
        comp.subpd(tmp, operand); // tmp -= operand     [-re, -im]      negate operand
        comp.movapd(result, tmp); // result = tmp       [-re, -im]
    }
    else if (op == '|') // modulus operator |x + yi| returns x^2 + y^2
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        node.operand().visit(*this);
        if (!success())
        {
            return;
        }
        comp.mulpd(operand, operand);     // op *= op           [x^2, y^2]
        comp.xorpd(result, result);       // result = 0         [0.0, 0.0]
        comp.movsd(result, operand);      // result.x = op.x    [x^2, 0.0]
        comp.shufpd(operand, operand, 1); // op = op.yx         [y^2, x^2]
        comp.addsd(result, operand);      // result.x += op.x   [x^2 + y^2, 0.0]
    }
    else
    {
        m_success = false;
    }
}

static bool call(asmjit::x86::Compiler &comp, double (*fn)(double, double), asmjit::x86::Xmm result, asmjit::x86::Xmm right)
{
    // For exponentiation, we can use the pow intrinsic
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    comp.invoke(&call, target, asmjit::FuncSignature::build<double, double, double>());
    call->setArg(0, result);
    call->setArg(1, right);
    call->setRet(0, result);
    return true;
}

void Compiler::visit(const BinaryOpNode &node)
{
    node.left().visit(*this);
    asmjit::Label skip_right = comp.newLabel();
    asmjit::x86::Xmm right{comp.newXmm()};
    const std::string &op{node.op()};
    if (op == "&&")
    {
        asmjit::x86::Xmm zero{comp.newXmm()};
        comp.xorpd(zero, zero);     // xmm = 0.0
        comp.ucomisd(result, zero); // xmm0 <=> 0.0?
        asmjit::Label eval_right = comp.newLabel();
        comp.jne(eval_right); // xmm0 != 0.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(right, asmjit::x86::ptr(one));
        comp.jmp(skip_right);
        comp.bind(eval_right);
        comp.movsd(result, asmjit::x86::ptr(one));
    }
    if (op == "||")
    {
        asmjit::x86::Xmm zero{comp.newXmm()};
        comp.xorpd(zero, zero);     // xmm = 0.0
        comp.ucomisd(result, zero); // result <=> 0.0?
        asmjit::Label eval_right = comp.newLabel();
        comp.je(eval_right); // result == 0.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(result, asmjit::x86::ptr(one));
        comp.jmp(skip_right);
        comp.bind(eval_right);
        comp.movsd(result, zero);
    }
    node.right().visit(*this);
    comp.bind(skip_right);
    if (op == "+")
    {
        comp.addpd(result, right); // result += right
        return;
    }
    if (op == "-")
    {
        comp.subpd(result, right); // result -= right
        return;
    }
    if (op == "*")
    {
        // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
        asmjit::x86::Xmm xmm0{result};        // xmm0 = [a, b]
        asmjit::x86::Xmm xmm1{right};         // xmm1 = [c, d]
        asmjit::x86::Xmm xmm2{comp.newXmm()}; //
        asmjit::x86::Xmm xmm3{comp.newXmm()}; //
        comp.movapd(xmm2, xmm0);              // xmm2 = xmm0            [a, b]
        comp.mulpd(xmm2, xmm1);               // xmm2 *= xmm1           [ac, bd]
        comp.movapd(xmm3, xmm1);              // xmm3 = xmm1            [c, d]
        comp.shufpd(xmm3, xmm3, 1);           // xmm3 = xmm3.yx         [d, c]
        comp.mulpd(xmm3, xmm0);               // xmm3 *= xmm0           [ad, bc]
        comp.movapd(xmm0, xmm2);              // xmm0 = xmm2            [ac, bd]
        comp.shufpd(xmm2, xmm2, 1);           // xmm2 = xmm2.yx         [bd, ac]
        comp.subsd(xmm0, xmm2);               // xmm0.x -= xmm2.x       [ac - bd, bd]
        comp.movapd(xmm1, xmm3);              // xmm1 = xmm3            [ad, bc]
        comp.shufpd(xmm3, xmm3, 1);           // xmm3 = xmm3.yx         [bc, ad]
        comp.addsd(xmm1, xmm3);               // xmm1.x += xmm3.x       [ad + bc, ad]
        comp.unpcklpd(xmm0, xmm1);            // xmm0 = xmm0.x, xmm1.x  [ac - bd, ad + bc]
        return;
    }
    if (op == "/")
    {
        // (u + vi) / (x + yi) = ((ux + vy) + (vx - uy)i) / (x^2 + y^2)
        // (1 + 2i) / (3 + 4i) = ((1*3 + 2*4) + (2*3 - 1*4)i) / (3^2 + 4^2)
        //                     = ((3 + 8) + (6 - 4)i) / (9 + 16)
        //                     = (11 + 2i) / 25
        asmjit::x86::Xmm xmm0{result};        // xmm0 = [u, v]
        asmjit::x86::Xmm xmm1{right};         // xmm1 = [x, y]
        asmjit::x86::Xmm xmm2{comp.newXmm()}; //
        asmjit::x86::Xmm xmm3{comp.newXmm()}; //
        asmjit::x86::Xmm xmm4{comp.newXmm()}; //
        comp.movapd(xmm2, xmm1);              // xmm2 = [x, y]
        comp.mulpd(xmm2, xmm2);               // xmm2 *= xmm1      [x^2, y^2]              squares
        comp.movapd(xmm3, xmm2);              // xmm3 = xmm2       [x^2, y^2]
        comp.shufpd(xmm3, xmm3, 1);           // xmm3 = xmm2.yx    [y^2, x^2]              swap lanes
        comp.addpd(xmm2, xmm3);               // xmm2 += xmm3      [x^2 + y^2, x^2 + y^2]  denominator in both lanes
        comp.movapd(xmm3, xmm0);              // xmm3 = xmm0       [u, v]
        comp.mulpd(xmm3, xmm1);               // xmm3 *= xmm1      [ux, vy]              real part products
        comp.shufpd(xmm0, xmm0, 1);           // xmm0 = xmm0.yx    [v, u]                  swap lanes
        comp.movapd(xmm4, xmm1);              // xmm4 = xmm1       [x, y]
        comp.mulpd(xmm4, xmm0);               // xmm4 *= xmm0      [vx, uy]              imaginary part products
                                              //
                                              // at this point:
                                              //   xmm0 = [v, u]
                                              //   xmm1 = [x, y]
                                              //   xmm2 = [x^2 + y^2, x^2 + y^2]           real, imaginary denominator
                                              //   xmm3 = [ux, vy]                         real part products
                                              //   xmm4 = [vx, uy]                         imaginary part products
                                              //
        comp.movapd(xmm0, xmm3);              // xmm0 = xmm3       [ux, vy]
        comp.shufpd(xmm0, xmm0, 1);           // xmm0 = xmm0.yx    [vy, ux]                swap lanes
        comp.addsd(xmm0, xmm3);               // xmm0.x += xmm3.x  [ux + vy, vy]           add real parts
                                //                                           xmm0.x is real part of numerator
        comp.movapd(xmm1, xmm4);    // xmm1 = xmm4       [vx, uy]
        comp.shufpd(xmm1, xmm1, 1); // xmm1 = xmm1.yx    [uy, vx]                swap lanes
        comp.movapd(xmm3, xmm4);    // xmm3 = xmm4       [vx, uy]
        comp.subsd(xmm4, xmm1);     // xmm4.x -= xmm1.x  [vx - uy, uy]           xmm4.x is imaginary part of numerator
        comp.unpcklpd(xmm0, xmm4);  // xmm0.y = xmm4.x   [ux + vy, vx - uy]      swizzle lanes
        comp.divpd(xmm0, xmm2);     // xmm0 /= xmm2      [ux + vy, vx - uy]/(x^2 + y^2)
        return;
    }
    if (op == "^")
    {
        // For exponentiation, we can use the pow intrinsic
        call(comp, std::pow, result, right);
        return;
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=")
    {
        // Compare left and right, set result to 1.0 on true, else 0.0
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        comp.ucomisd(result, right); // xmm0 <=> xmm1
        if (op == "<")
        {
            comp.jb(success); // xmm0 < xmm1?
        }
        else if (op == "<=")
        {
            comp.jbe(success); // xmm0 <= xmm1?
        }
        else if (op == ">")
        {
            comp.ja(success); // xmm0 > xmm1?
        }
        else if (op == ">=")
        {
            comp.jae(success); // xmm0 >= xmm1?
        }
        else if (op == "==")
        {
            comp.je(success); // xmm0 == xmm1?
        }
        else if (op == "!=")
        {
            comp.jne(success); // xmm0 != xmm1?
        }
        else
        {
            m_success = false; // Unsupported operator
            return;
        }
        comp.xorpd(result, result); // result = 0.0
        comp.jmp(end);
        comp.bind(success);
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(result, asmjit::x86::ptr(one));
        comp.bind(end);
        return;
    }
    if (op == "&&")
    {
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        asmjit::x86::Xmm zero{comp.newXmm()};
        comp.xorpd(zero, zero);     // xmm = 0.0
        comp.ucomisd(result, zero); // result <=> 0.0?
        comp.je(end);               // result == 0.0?
        comp.ucomisd(right, zero);  // right <=> 0.0?
        comp.jne(success);
        comp.movsd(result, zero);
        comp.jmp(end);
        comp.bind(success);
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(result, asmjit::x86::ptr(one));
        comp.bind(end);
        return;
    }
    if (op == "||")
    {
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        asmjit::x86::Xmm zero{comp.newXmm()};
        comp.xorpd(zero, zero);     // xmm = 0.0
        comp.ucomisd(result, zero); // result <=> 0.0?
        comp.jne(end);              // result != 0.0
        comp.ucomisd(right, zero);  // right <=> 0.0?
        comp.jne(success);
        comp.movsd(result, zero);
        comp.jmp(end);
        comp.bind(success);
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(result, asmjit::x86::ptr(one));
        comp.bind(end);
        return;
    }

    m_success = false;
}

void Compiler::visit(const AssignmentNode &node)
{
    asmjit::Label label = get_symbol_label(comp, state.data.symbols, node.variable());
    node.expression().visit(*this);
    if (!success())
    {
        return;
    }
    comp.movsd(asmjit::x86::ptr(label), result);
}

void Compiler::visit(const StatementSeqNode &node)
{
    for (const Expr &statement : node.statements())
    {
        statement->visit(*this);
        if (!success())
        {
            return;
        }
    }
}

void Compiler::visit(const IfStatementNode &node)
{
    asmjit::Label else_label = comp.newLabel();
    asmjit::Label end_label = comp.newLabel();
    node.condition().visit(*this);
    asmjit::x86::Xmm zero{comp.newXmm()};
    comp.xorpd(zero, zero);     // xmm = 0.0
    comp.ucomisd(result, zero); // result <=> 0.0?
    comp.jz(else_label);        // if result == 0.0, jump to else block
    if (node.has_then_block())
    {
        node.then_block().visit(*this);
        if (!success())
        {
            return;
        }
    }
    else
    {
        // If no then block, just set result to 1.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(result, asmjit::x86::ptr(one));
    }
    comp.jmp(end_label); // Jump over else block
    comp.bind(else_label);
    if (node.has_else_block())
    {
        node.else_block().visit(*this);
        if (!success())
        {
            return;
        }
    }
    else
    {
        // If no else block, set result to 0.0
        comp.movsd(result, zero);
    }
    comp.bind(end_label);
}

bool compile(
    const std::shared_ptr<Node> &expr, asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result)
{
    Compiler compiler(comp, state, result);
    expr->visit(compiler);
    return compiler.success();
}

} // namespace formula::ast
