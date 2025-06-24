#include "ast.h"

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

asmjit::Label get_constant_label(asmjit::x86::Compiler &comp, ConstantBindings &labels, const Complex &value)
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

asmjit::Label get_symbol_label(asmjit::x86::Compiler &comp, SymbolBindings &labels, std::string name)
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

Complex NumberNode::interpret(SymbolTable & /*symbols*/) const
{
    return {m_value, 0.0};
}

bool NumberNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_constant_label(comp, state.data.constants, {m_value, 0.0});
    comp.movlpd(result, asmjit::x86::ptr(label));
    comp.movhpd(result, asmjit::x86::ptr(label, sizeof(double)));
    return true;
}

Complex IdentifierNode::interpret(SymbolTable &symbols) const
{
    if (const auto &it = symbols.find(m_name); it != symbols.end())
    {
        return it->second;
    }
    return {};
}

bool IdentifierNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label{get_symbol_label(comp, state.data.symbols, m_name)};
    comp.movlpd(result, asmjit::x86::ptr(label));
    comp.movhpd(result, asmjit::x86::ptr(label, sizeof(double)));
    return true;
}

Complex FunctionCallNode::interpret(SymbolTable &symbols) const
{
    return evaluate(m_name, m_arg->interpret(symbols));
}

bool call(asmjit::x86::Compiler &comp, double (*fn)(double), asmjit::x86::Xmm result)
{
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    comp.invoke(&call, target, asmjit::FuncSignature::build<double, double>());
    call->setArg(0, result);
    call->setRet(0, result);
    return true;
}

bool FunctionCallNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    if (m_name == "flip")
    {
        if (!m_arg->compile(comp, state, result))
        {
            return false;
        }
        comp.shufpd(result, result, 1); // result = result.yx
        return true;
    }
    //if (ComplexFunction *fn = lookup_complex(m_name))
    //{
    //    m_arg->compile(comp, state, result);
    //    return call(comp, fn, result);
    //}
    if (RealFunction *fn = lookup_real(m_name))
    {
        if (!m_arg->compile(comp, state, result))
        {
            return false;
        }
        return call(comp, fn, result);
    }
    return false;
}

Complex UnaryOpNode::interpret(SymbolTable &symbols) const
{
    if (m_op == '+')
    {
        return m_operand->interpret(symbols);
    }
    if (m_op == '-')
    {
        return {-m_operand->interpret(symbols).re, 0.0};
    }
    if (m_op == '|')
    {
        Complex value = m_operand->interpret(symbols);
        return {value.re * value.re + value.im * value.im, 0.0};
    }

    throw std::runtime_error(std::string{"Invalid unary prefix operator '"} + m_op + "'");
}

bool UnaryOpNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    if (m_op == '+')
    {
        return m_operand->compile(comp, state, result);
    }
    if (m_op == '-')
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        if (!m_operand->compile(comp, state, operand))
        {
            return false;
        }
        asmjit::x86::Xmm tmp = comp.newXmm();
        comp.xorpd(tmp, tmp);     // xmm1 = 0.0
        comp.subpd(tmp, operand); // xmm1 -= xmm0
        comp.movapd(result, tmp); // xmm0 = xmm1
        return true;
    }
    if (m_op == '|') // modulus operator |x + yi| returns x^2 + y^2
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        if (!m_operand->compile(comp, state, operand))
        {
            return false;
        }
        comp.mulpd(operand, operand);     // op *= op       [x^2, y^2]
        comp.xorpd(result, result);       // result = 0
        comp.movsd(result, operand);      // result = op.x
        comp.shufpd(operand, operand, 1); // op = op.yx     [y^2, y^2]
        comp.addsd(result, operand);      // result += op.x [x^2 + y^2, 0.0]
        return true;
    }

    return false;
}

Complex BinaryOpNode::interpret(SymbolTable &symbols) const
{
    const Complex left = m_left->interpret(symbols);
    const auto bool_result = [](bool condition)
    {
        return Complex{condition ? 1.0 : 0.0, 0.0};
    };
    if (m_op == "&&") // short-circuit AND
    {
        if (left == Complex{0.0, 0.0})
        {
            return {0.0, 0.0};
        }
        return bool_result(m_right->interpret(symbols) != Complex{0.0, 0.0});
    }
    if (m_op == "||") // short-circuit OR
    {
        if (left != Complex{0.0, 0.0})
        {
            return {1.0, 0.0};
        }
        return bool_result(m_right->interpret(symbols) != Complex{0.0, 0.0});
    }
    const Complex right = m_right->interpret(symbols);
    if (m_op == "+")
    {
        return left + right;
    }
    if (m_op == "-")
    {
        return left - right;
    }
    if (m_op == "*")
    {
        return left * right;
    }
    if (m_op == "/")
    {
        return left / right;
    }
    if (m_op == "^")
    {
        return {std::pow(left.re, right.re), 0.0};
    }
    if (m_op == "<")
    {
        return bool_result(left.re < right.re);
    }
    if (m_op == "<=")
    {
        return bool_result(left.re <= right.re);
    }
    if (m_op == ">")
    {
        return bool_result(left.re > right.re);
    }
    if (m_op == ">=")
    {
        return bool_result(left.re >= right.re);
    }
    if (m_op == "==")
    {
        return bool_result(left == right);
    }
    if (m_op == "!=")
    {
        return bool_result(left != right);
    }

    throw std::runtime_error(std::string{"Invalid binary operator '"} + m_op + "'");
}

bool call(asmjit::x86::Compiler &comp, double (*fn)(double, double), asmjit::x86::Xmm result, asmjit::x86::Xmm right)
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

bool BinaryOpNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    m_left->compile(comp, state, result);
    asmjit::Label skip_right = comp.newLabel();
    asmjit::x86::Xmm right{comp.newXmm()};
    if (m_op == "&&")
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
    if (m_op == "||")
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
    m_right->compile(comp, state, right);
    comp.bind(skip_right);
    if (m_op == "+")
    {
        comp.addpd(result, right); // result += right
        return true;
    }
    if (m_op == "-")
    {
        comp.subpd(result, right); // result -= right
        return true;
    }
    if (m_op == "*")
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
        return true;
    }
    if (m_op == "/")
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
        comp.movapd(xmm1, xmm4);              // xmm1 = xmm4       [vx, uy]
        comp.shufpd(xmm1, xmm1, 1);           // xmm1 = xmm1.yx    [uy, vx]                swap lanes
        comp.movapd(xmm3, xmm4);              // xmm3 = xmm4       [vx, uy]
        comp.subsd(xmm4, xmm1);               // xmm4.x -= xmm1.x  [vx - uy, uy]           xmm4.x is imaginary part of numerator
        comp.unpcklpd(xmm0, xmm4);            // xmm0.y = xmm4.x   [ux + vy, vx - uy]      swizzle lanes
        comp.divpd(xmm0, xmm2);               // xmm0 /= xmm2      [ux + vy, vx - uy]/(x^2 + y^2)
        return true;
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
        comp.ucomisd(result, right); // xmm0 <=> xmm1
        if (m_op == "<")
        {
            comp.jb(success); // xmm0 < xmm1?
        }
        else if (m_op == "<=")
        {
            comp.jbe(success); // xmm0 <= xmm1?
        }
        else if (m_op == ">")
        {
            comp.ja(success); // xmm0 > xmm1?
        }
        else if (m_op == ">=")
        {
            comp.jae(success); // xmm0 >= xmm1?
        }
        else if (m_op == "==")
        {
            comp.je(success); // xmm0 == xmm1?
        }
        else if (m_op == "!=")
        {
            comp.jne(success); // xmm0 != xmm1?
        }
        else
        {
            return false; // Unsupported operator
        }
        comp.xorpd(result, result); // result = 0.0
        comp.jmp(end);
        comp.bind(success);
        asmjit::Label one = get_constant_label(comp, state.data.constants, {1.0, 0.0});
        comp.movsd(result, asmjit::x86::ptr(one));
        comp.bind(end);
        return true;
    }
    if (m_op == "&&")
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
        return true;
    }
    if (m_op == "||")
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
        return true;
    }

    return false;
}

Complex AssignmentNode::interpret(SymbolTable &symbols) const
{
    Complex value = m_expression->interpret(symbols);
    symbols[m_variable] = value;
    return value;
}

bool AssignmentNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_symbol_label(comp, state.data.symbols, m_variable);
    if (!m_expression->compile(comp, state, result))
    {
        return false;
    }
    comp.movsd(asmjit::x86::ptr(label), result);
    return true;
}

Complex StatementSeqNode::interpret(SymbolTable &symbols) const
{
    Complex value{};
    for (const auto &statement : m_statements)
    {
        value = statement->interpret(symbols);
    }
    return value;
}

bool StatementSeqNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    return std::all_of(m_statements.begin(), m_statements.end(),
        [&comp, &state, &result](const Expr &statement) { return statement->compile(comp, state, result); });
}

Complex IfStatementNode::interpret(SymbolTable &symbols) const
{
    if (m_condition->interpret(symbols) != Complex{0.0, 0.0})
    {
        return m_then_block ? m_then_block->interpret(symbols) : Complex{1.0, 0.0};
    }
    if (m_else_block)
    {
        return m_else_block->interpret(symbols);
    }

    return {0.0, 0.0};
}

bool IfStatementNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label else_label = comp.newLabel();
    asmjit::Label end_label = comp.newLabel();
    m_condition->compile(comp, state, result);
    asmjit::x86::Xmm zero{comp.newXmm()};
    comp.xorpd(zero, zero);     // xmm = 0.0
    comp.ucomisd(result, zero); // result <=> 0.0?
    comp.jz(else_label);        // if result == 0.0, jump to else block
    if (m_then_block)
    {
        if (!m_then_block->compile(comp, state, result))
        {
            return false;
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
    if (m_else_block)
    {
        if (!m_else_block->compile(comp, state, result))
        {
            return false;
        }
    }
    else
    {
        // If no else block, set result to 0.0
        comp.movsd(result, zero);
    }
    comp.bind(end_label);
    return true;
}

} // namespace formula::ast
