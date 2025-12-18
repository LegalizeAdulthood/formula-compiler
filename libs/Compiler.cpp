// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Compiler.h>

#include <formula/Visitor.h>

#include "functions.h"
#include <formula/Node.h>

#include <cassert>
#include <cmath>
#include <vector>

#define ASMJIT_STORE(expr_)                       \
    do                                            \
    {                                             \
        if (const asmjit::Error err = expr_; err) \
        {                                         \
            m_err = err;                          \
            return;                               \
        }                                         \
    } while (false)

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

static asmjit::Label get_symbol_label(asmjit::x86::Compiler &comp, SymbolBindings &labels, const std::string &name)
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

class Compiler : public NullVisitor
{
public:
    Compiler(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result, CompileError &err) :
        comp(comp),
        state(state),
        m_err(err)
    {
        m_result.push_back(result);
    }
    Compiler(const Compiler &rhs) = delete;
    Compiler(Compiler &&rhs) = delete;
    ~Compiler() override = default;
    Compiler &operator=(const Compiler &rhs) = delete;
    Compiler &operator=(Compiler &&rhs) = delete;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const LiteralNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

    bool success() const
    {
        return !m_err;
    }

private:
    void compile_operand(const Node &node, asmjit::x86::Xmm operand);

    asmjit::x86::Compiler &comp;
    EmitterState &state;
    std::vector<asmjit::x86::Xmm> m_result;
    CompileError &m_err;
};

void Compiler::visit(const LiteralNode &node)
{
    Complex value{};
    switch (node.value().index())
    {
    case 0:
        value.re = std::get<int>(node.value());
        break;

    case 1:
        value.re = std::get<double>(node.value());
        break;

    case 2:
        value = std::get<Complex>(node.value());
        break;

    default:
        throw std::runtime_error("Unknown LiteralNode variant index " + std::to_string(node.value().index()));
    }
    asmjit::Label label = get_constant_label(comp, state.data.constants, value);
    ASMJIT_STORE(comp.movlpd(m_result.back(), asmjit::x86::ptr(label)));
    ASMJIT_STORE(comp.movhpd(m_result.back(), asmjit::x86::ptr(label, sizeof(double))));
}

void Compiler::visit(const IdentifierNode &node)
{
    asmjit::Label label{get_symbol_label(comp, state.data.symbols, node.name())};
    ASMJIT_STORE(comp.movlpd(m_result.back(), asmjit::x86::ptr(label)));
    ASMJIT_STORE(comp.movhpd(m_result.back(), asmjit::x86::ptr(label, sizeof(double))));
}

static CompileError call(asmjit::x86::Compiler &comp, RealFunction *fn, asmjit::x86::Xmm result)
{
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    ASMJIT_CHECK(comp.invoke(&call, target, asmjit::FuncSignature::build<double, double>()));
    call->setArg(0, result);
    call->setRet(0, result);
    return {};
}

#ifdef _WIN32
static CompileError call_unary(asmjit::x86::Compiler &comp, ComplexFunction *fn, asmjit::x86::Xmm result)
{
    // x64 MSVC ABI: Complex (16 bytes) is passed and returned by pointer
    // Function signature: Complex* fn(Complex* return_slot, const Complex* arg)
    // - RCX = pointer to return slot (caller allocates)
    // - RDX = pointer to argument
    // - Return value: RAX = RCX (pointer to return slot)

    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};

    // Allocate 16 bytes on stack for return value
    asmjit::x86::Mem return_slot = comp.newStack(16, 16);

    // Allocate 16 bytes on stack for argument
    asmjit::x86::Mem arg_slot = comp.newStack(16, 16);

    // Store result XMM register to argument slot (both low and high parts)
    ASMJIT_CHECK(comp.movlpd(arg_slot, result));
    asmjit::x86::Mem arg_slot_high = arg_slot.cloneAdjusted(8);
    ASMJIT_CHECK(comp.movhpd(arg_slot_high, result));

    // Get addresses of the stack slots
    asmjit::x86::Gp return_ptr = comp.newIntPtr();
    asmjit::x86::Gp arg_ptr = comp.newIntPtr();
    ASMJIT_CHECK(comp.lea(return_ptr, return_slot));
    ASMJIT_CHECK(comp.lea(arg_ptr, arg_slot));

    // Create invoke node with signature: void* fn(void*, const void*)
    asmjit::InvokeNode *invoke_node;
    ASMJIT_CHECK(comp.invoke(&invoke_node, target, asmjit::FuncSignature::build<void *, void *, const void *>()));

    // Set arguments: RCX = return_slot address, RDX = arg_slot address
    invoke_node->setArg(0, return_ptr);
    invoke_node->setArg(1, arg_ptr);

    // Load return value from return_slot back into result XMM register (both parts)
    ASMJIT_CHECK(comp.movlpd(result, return_slot));
    asmjit::x86::Mem return_slot_high = return_slot.cloneAdjusted(8);
    ASMJIT_CHECK(comp.movhpd(result, return_slot_high));

    return {};
}
#else
static CompileError call_unary(asmjit::x86::Compiler &comp, ComplexFunction *fn, asmjit::x86::Xmm result)
{
    // System V AMD64 ABI: Complex (16 bytes) is passed and returned in XMM registers
    // Function signature: Complex fn(Complex arg)
    // - XMM0 = low 64 bits (real part)
    // - XMM1 = high 64 bits (imaginary part)
    // - Return: XMM0 = real part, XMM1 = imaginary part

    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};

    // Split the input complex number into two XMM registers
    asmjit::x86::Xmm arg_real = comp.newXmmSd(); // XMM register for real part
    asmjit::x86::Xmm arg_imag = comp.newXmmSd(); // XMM register for imaginary part

    // Extract real and imaginary parts from result
    ASMJIT_CHECK(comp.movsd(arg_real, result));       // arg_real = result.low (real part)
    ASMJIT_CHECK(comp.movapd(arg_imag, result));      // arg_imag = result (both parts)
    ASMJIT_CHECK(comp.shufpd(arg_imag, arg_imag, 1)); // arg_imag = result.high (imaginary part)

    // Create invoke node with signature: void fn(double, double) ? (double, double)
    asmjit::InvokeNode *invoke_node;
    ASMJIT_CHECK(comp.invoke(&invoke_node, target, asmjit::FuncSignature::build<void, double, double>()));

    // Set arguments: arg0 (XMM0) = real, arg1 (XMM1) = imaginary
    invoke_node->setArg(0, arg_real);
    invoke_node->setArg(1, arg_imag);

    // Get return values: ret0 (XMM0) = real, ret1 (XMM1) = imaginary
    asmjit::x86::Xmm ret_real = comp.newXmmSd();
    asmjit::x86::Xmm ret_imag = comp.newXmmSd();
    invoke_node->setRet(0, ret_real);
    invoke_node->setRet(1, ret_imag);

    // Combine the two return values into result XMM register
    ASMJIT_CHECK(comp.movsd(result, ret_real));    // result.low = ret_real
    ASMJIT_CHECK(comp.unpcklpd(result, ret_imag)); // result.high = ret_imag

    return {};
}
#endif

using ComplexBinOp = Complex(const Complex &lhs, const Complex &rhs);

#ifdef _WIN32
static CompileError call_binary(asmjit::x86::Compiler &comp, ComplexBinOp *fn, asmjit::x86::Xmm result, asmjit::x86::Xmm right)
{
    // x64 MSVC ABI: Complex (16 bytes) is passed and returned by pointer
    // Function signature: Complex* fn(Complex* return_slot, const Complex* left, const Complex* right)
    // - RCX = pointer to return slot (caller allocates)
    // - RDX = pointer to left argument
    // - R8  = pointer to right argument
    // - Return value: RAX = RCX (pointer to return slot)

    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};

    // Allocate 16 bytes on stack for return value
    asmjit::x86::Mem return_slot = comp.newStack(16, 16);

    // Allocate 16 bytes on stack for left argument
    asmjit::x86::Mem left_slot = comp.newStack(16, 16);

    // Allocate 16 bytes on stack for right argument
    asmjit::x86::Mem right_slot = comp.newStack(16, 16);

    // Store left XMM register (result) to left_slot
    ASMJIT_CHECK(comp.movlpd(left_slot, result));
    asmjit::x86::Mem left_slot_high = left_slot.cloneAdjusted(8);
    ASMJIT_CHECK(comp.movhpd(left_slot_high, result));

    // Store right XMM register to right_slot
    ASMJIT_CHECK(comp.movlpd(right_slot, right));
    asmjit::x86::Mem right_slot_high = right_slot.cloneAdjusted(8);
    ASMJIT_CHECK(comp.movhpd(right_slot_high, right));

    // Get addresses of the stack slots
    asmjit::x86::Gp return_ptr = comp.newIntPtr();
    asmjit::x86::Gp left_ptr = comp.newIntPtr();
    asmjit::x86::Gp right_ptr = comp.newIntPtr();
    ASMJIT_CHECK(comp.lea(return_ptr, return_slot));
    ASMJIT_CHECK(comp.lea(left_ptr, left_slot));
    ASMJIT_CHECK(comp.lea(right_ptr, right_slot));

    // Create invoke node with signature: void* fn(void*, const void*, const void*)
    asmjit::InvokeNode *invoke_node;
    ASMJIT_CHECK(
        comp.invoke(&invoke_node, target, asmjit::FuncSignature::build<void *, void *, const void *, const void *>()));

    // Set arguments: RCX = return_slot, RDX = left_slot, R8 = right_slot
    invoke_node->setArg(0, return_ptr);
    invoke_node->setArg(1, left_ptr);
    invoke_node->setArg(2, right_ptr);

    // Load return value from return_slot back into result XMM register
    ASMJIT_CHECK(comp.movlpd(result, return_slot));
    asmjit::x86::Mem return_slot_high = return_slot.cloneAdjusted(8);
    ASMJIT_CHECK(comp.movhpd(result, return_slot_high));

    return {};
}
#else
// Linux version
static CompileError call_binary(asmjit::x86::Compiler &comp, ComplexBinOp *fn, asmjit::x86::Xmm result, asmjit::x86::Xmm right)
{
    // System V AMD64 ABI: Complex (16 bytes) is passed and returned in XMM registers
    // Function signature: Complex fn(Complex left, Complex right)
    // - Left: XMM0 = real, XMM1 = imaginary
    // - Right: XMM2 = real, XMM3 = imaginary
    // - Return: XMM0 = real, XMM1 = imaginary

    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};

    // Save the result and right XMM registers if needed
    asmjit::x86::Mem result_save_slot = comp.newStack(16, 16);
    ASMJIT_CHECK(comp.movdqa(result_save_slot, result));

    // Split the input complex numbers into separate XMM registers
    asmjit::x86::Xmm left_real = comp.newXmmSd();
    asmjit::x86::Xmm left_imag = comp.newXmmSd();
    asmjit::x86::Xmm right_real = comp.newXmmSd();
    asmjit::x86::Xmm right_imag = comp.newXmmSd();

    // Extract left operand (result) parts
    ASMJIT_CHECK(comp.movsd(left_real, result));        // left_real = result.low
    ASMJIT_CHECK(comp.movapd(left_imag, result));       // left_imag = result
    ASMJIT_CHECK(comp.shufpd(left_imag, left_imag, 1)); // left_imag = result.high

    // Extract right operand parts
    ASMJIT_CHECK(comp.movsd(right_real, right));          // right_real = right.low
    ASMJIT_CHECK(comp.movapd(right_imag, right));         // right_imag = right
    ASMJIT_CHECK(comp.shufpd(right_imag, right_imag, 1)); // right_imag = right.high

    // Save right register for later if needed
    asmjit::x86::Mem right_save_slot = comp.newStack(16, 16);
    ASMJIT_CHECK(comp.movdqa(right_save_slot, right));

    // Create invoke node with signature: void fn(double, double, double, double) ? (double, double)
    asmjit::InvokeNode *invoke_node;
    ASMJIT_CHECK(
        comp.invoke(&invoke_node, target, asmjit::FuncSignature::build<void, double, double, double, double>()));

    // Set arguments in order: left_real (XMM0), left_imag (XMM1), right_real (XMM2), right_imag (XMM3)
    invoke_node->setArg(0, left_real);
    invoke_node->setArg(1, left_imag);
    invoke_node->setArg(2, right_real);
    invoke_node->setArg(3, right_imag);

    // Allocate stack slots to save return values
    asmjit::x86::Mem ret_save_slot = comp.newStack(16, 16);

    // Get return values: ret0 (XMM0) = real, ret1 (XMM1) = imaginary
    asmjit::x86::Xmm ret_real = comp.newXmmSd();
    asmjit::x86::Xmm ret_imag = comp.newXmmSd();
    invoke_node->setRet(0, ret_real);
    invoke_node->setRet(1, ret_imag);

    // Save return values to stack slot
    ASMJIT_CHECK(comp.movsd(ret_save_slot, ret_real));
    asmjit::x86::Mem ret_save_slot_high = ret_save_slot.cloneAdjusted(8);
    ASMJIT_CHECK(comp.movsd(ret_save_slot_high, ret_imag));

    // Load return value from stack back into result XMM register
    ASMJIT_CHECK(comp.movsd(result, ret_save_slot));
    ASMJIT_CHECK(comp.movhpd(result, ret_save_slot_high));

    return {};
}
#endif

static CompileError call(
    asmjit::x86::Compiler &comp, double (*fn)(double, double), asmjit::x86::Xmm result, asmjit::x86::Xmm right)
{
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    ASMJIT_CHECK(comp.invoke(&call, target, asmjit::FuncSignature::build<double, double, double>()));
    call->setArg(0, result);
    call->setArg(1, right);
    call->setRet(0, result);
    return {};
}

void Compiler::visit(const FunctionCallNode &node)
{
    node.arg()->visit(*this);
    if (!success())
    {
        return;
    }
    const std::string &name{node.name()};
    if (name == "conj")
    {
        asmjit::x86::Xmm xmm1{comp.newXmm()};
        ASMJIT_STORE(comp.xorpd(xmm1, xmm1));                // xmm1 = 0.0
        ASMJIT_STORE(comp.subpd(xmm1, m_result.back()));     // xmm1 -= result       [-re, -im]
        ASMJIT_STORE(comp.shufpd(m_result.back(), xmm1, 2)); // result.y = xmm1.y    [re, -im]
        return;
    }
    if (name == "flip")
    {
        ASMJIT_STORE(comp.shufpd(m_result.back(), m_result.back(), 1)); // result = result.yx
        return;
    }
    if (name == "ident")
    {
        // identity does nothing
        return;
    }
    if (ComplexFunction *fn = lookup_complex(name))
    {
        if (const CompileError err = call_unary(comp, fn, m_result.back()); err)
        {
            m_err = err;
        }
    }
}

void Compiler::visit(const UnaryOpNode &node)
{
    const char op{node.op()};
    if (op == '+')
    {
        node.operand()->visit(*this);
    }
    else if (op == '-')
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        compile_operand(*node.operand(), operand);
        if (!success())
        {
            return;
        }
        asmjit::x86::Xmm tmp = comp.newXmm();
        ASMJIT_STORE(comp.xorpd(tmp, tmp));              // tmp = 0.0          [0.0, 0.0]
        ASMJIT_STORE(comp.subpd(tmp, operand));          // tmp -= operand     [-re, -im]      negate operand
        ASMJIT_STORE(comp.movapd(m_result.back(), tmp)); // result = tmp       [-re, -im]
    }
    else if (op == '|') // modulus operator |x + yi| returns x^2 + y^2
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        compile_operand(*node.operand(), operand);
        if (!success())
        {
            return;
        }
        ASMJIT_STORE(comp.mulpd(operand, operand));                 // op *= op           [x^2, y^2]
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back())); // result = 0         [0.0, 0.0]
        ASMJIT_STORE(comp.movsd(m_result.back(), operand));         // result.x = op.x    [x^2, 0.0]
        ASMJIT_STORE(comp.shufpd(operand, operand, 1));             // op = op.yx         [y^2, x^2]
        ASMJIT_STORE(comp.addsd(m_result.back(), operand));         // result.x += op.x   [x^2 + y^2, 0.0]
    }
    else
    {
        m_err = asmjit::kErrorInvalidArgument;
    }
}

void Compiler::visit(const BinaryOpNode &node)
{
    node.left()->visit(*this);
    asmjit::Label skip_right = comp.newLabel();
    asmjit::x86::Xmm right{comp.newXmm()};
    const std::string &op{node.op()};
    if (op == "&&")
    {
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_STORE(comp.xorpd(zero, zero));              // zero = 0.0
        ASMJIT_STORE(comp.ucomisd(m_result.back(), zero)); // result.re <=> 0.0?
        asmjit::Label eval_right = comp.newLabel();
        ASMJIT_STORE(comp.jne(eval_right)); // result.re != 0.0, evaluate right
        // Left is false (re == 0.0), short-circuit: result is 0.0, skip everything
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back())); // result = {0.0, 0.0}
        ASMJIT_STORE(comp.jmp(skip_right));
        ASMJIT_STORE(comp.bind(eval_right));
        // Left is true, continue to evaluate right (result unchanged)
    }
    else if (op == "||")
    {
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_STORE(comp.xorpd(zero, zero));              // zero = 0.0
        ASMJIT_STORE(comp.ucomisd(m_result.back(), zero)); // result.re <=> 0.0?
        asmjit::Label eval_right = comp.newLabel();
        ASMJIT_STORE(comp.je(eval_right)); // result.re == 0.0, evaluate right
        // Left is true (re != 0.0), short-circuit: result is 1.0, skip everything
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back())); // Zero result first
        ASMJIT_STORE(comp.movsd(m_result.back(), asmjit::x86::ptr(one)));
        ASMJIT_STORE(comp.jmp(skip_right));
        ASMJIT_STORE(comp.bind(eval_right));
        // Left is false, continue to evaluate right (result unchanged)
    }
    compile_operand(*node.right(), right);
    comp.bind(skip_right);
    if (op == "+")
    {
        comp.addpd(m_result.back(), right); // result += right
        return;
    }
    if (op == "-")
    {
        comp.subpd(m_result.back(), right); // result -= right
        return;
    }
    if (op == "*")
    {
        // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
        asmjit::x86::Xmm xmm0{m_result.back()}; // xmm0 = [a, b]
        asmjit::x86::Xmm xmm1{right};           // xmm1 = [c, d]
        asmjit::x86::Xmm xmm2{comp.newXmm()};   //
        asmjit::x86::Xmm xmm3{comp.newXmm()};   //
        comp.movapd(xmm2, xmm0);                // xmm2 = xmm0            [a, b]
        comp.mulpd(xmm2, xmm1);                 // xmm2 *= xmm1           [ac, bd]
        comp.movapd(xmm3, xmm1);                // xmm3 = xmm1            [c, d]
        comp.shufpd(xmm3, xmm3, 1);             // xmm3 = xmm3.yx         [d, c]
        comp.mulpd(xmm3, xmm0);                 // xmm3 *= xmm0           [ad, bc]
        comp.movapd(xmm0, xmm2);                // xmm0 = xmm2            [ac, bd]
        comp.shufpd(xmm2, xmm2, 1);             // xmm2 = xmm2.yx         [bd, ac]
        comp.subsd(xmm0, xmm2);                 // xmm0.x -= xmm2.x       [ac - bd, bd]
        comp.movapd(xmm1, xmm3);                // xmm1 = xmm3            [ad, bc]
        comp.shufpd(xmm3, xmm3, 1);             // xmm3 = xmm3.yx         [bc, ad]
        comp.addsd(xmm1, xmm3);                 // xmm1.x += xmm3.x       [ad + bc, ad]
        comp.unpcklpd(xmm0, xmm1);              // xmm0 = xmm0.x, xmm1.x  [ac - bd, ad + bc]
        return;
    }
    if (op == "/")
    {
        // (u + vi) / (x + yi) = ((ux + vy) + (vx - uy)i) / (x^2 + y^2)
        // (1 + 2i) / (3 + 4i) = ((1*3 + 2*4) + (2*3 - 1*4)i) / (3^2 + 4^2)
        //                     = ((3 + 8) + (6 - 4)i) / (9 + 16)
        //                     = (11 + 2i) / 25
        asmjit::x86::Xmm xmm0{m_result.back()}; // xmm0 = [u, v]
        asmjit::x86::Xmm xmm1{right};           // xmm1 = [x, y]
        asmjit::x86::Xmm xmm2{comp.newXmm()};   //
        asmjit::x86::Xmm xmm3{comp.newXmm()};   //
        asmjit::x86::Xmm xmm4{comp.newXmm()};   //
        comp.movapd(xmm2, xmm1);                // xmm2 = [x, y]
        comp.mulpd(xmm2, xmm1);                 // xmm2 *= xmm1      [x^2, y^2]              squares
        comp.movapd(xmm3, xmm2);                // xmm3 = xmm2       [x^2, y^2]
        comp.shufpd(xmm3, xmm3, 1);             // xmm3 = xmm2.yx    [y^2, x^2]              swap lanes
        comp.addpd(xmm2, xmm3);                 // xmm2 += xmm3      [x^2 + y^2, x^2 + y^2]  denominator in both lanes
        comp.movapd(xmm3, xmm0);                // xmm3 = xmm0       [u, v]
        comp.mulpd(xmm3, xmm1);                 // xmm3 *= xmm1      [ux, vy]              real part products
        comp.shufpd(xmm0, xmm0, 1);             // xmm0 = xmm0.yx    [v, u]                  swap lanes
        comp.movapd(xmm4, xmm1);                // xmm4 = xmm1       [x, y]
        comp.mulpd(xmm4, xmm0);                 // xmm4 *= xmm0      [vx, uy]              imaginary part products
                                                //
                                                // at this point:
                                                //   xmm0 = [v, u]
                                                //   xmm1 = [x, y]
                                                //   xmm2 = [x^2 + y^2, x^2 + y^2]           real, imaginary denominator
                                                //   xmm3 = [ux, vy]                         real part products
                                                //   xmm4 = [vx, uy]                         imaginary part products
                                                //
        comp.movapd(xmm0, xmm3);                // xmm0 = xmm3       [ux, vy]
        comp.shufpd(xmm0, xmm0, 1);             // xmm0 = xmm0.yx    [vy, ux]                swap lanes
        comp.addsd(xmm0, xmm3);                 // xmm0.x += xmm3.x  [ux + vy, vy]           add real parts
                                                //                                           xmm0.x is numerator real
        comp.movapd(xmm1, xmm4);                // xmm1 = xmm4       [vx, uy]
        comp.shufpd(xmm1, xmm1, 1);             // xmm1 = xmm1.yx    [uy, vx]                swap lanes
        comp.movapd(xmm3, xmm4);                // xmm3 = xmm4       [vx, uy]
        comp.subsd(xmm4, xmm1);                 // xmm4.x -= xmm1.x  [vx - uy, uy]           xmm4.x is numerator imag
        comp.unpcklpd(xmm0, xmm4);              // xmm0.y = xmm4.x   [ux + vy, vx - uy]      swizzle lanes
        comp.divpd(xmm0, xmm2);                 // xmm0 /= xmm2      [ux + vy, vx - uy]/(x^2 + y^2)
        return;
    }
    if (op == "^")
    {
        if (const CompileError err = call_binary(comp, pow, m_result.back(), right); err)
        {
            m_err = err;
        }
        return;
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=")
    {
        // Compare left and right, set result to 1.0 on true, else 0.0
        asmjit::Label success = comp.newLabel();
        asmjit::Label fail = comp.newLabel();
        asmjit::Label end = comp.newLabel();

        if (op == "==" || op == "!=")
        {
            // For equality operators, compare both real and imaginary parts
            asmjit::x86::Xmm left_copy = comp.newXmm();
            asmjit::x86::Xmm right_copy = comp.newXmm();

            // Compare real parts (low 64 bits)
            ASMJIT_STORE(comp.ucomisd(m_result.back(), right)); // compare real parts
            if (op == "==")
            {
                ASMJIT_STORE(comp.jne(fail)); // If real parts differ, not equal (jump to return 0.0)
            }
            else // op == "!="
            {
                ASMJIT_STORE(comp.jne(success)); // If real parts differ, not equal (jump to return 1.0)
            }

            // Real parts match (for ==) or are equal (for !=), now compare imaginary parts
            ASMJIT_STORE(comp.movapd(left_copy, m_result.back())); // Copy left operand
            ASMJIT_STORE(comp.movapd(right_copy, right));          // Copy right operand
            ASMJIT_STORE(comp.shufpd(left_copy, left_copy, 1));    // Move high to low (imaginary part)
            ASMJIT_STORE(comp.shufpd(right_copy, right_copy, 1));  // Move high to low (imaginary part)
            ASMJIT_STORE(comp.ucomisd(left_copy, right_copy));     // Compare imaginary parts

            if (op == "==")
            {
                ASMJIT_STORE(comp.je(success)); // Both parts match -> equal (return 1.0)
                // Fall through to fail
            }
            else // op == "!="
            {
                ASMJIT_STORE(comp.jne(success)); // Imaginary parts differ -> not equal (return 1.0)
                // Fall through to fail
            }
        }
        else
        {
            // For ordering operators, only compare real parts (low 64 bits)
            ASMJIT_STORE(comp.ucomisd(m_result.back(), right)); // xmm0 <=> xmm1
            if (op == "<")
            {
                ASMJIT_STORE(comp.jb(success)); // xmm0 < xmm1?
            }
            else if (op == "<=")
            {
                ASMJIT_STORE(comp.jbe(success)); // xmm0 <= xmm1?
            }
            else if (op == ">")
            {
                ASMJIT_STORE(comp.ja(success)); // xmm0 > xmm1?
            }
            else if (op == ">=")
            {
                ASMJIT_STORE(comp.jae(success)); // xmm0 >= xmm1?
            }
        }

        // Condition false: return 0.0
        ASMJIT_STORE(comp.bind(fail));
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back())); // result = 0.0
        ASMJIT_STORE(comp.jmp(end));

        // Condition true: return 1.0
        ASMJIT_STORE(comp.bind(success));
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_STORE(comp.movsd(m_result.back(), asmjit::x86::ptr(one)));

        ASMJIT_STORE(comp.bind(end));
        return;
    }
    if (op == "&&")
    {
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        asmjit::x86::Xmm zero{comp.newXmm()};
        ASMJIT_STORE(comp.xorpd(zero, zero));              // zero = 0.0
        ASMJIT_STORE(comp.ucomisd(m_result.back(), zero)); // result.re <=> 0.0?
        ASMJIT_STORE(comp.je(end));                        // result.re == 0.0, already false
        ASMJIT_STORE(comp.ucomisd(right, zero));           // right.re <=> 0.0?
        ASMJIT_STORE(comp.jne(success));                   // right.re != 0.0, both true
        // Right is false, set result to 0.0
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back()));
        ASMJIT_STORE(comp.jmp(end));
        ASMJIT_STORE(comp.bind(success));
        // Both true, set result to 1.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back())); // Zero imaginary part
        ASMJIT_STORE(comp.movsd(m_result.back(), asmjit::x86::ptr(one)));
        ASMJIT_STORE(comp.bind(end));
        return;
    }
    if (op == "||")
    {
        asmjit::Label success = comp.newLabel();
        asmjit::Label end = comp.newLabel();
        asmjit::x86::Xmm zero{comp.newXmm()};

        ASMJIT_STORE(comp.xorpd(zero, zero));              // zero = 0.0
        ASMJIT_STORE(comp.ucomisd(m_result.back(), zero)); // result.re <=> 0.0?
        ASMJIT_STORE(comp.jne(end));                       // result.re != 0.0, already true
        ASMJIT_STORE(comp.ucomisd(right, zero));           // right.re <=> 0.0?
        ASMJIT_STORE(comp.jne(success));                   // right.re != 0.0, right is true
        // Both false, set result to 0.0
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back()));
        ASMJIT_STORE(comp.jmp(end));
        ASMJIT_STORE(comp.bind(success));
        // Right is true, set result to 1.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_STORE(comp.xorpd(m_result.back(), m_result.back())); // Zero imaginary part
        ASMJIT_STORE(comp.movsd(m_result.back(), asmjit::x86::ptr(one)));
        ASMJIT_STORE(comp.bind(end));
        return;
    }
}

void Compiler::visit(const AssignmentNode &node)
{
    asmjit::Label label = get_symbol_label(comp, state.data.symbols, node.variable());
    node.expression()->visit(*this);
    if (!success())
    {
        return;
    }
    ASMJIT_STORE(comp.movsd(asmjit::x86::ptr(label), m_result.back()));
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
    node.condition()->visit(*this);
    asmjit::x86::Xmm zero{comp.newXmm()};
    ASMJIT_STORE(comp.xorpd(zero, zero));              // xmm = 0.0
    ASMJIT_STORE(comp.ucomisd(m_result.back(), zero)); // result <=> 0.0?
    ASMJIT_STORE(comp.jz(else_label));                 // if result == 0.0, jump to else block
    if (node.has_then_block())
    {
        node.then_block()->visit(*this);
        if (!success())
        {
            return;
        }
    }
    else
    {
        // If no then block, just set result to 1.0
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        ASMJIT_STORE(comp.movsd(m_result.back(), asmjit::x86::ptr(one)));
    }
    ASMJIT_STORE(comp.jmp(end_label)); // Jump over else block
    ASMJIT_STORE(comp.bind(else_label));
    if (node.has_else_block())
    {
        node.else_block()->visit(*this);
        if (!success())
        {
            return;
        }
    }
    else
    {
        // If no else block, set result to 0.0
        ASMJIT_STORE(comp.movsd(m_result.back(), zero));
    }
    ASMJIT_STORE(comp.bind(end_label));
}

void Compiler::compile_operand(const Node &node, asmjit::x86::Xmm operand)
{
    m_result.push_back(operand);
    node.visit(*this);
    assert(m_result.back() == operand);
    m_result.pop_back();
}

CompileError compile(
    const std::shared_ptr<Node> &expr, asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result)
{
    CompileError err;
    Compiler compiler(comp, state, result, err);
    expr->visit(compiler);
    return err;
}

} // namespace formula::ast
