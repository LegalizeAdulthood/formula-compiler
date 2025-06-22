#include "ast.h"

#include "functions.h"

#include <algorithm>

namespace formula::ast
{

asmjit::Label get_constant_label(asmjit::x86::Compiler &comp, ConstantBindings &labels, double value)
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

Complex NumberNode::interpret(SymbolTable &) const
{
    return {m_value, 0.0};
}

bool NumberNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_constant_label(comp, state.data.constants, m_value);
    comp.movq(result, asmjit::x86::ptr(label));
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
    comp.movq(result, asmjit::x86::ptr(label));
    return true;
}

Complex FunctionCallNode::interpret(SymbolTable &symbols) const
{
    return evaluate(m_name, m_arg->interpret(symbols));
}

bool call(asmjit::x86::Compiler &comp, double (*fn)(double), asmjit::x86::Xmm result)
{
    // For exponentiation, we can use the pow intrinsic
    asmjit::InvokeNode *call;
    asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(fn))};
    comp.invoke(&call, target, asmjit::FuncSignature::build<double, double>());
    call->setArg(0, result);
    call->setRet(0, result);
    return true;
}

bool FunctionCallNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    auto fn = lookup_real(m_name);
    if (fn == nullptr)
    {
        return false;
    }

    m_arg->compile(comp, state, result);
    return call(comp, fn, result);
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
        return {std::abs(m_operand->interpret(symbols).re), 0.0};
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
        comp.subsd(tmp, operand); // xmm1 = 0.0 - xmm0
        comp.movsd(result, tmp);  // xmm0 = xmm1
        return true;
    }
    if (m_op == '|')
    {
        asmjit::x86::Xmm operand{comp.newXmm()};
        if (!m_operand->compile(comp, state, operand))
        {
            return false;
        }
        // Use the intrinsic for absolute value
        asmjit::InvokeNode *call;
        asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(static_cast<double (*)(double)>(std::abs)))};
        comp.invoke(&call, target, asmjit::FuncSignature::build<double, double>());
        call->setArg(0, operand);
        call->setRet(0, result);
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
        asmjit::Label one = get_constant_label(comp, state.data.constants, 1.0);
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
        asmjit::Label one = get_constant_label(comp, state.data.constants, 1.0);
        comp.movsd(result, asmjit::x86::ptr(one));
        comp.jmp(skip_right);
        comp.bind(eval_right);
        comp.movsd(result, zero);
    }
    m_right->compile(comp, state, right);
    comp.bind(skip_right);
    if (m_op == "+")
    {
        comp.addsd(result, right);
        return true;
    }
    if (m_op == "-")
    {
        comp.subsd(result, right); // xmm0 = xmm0 - xmm1
        return true;
    }
    if (m_op == "*")
    {
        comp.mulsd(result, right); // xmm0 = xmm0 * xmm1
        return true;
    }
    if (m_op == "/")
    {
        comp.divsd(result, right); // xmm0 = xmm0 / xmm1
        return true;
    }
    if (m_op == "^")
    {
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
        asmjit::Label one = get_constant_label(comp, state.data.constants, 1.0);
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
        asmjit::Label one = get_constant_label(comp, state.data.constants, 1.0);
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
        asmjit::Label one = get_constant_label(comp, state.data.constants, 1.0);
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
        asmjit::Label one = get_constant_label(comp, state.data.constants, 1.0);
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
