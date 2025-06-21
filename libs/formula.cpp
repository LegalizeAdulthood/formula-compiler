#include "formula/formula.h"

#include <asmjit/core.h>
#include <asmjit/x86.h>
#include <boost/parser/parser.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <variant>

namespace bp = boost::parser;

using namespace boost::parser::literals; // for _l, _attr, etc.

namespace formula
{

namespace
{

struct LabelBinding
{
    asmjit::Label label;
    bool bound;
};

using SymbolTable = std::map<std::string, double>;
using ConstantBindings = std::map<double, LabelBinding>;
using SymbolBindings = std::map<std::string, LabelBinding>;

struct DataSection
{
    asmjit::Section *data{};    // Section for data storage
    ConstantBindings constants; // Map of constants to labels
    SymbolBindings symbols;     // Map of symbols to labels
};

struct EmitterState
{
    SymbolTable symbols;
    DataSection data;
};

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

void emit_data_section(asmjit::x86::Compiler &comp, EmitterState &state)
{
    comp.section(state.data.data);
    for (auto &[name, binding] : state.data.symbols)
    {
        if (binding.bound)
        {
            continue;
        }
        binding.bound = true;
        comp.bind(binding.label);
        if (const auto it = state.symbols.find(name); it != state.symbols.end())
        {
            comp.embedDouble(it->second); // Embed the symbol value in the data section
        }
        else
        {
            throw std::runtime_error("Symbol not found: " + name);
        }
    }
    for (auto &[value, binding] : state.data.constants)
    {
        if (binding.bound)
        {
            continue;
        }
        binding.bound = true;
        comp.bind(binding.label);
        comp.embedDouble(value); // Embed the double value in the data section
    }
}

void update_symbols(asmjit::x86::Compiler &comp, SymbolTable &symbols, SymbolBindings &bindings)
{
    asmjit::x86::Gp tmp{comp.newUIntPtr()};
    for (auto &[name, binding] : bindings)
    {
        comp.mov(tmp, asmjit::x86::ptr(binding.label));
        comp.mov(asmjit::x86::ptr(std::uintptr_t(&symbols[name])), tmp);
    }
}

class Node
{
public:
    virtual ~Node() = default;

    virtual double interpret(SymbolTable &symbols) const = 0;
    virtual bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const = 0;
};

using Expr = std::shared_ptr<Node>;

class NumberNode : public Node
{
public:
    NumberNode(double value) :
        m_value(value)
    {
    }
    ~NumberNode() override = default;

    double interpret(SymbolTable &) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    double m_value{};
};

double NumberNode::interpret(SymbolTable &) const
{
    return m_value;
}

bool NumberNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_constant_label(comp, state.data.constants, m_value);
    comp.movq(result, asmjit::x86::ptr(label));
    return true;
}

const auto make_number = [](auto &ctx)
{
    return std::make_shared<NumberNode>(bp::_attr(ctx));
};

class IdentifierNode : public Node
{
public:
    IdentifierNode(std::string value) :
        m_name(std::move(value))
    {
    }
    ~IdentifierNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::string m_name;
};

double IdentifierNode::interpret(SymbolTable &symbols) const
{
    if (const auto &it = symbols.find(m_name); it != symbols.end())
    {
        return it->second;
    }
    return 0.0;
}

bool IdentifierNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label{get_symbol_label(comp, state.data.symbols, m_name)};
    comp.movq(result, asmjit::x86::ptr(label));
    return true;
}

const auto make_identifier = [](auto &ctx)
{
    return std::make_shared<IdentifierNode>(bp::_attr(ctx));
};

class FunctionCallNode : public Node
{
public:
    FunctionCallNode(std::string name, Expr arg) :
        m_name(std::move(name)),
        m_arg(std::move(arg))
    {
    }
    ~FunctionCallNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::string m_name;
    Expr m_arg;
};

using StandardFunction = double(double);

struct FunctionMap
{
    std::string_view name;
    StandardFunction *fn{};
};

bool operator<(const FunctionMap &entry, const std::string &name)
{
    return entry.name < name;
}

double cabs(double arg)
{
    return std::abs(arg);
}

double cosxx(double arg)
{
    // TODO: this is bogus and needs to be corrected for complex argument z=x+iy
    // cos(x)cosh(y) + isin(x)sinh(y)
    return std::cos(arg)*std::cosh(arg);
}

double conj(double arg)
{
    return -arg;
}

double cotan(double arg)
{
    return std::cos(arg) / std::sin(arg);
}

double cotanh(double arg)
{
    return std::cosh(arg) / std::sinh(arg);
}

double flip(double arg)
{
    return -arg;
}

double fn1(double arg)
{
    return arg;
}

double fn2(double arg)
{
    return arg;
}

double fn3(double arg)
{
    return arg;
}

double fn4(double arg)
{
    return arg;
}

double ident(double arg)
{
    return arg;
}

double imag(double arg)
{
    return -arg;
}

double one(double /*arg*/)
{
    return 1.0;
}

double real(double arg)
{
    return arg;
}

double sqr(double arg)
{
    return arg * arg;
}

double zero(double /*arg*/)
{
    return 0.0;
}

double set_rand(double arg)
{
    std::srand(static_cast<int>(arg));
    return 0.0;
}

const FunctionMap s_standard_functions[]{
    {"abs", std::abs},     //
    {"acos", std::acos},   //
    {"acosh", std::acosh}, //
    {"asin", std::asin},   //
    {"asinh", std::asinh}, //
    {"atan", std::atan},   //
    {"atanh", std::atanh}, //
    {"cabs", cabs},        //
    {"ceil", std::ceil},   //
    {"conj", conj},        //
    {"cos", std::cos},     //
    {"cosh", std::cosh},   //
    {"cosxx", cosxx},      //
    {"cotan", cotan},      //
    {"cotanh", cotanh},    //
    {"exp", std::exp},     //
    {"flip", flip},        //
    {"floor", std::floor}, //
    {"fn1", fn1},          //
    {"fn2", fn2},          //
    {"fn3", fn3},          //
    {"fn4", fn4},          //
    {"ident", ident},      //
    {"imag", imag},        //
    {"log", std::log},     //
    {"one", one},          //
    {"real", real},        //
    {"round", std::round}, //
    {"sin", std::sin},     //
    {"sinh", std::sinh},   //
    {"sqr", sqr},          //
    {"sqrt", std::sqrt},   //
    {"srand", set_rand},   //
    {"tan", std::tan},     //
    {"tanh", std::tanh},   //
    {"trunc", std::trunc}, //
    {"zero", zero},        //
};

double FunctionCallNode::interpret(SymbolTable &symbols) const
{
    if (auto end = std::end(s_standard_functions), it = std::lower_bound(std::begin(s_standard_functions), end, m_name);
        it != end)
    {
        return it->fn(m_arg->interpret(symbols));
    }
    throw std::runtime_error("function '" + m_name + "' not found");
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
    const auto end = std::end(s_standard_functions);
    auto it = std::lower_bound(std::begin(s_standard_functions), end, m_name);
    if (it == end)
    {
        return false;
    }

    m_arg->compile(comp, state, result);
    return call(comp, it->fn, result);
}

const auto make_function_call = [](auto &ctx)
{
    const auto &attr{bp::_attr(ctx)};
    return std::make_shared<FunctionCallNode>(std::get<0>(attr), std::get<1>(attr));
};

class UnaryOpNode : public Node
{
public:
    UnaryOpNode(char op, Expr operand) :
        m_op(op),
        m_operand(std::move(operand))
    {
    }
    ~UnaryOpNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    char m_op;
    Expr m_operand;
};

double UnaryOpNode::interpret(SymbolTable &symbols) const
{
    if (m_op == '+')
    {
        return m_operand->interpret(symbols);
    }
    if (m_op == '-')
    {
        return -m_operand->interpret(symbols);
    }
    if (m_op == '|')
    {
        return std::abs(m_operand->interpret(symbols));
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

const auto make_unary_op = [](auto &ctx)
{
    return std::make_shared<UnaryOpNode>(std::get<0>(bp::_attr(ctx)), std::get<1>(bp::_attr(ctx)));
};

class BinaryOpNode : public Node
{
public:
    BinaryOpNode() = default;
    BinaryOpNode(Expr left, char op, Expr right) :
        m_left(std::move(left)),
        m_op(1, op),
        m_right(std::move(right))
    {
    }
    BinaryOpNode(Expr left, std::string op, Expr right) :
        m_left(std::move(left)),
        m_op(std::move(op)),
        m_right(std::move(right))
    {
    }
    ~BinaryOpNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    Expr m_left;
    std::string m_op;
    Expr m_right;
};

double BinaryOpNode::interpret(SymbolTable &symbols) const
{
    const double left = m_left->interpret(symbols);
    const auto bool_result = [](bool condition)
    {
        return condition ? 1.0 : 0.0;
    };
    if (m_op == "&&") // short-circuit AND
    {
        if (left == 0.0)
        {
            return 0.0;
        }
        return bool_result(m_right->interpret(symbols) != 0.0);
    }
    if (m_op == "||") // short-circuit OR
    {
        if (left != 0.0)
        {
            return 1.0;
        }
        return bool_result(m_right->interpret(symbols) != 0.0);
    }
    const double right = m_right->interpret(symbols);
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
        return std::pow(left, right);
    }
    if (m_op == "<")
    {
        return bool_result(left < right);
    }
    if (m_op == "<=")
    {
        return bool_result(left <= right);
    }
    if (m_op == ">")
    {
        return bool_result(left > right);
    }
    if (m_op == ">=")
    {
        return bool_result(left >= right);
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

const auto make_binary_op_seq = [](auto &ctx)
{
    auto left = std::get<0>(bp::_attr(ctx));
    for (const auto &op : std::get<1>(bp::_attr(ctx)))
    {
        left = std::make_shared<BinaryOpNode>(left, std::get<0>(op), std::get<1>(op));
    }
    return left;
};

class AssignmentNode : public Node
{
public:
    AssignmentNode(std::string variable, Expr expression) :
        m_variable(std::move(variable)),
        m_expression(std::move(expression))
    {
    }
    ~AssignmentNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::string m_variable;
    Expr m_expression;
};

double AssignmentNode::interpret(SymbolTable &symbols) const
{
    double value = m_expression->interpret(symbols);
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

const auto make_assign = [](auto &ctx)
{
    auto variable_seq = std::get<0>(bp::_attr(ctx));
    auto rhs = std::get<1>(bp::_attr(ctx));
    for (const auto &variable : variable_seq)
    {
        rhs = std::make_shared<AssignmentNode>(variable, rhs);
    }
    return rhs;
};

class StatementSeqNode : public Node
{
public:
    StatementSeqNode(std::vector<Expr> statements) :
        m_statements(std::move(statements))
    {
    }
    ~StatementSeqNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::vector<Expr> m_statements;
};

double StatementSeqNode::interpret(SymbolTable &symbols) const
{
    return m_statements.back()->interpret(symbols);
}

bool StatementSeqNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    return m_statements.back()->compile(comp, state, result);
}

const auto make_statement_seq = [](auto &ctx)
{
    return std::make_shared<StatementSeqNode>(bp::_attr(ctx));
};

class IfStatementNode : public Node
{
public:
    IfStatementNode(Expr condition, Expr then) :
        m_condition(condition),
        m_then(then)
    {
    }
    ~IfStatementNode() override = default;

    double interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    Expr m_condition;
    Expr m_then;
};

double IfStatementNode::interpret(SymbolTable &symbols) const
{
    if (m_condition->interpret(symbols) != 0.0)
    {
        return m_then ? m_then->interpret(symbols) : 1.0;
    }

    return 0.0;
}

bool IfStatementNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label condition_label = comp.newLabel();
    asmjit::Label end_label = comp.newLabel();
    m_condition->compile(comp, state, result);
    asmjit::x86::Xmm zero{comp.newXmm()};
    comp.xorpd(zero, zero);     // xmm = 0.0
    comp.ucomisd(result, zero); // result <=> 0.0?
    comp.jz(end_label);         // if result == 0.0, jump to end block
    if (m_then)
    {
        if (!m_then->compile(comp, state, result))
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
    comp.bind(end_label);
    return true;
}

const auto make_if_statement = [](auto &ctx)
{
    const auto &attr{bp::_attr(ctx)};
    return std::make_shared<IfStatementNode>(std::get<0>(attr), std::get<1>(attr));
};

struct FormulaDefinition
{
    Expr initialize;
    Expr iterate;
    Expr bailout;
};

// Terminal parsers
const auto alpha = bp::char_('a', 'z') | bp::char_('A', 'Z');
const auto digit = bp::char_('0', '9');
const auto alnum = alpha | digit | bp::char_('_');
const auto identifier = bp::lexeme[alpha >> *alnum];
const auto reserved_variable = bp::lexeme[                                 //
    ("p1"_l | "p2"_l | "p3"_l | "p4"_l | "p5"_l |                          //
        "pixel"_l | "lastsqr"_l | "rand"_l | "pi"_l | "e"_l |              //
        "maxit"_l | "scrnmax"_l | "scrnpix"_l | "whitesq"_l | "ismand"_l | //
        "center"_l | "magxmag"_l | "rotskew"_l)                            //
    >> !alnum];                                                            //
const auto reserved_function = bp::lexeme[                                 //
    ("sinh"_p | "cosh"_p | "cosxx"_p | "sin"_p | "cos"_p |                 //
        "cotanh"_p | "cotan"_p | "tanh"_p | "tan"_p | "sqrt"_p |           //
        "log"_p | "exp"_p | "abs"_p | "conj"_p | "real"_p |                //
        "imag"_p | "flip"_p | "fn1"_p | "fn2"_p | "fn3"_p |                //
        "fn4"_p | "srand"_p | "asinh"_p | "acosh"_p | "asin"_p |           //
        "acos"_p | "atanh"_p | "atan"_p | "cabs"_p | "sqr"_p |             //
        "floor"_p | "ceil"_p | "trunc"_p | "round"_p | "ident"_p |         //
        "one"_p | "zero"_p)                                                //
    >> !alnum];                                                            //
const auto reserved_word = bp::lexeme[("if"_l | "endif"_l) >> !alnum];
const auto user_variable = identifier - reserved_function - reserved_variable - reserved_word;
const auto rel_op = "<="_p | ">="_p | "<"_p | ">"_p | "=="_p | "!="_p;
const auto logical_op = "&&"_p | "||"_p;
const auto skipper = bp::blank | (bp::char_(';') >> *(bp::char_ - bp::eol));

// Grammar rules
bp::rule<struct NumberTag, Expr> number = "number";
bp::rule<struct IdentifierTag, Expr> variable = "variable";
bp::rule<struct FunctionCallTag, Expr> function_call = "function call";
bp::rule<struct UnaryOpTag, Expr> unary_op = "unary operator";
bp::rule<struct FactorTag, Expr> factor = "additive factor";
bp::rule<struct PowerTag, Expr> power = "exponentiation";
bp::rule<struct TermTag, Expr> term = "multiplicative term";
bp::rule<struct AdditiveTag, Expr> additive = "additive expression";
bp::rule<struct AssignmentTag, Expr> assignment = "assignment statement";
bp::rule<struct ExprTag, Expr> expr = "expression";
bp::rule<struct ComparativeTag, Expr> comparative = "comparative expression";
bp::rule<struct ConjunctiveTag, Expr> conjunctive = "conjunctive expression";
bp::rule<struct IfStatementTag, Expr> if_statement = "if statement";
bp::rule<struct StatementTag, Expr> statement = "statement";
bp::rule<struct StatementSequenceTag, Expr> statement_seq = "statement sequence";
bp::rule<struct FormulaDefinitionTag, FormulaDefinition> formula = "formula definition";

const auto number_def = bp::double_[make_number];
const auto variable_def = (identifier - reserved_function - reserved_word)[make_identifier];
const auto function_call_def = (reserved_function >> '(' >> expr >> ')')[make_function_call];
const auto unary_op_def = (bp::char_("-+") >> factor)[make_unary_op] | (bp::char_('|') >> expr >> '|')[make_unary_op];
const auto factor_def = number | function_call | variable | '(' >> expr >> ')' | unary_op;
const auto power_def = (factor >> *(bp::char_('^') >> factor))[make_binary_op_seq];
const auto term_def = (power >> *(bp::char_("*/") >> power))[make_binary_op_seq];
const auto additive_def = (term >> *(bp::char_("+-") >> term))[make_binary_op_seq];
const auto assignment_def = (+(user_variable >> '=') >> additive)[make_assign];
const auto expr_def = assignment | additive;
const auto comparative_def = (expr >> *(rel_op >> expr))[make_binary_op_seq];
const auto conjunctive_def = (comparative >> *(logical_op >> comparative))[make_binary_op_seq];
const auto empty_body = bp::attr<Expr>(nullptr);
const auto if_condition = "if"_l >> '(' >> conjunctive >> ')' >> +bp::eol;
const auto if_statement_def = (if_condition >> (statement_seq | empty_body) >> "endif")[make_if_statement];
const auto statement_def = if_statement | conjunctive;
const auto statement_seq_def = (statement % +bp::eol)[make_statement_seq] >> *bp::eol;
const auto formula_def = (statement_seq >> bp::lit(':') >> statement_seq >> bp::lit(',') >> statement_seq) //
    | (bp::attr<Expr>(nullptr) >> statement_seq >> bp::attr<Expr>(nullptr));

BOOST_PARSER_DEFINE_RULES(number, variable, function_call, unary_op,           //
    factor, power, term, additive, assignment, expr, comparative, conjunctive, //
    if_statement, statement, statement_seq, formula);

using Function = double();

class ParsedFormula : public Formula
{
public:
    ParsedFormula(FormulaDefinition ast) :
        m_ast(ast)
    {
        m_state.symbols["e"] = std::exp(1.0);
        m_state.symbols["pi"] = std::atan2(0.0, -1.0);
    }
    ~ParsedFormula() override = default;

    void set_value(std::string_view name, double value) override
    {
        m_state.symbols[std::string{name}] = value;
    }
    double get_value(std::string_view name) const override
    {
        if (auto it = m_state.symbols.find(std::string{name}); it != m_state.symbols.end())
        {
            return it->second;
        }
        return 0.0;
    }

    double interpret(Part part) override;
    bool compile() override;
    double run(Part part) override;

private:
    bool init_code_holder(asmjit::CodeHolder &code);
    bool compile_part(asmjit::x86::Compiler &comp, Expr node, asmjit::Label &label);

    EmitterState m_state;
    FormulaDefinition m_ast;
    Function *m_initialize{};
    Function *m_iterate{};
    Function *m_bailout{};
    asmjit::JitRuntime m_runtime;
    asmjit::FileLogger m_logger{stdout};
};

double ParsedFormula::interpret(Part part)
{
    switch (part)
    {
    case INITIALIZE:
        return m_ast.initialize->interpret(m_state.symbols);
    case ITERATE:
        return m_ast.iterate->interpret(m_state.symbols);
    case BAILOUT:
        return m_ast.bailout->interpret(m_state.symbols);
    }
    throw std::runtime_error("Invalid part for interpreter");
}

bool ParsedFormula::init_code_holder(asmjit::CodeHolder &code)
{
    code.init(m_runtime.environment(), m_runtime.cpuFeatures());
    code.setLogger(&m_logger);
    if (asmjit::Error err =
            code.newSection(&m_state.data.data, ".data", SIZE_MAX, asmjit::SectionFlags::kNone, sizeof(double), 0))
    {
        std::cerr << "Failed to create data section: " << asmjit::DebugUtils::errorAsString(err) << '\n';
        return false;
    }
    return true;
}

bool ParsedFormula::compile_part(asmjit::x86::Compiler &comp, Expr node, asmjit::Label &label)
{
    if (!node)
    {
        return true; // Nothing to compile
    }

    label = comp.addFunc(asmjit::FuncSignature::build<double>())->label();
    asmjit::x86::Xmm result = comp.newXmmSd();
    if (!node->compile(comp, m_state, result))
    {
        std::cerr << "Failed to compile AST\n";
        return false;
    }
    update_symbols(comp, m_state.symbols, m_state.data.symbols);
    comp.ret(result);
    comp.endFunc();
    return true;
}

template <typename FunctionPtr>
FunctionPtr function_cast(const asmjit::CodeHolder &code, char *module, const asmjit::Label label)
{
    if (!label.isValid())
    {
        return nullptr;
    }
    const size_t offset{code.labelOffsetFromBase(label)};
    return reinterpret_cast<FunctionPtr>(module + offset);
}

bool ParsedFormula::compile()
{
    asmjit::CodeHolder code;
    if (!init_code_holder(code))
    {
        return false;
    }
    asmjit::x86::Compiler comp(&code);

    asmjit::Label init_label{};
    asmjit::Label iterate_label{};
    asmjit::Label bailout_label{};
    const bool result =                                     //
        compile_part(comp, m_ast.initialize, init_label)    //
        && compile_part(comp, m_ast.iterate, iterate_label) //
        && compile_part(comp, m_ast.bailout, bailout_label);
    if (!result)
    {
        std::cerr << "Failed to compile parts\n";
        return false;
    }
    emit_data_section(comp, m_state);
    comp.finalize();

    char *module{};
    if (const asmjit::Error err = m_runtime.add(&module, &code); err || !module)
    {
        std::cerr << "Failed to compile formula: " << asmjit::DebugUtils::errorAsString(err) << '\n';
        return false;
    }
    m_initialize = function_cast<Function *>(code, module, init_label);
    m_iterate = function_cast<Function *>(code, module, iterate_label);
    m_bailout = function_cast<Function *>(code, module, bailout_label);

    return true;
}

double ParsedFormula::run(Part part)
{
    switch (part)
    {
    case INITIALIZE:
        return m_initialize ? m_initialize() : 0.0;
    case ITERATE:
        return m_iterate ? m_iterate() : 0.0;
    case BAILOUT:
        return m_bailout ? m_bailout() : 0.0;
    }
    throw std::runtime_error("Invalid part for run");
}

} // namespace

std::shared_ptr<Formula> parse(std::string_view text)
{
    FormulaDefinition ast;

    try
    {
        bool debug{};
        if (auto success = bp::parse(text, formula, skipper, ast, debug ? bp::trace::on : bp::trace::off);
            success && ast.iterate)
        {
            return std::make_shared<ParsedFormula>(ast);
        }
    }
    catch (const bp::parse_error<std::string_view::const_iterator> &e)
    {
        std::cerr << "Parse error: " << e.what() << '\n';
    }
    return {};
}

} // namespace formula
