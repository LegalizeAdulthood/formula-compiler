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

namespace formula
{

namespace
{

using SymbolTable = std::map<std::string, double>;
using ConstantLabels = std::map<double, asmjit::Label>;
using SymbolLabels = std::map<std::string, asmjit::Label>;

struct DataSection
{
    asmjit::Section *data{};  // Section for data storage
    ConstantLabels constants; // Map of constants to labels
    SymbolLabels symbols;     // Map of symbols to labels
};

struct EmitterState
{
    SymbolTable symbols;
    DataSection data;
};

asmjit::Label get_constant_label(asmjit::x86::Compiler &comp, ConstantLabels &labels, double value)
{
    if (const auto it = labels.find(value); it != labels.end())
    {
        return it->second;
    }

    // Create a new label for the constant
    asmjit::Label label = comp.newLabel();
    labels[value] = label;
    return label;
}

asmjit::Label get_symbol_label(asmjit::x86::Compiler &comp, SymbolLabels &labels, std::string name)
{
    if (const auto it = labels.find(name); it != labels.end())
    {
        return it->second;
    }

    // Create a new label for the symbol
    asmjit::Label label = comp.newNamedLabel(name.c_str());
    labels[name] = label;
    return label;
}

void emit_data_section(asmjit::x86::Compiler &comp, EmitterState &state)
{
    comp.section(state.data.data);
    for (const auto &[name, label] : state.data.symbols)
    {
        comp.bind(label);
        if (const auto it = state.symbols.find(name); it != state.symbols.end())
        {
            comp.embedDouble(it->second); // Embed the symbol value in the data section
        }
        else
        {
            throw std::runtime_error("Symbol not found: " + name);
        }
    }
    for (const auto &[value, label] : state.data.constants)
    {
        comp.bind(label);
        comp.embedDouble(value); // Embed the double value in the data section
    }
}

class Node
{
public:
    virtual ~Node() = default;

    virtual double interpret(SymbolTable &symbols) = 0;
    virtual bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const = 0;

    virtual double &lvalue(SymbolTable & /*symbols*/)
    {
        throw std::runtime_error("Node does not support lvalue access");
    }
};

class NumberNode : public Node
{
public:
    NumberNode(double value) :
        m_value(value)
    {
    }
    ~NumberNode() override = default;

    double interpret(SymbolTable &) override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    double m_value{};
};

double NumberNode::interpret(SymbolTable &)
{
    return m_value;
}

bool NumberNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label = get_constant_label(comp, state.data.constants, m_value);
    comp.movq(result, asmjit::x86::ptr(label));
    return true;
}

const auto make_number = [](auto &ctx) { return std::make_shared<NumberNode>(bp::_attr(ctx)); };

class IdentifierNode : public Node
{
public:
    IdentifierNode(std::string value) :
        m_name(std::move(value))
    {
    }
    ~IdentifierNode() override = default;

    double interpret(SymbolTable &symbols) override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

    double &lvalue(SymbolTable &symbols) override
    {
        return symbols[m_name];
    }

private:
    std::string m_name;
};

double IdentifierNode::interpret(SymbolTable &symbols)
{
    if (const auto &it = symbols.find(m_name); it != symbols.end())
    {
        return it->second;
    }
    return 0.0;
}

template <typename Emitter>
asmjit::Label get_identifier_label(Emitter &assem, EmitterState &state, const std::string &name)
{
    if (const auto &it = state.symbols.find(name); it != state.symbols.end())
    {
        return get_symbol_label(assem, state.data.symbols, it->first);
    }
    return get_constant_label(assem, state.data.constants, 0.0);
}

bool IdentifierNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    asmjit::Label label{get_identifier_label(comp, state, m_name)};
    comp.movq(result, asmjit::x86::ptr(label));
    return true;
}

const auto make_identifier = [](auto &ctx) { return std::make_shared<IdentifierNode>(bp::_attr(ctx)); };

class UnaryOpNode : public Node
{
public:
    UnaryOpNode(char op, std::shared_ptr<Node> operand) :
        m_op(op),
        m_operand(std::move(operand))
    {
    }
    ~UnaryOpNode() override = default;

    double interpret(SymbolTable &symbols) override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    char m_op;
    std::shared_ptr<Node> m_operand;
};

double UnaryOpNode::interpret(SymbolTable &symbols)
{
    if (m_op == '+')
    {
        return m_operand->interpret(symbols);
    }
    if (m_op == '-')
    {
        return -m_operand->interpret(symbols);
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

    return false;
}

const auto make_unary_op = [](auto &ctx)
{ return std::make_shared<UnaryOpNode>(std::get<0>(bp::_attr(ctx)), std::get<1>(bp::_attr(ctx))); };

class BinaryOpNode : public Node
{
public:
    BinaryOpNode() = default;
    BinaryOpNode(std::shared_ptr<Node> left, char op, std::shared_ptr<Node> right) :
        m_left(std::move(left)),
        m_op(op),
        m_right(std::move(right))
    {
    }
    ~BinaryOpNode() override = default;

    double interpret(SymbolTable &symbols) override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;
    double &lvalue(SymbolTable &symbols) override;

private:
    std::shared_ptr<Node> m_left;
    char m_op;
    std::shared_ptr<Node> m_right;
    double *m_lvalue{};
};

double BinaryOpNode::interpret(SymbolTable &symbols)
{
    if (m_op == '=')
    {
        double &left = lvalue(symbols);
        const double right = m_right->interpret(symbols);
        left = right; // Assignment operation
        return right;
    }

    const double left = m_left->interpret(symbols);
    const double right = m_right->interpret(symbols);
    if (m_op == '+')
    {
        return left + right;
    }
    if (m_op == '-')
    {
        return left - right;
    }
    if (m_op == '*')
    {
        return left * right;
    }
    if (m_op == '/')
    {
        return left / right;
    }
    if (m_op == '^')
    {
        return std::pow(left, right);
    }
    throw std::runtime_error(std::string{"Invalid binary operator '"} + m_op + "'");
}

bool BinaryOpNode::compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const
{
    m_left->compile(comp, state, result);
    asmjit::x86::Xmm right{comp.newXmm()};
    m_right->compile(comp, state, right);
    if (m_op == '+')
    {
        comp.addsd(result, right);
        return true;
    }
    if (m_op == '-')
    {
        comp.subsd(result, right); // xmm0 = xmm0 - xmm1
        return true;
    }
    if (m_op == '*')
    {
        comp.mulsd(result, right); // xmm0 = xmm0 * xmm1
        return true;
    }
    if (m_op == '/')
    {
        comp.divsd(result, right); // xmm0 = xmm0 / xmm1
        return true;
    }
    if (m_op == '^')
    {
        // For exponentiation, we can use the pow intrinsic
        asmjit::InvokeNode *call;
        asmjit::Imm target{asmjit::imm(reinterpret_cast<void *>(static_cast<double (*)(double, double)>(std::pow)))};
        comp.invoke(&call, target, asmjit::FuncSignature::build<double, double, double>());
        call->setArg(0, result);
        call->setArg(1, right);
        call->setRet(0, result);
        return true;
    }
    return false;
}

double &BinaryOpNode::lvalue(SymbolTable &symbols)
{
    if (m_lvalue == nullptr)
    {
        double &left = m_left->lvalue(symbols);
        m_lvalue = &left;
        return left;
    }
    return *m_lvalue;
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

using Expr = std::shared_ptr<Node>;

// Terminal parsers
const auto alpha = bp::char_('a', 'z') | bp::char_('A', 'Z');
const auto digit = bp::char_('0', '9');
const auto alnum = alpha | digit | bp::char_('_');
const auto identifier = bp::lexeme[alpha >> *alnum];

// Grammar rules
bp::rule<struct NumberTag, Expr> number = "number";
bp::rule<struct IdentifierTag, Expr> variable = "variable";
bp::rule<struct ExprTag, Expr> expr = "expression";
bp::rule<struct PowerTag, Expr> power = "exponentiation";
bp::rule<struct TermTag, Expr> term = "multiplicative term";
bp::rule<struct FactorTag, Expr> factor = "additive factor";
bp::rule<struct UnaryOpTag, Expr> unary_op = "unary operator";
bp::rule<struct AssignTag, Expr> assign = "assignment operator";

const auto number_def = bp::double_[make_number];
const auto variable_def = identifier[make_identifier];
const auto unary_op_def = (bp::char_("-+") >> factor)[make_unary_op];
const auto factor_def = number | variable | '(' >> expr >> ')' | unary_op;
const auto power_def = (factor >> *(bp::char_('^') >> factor))[make_binary_op_seq];
const auto term_def = (power >> *(bp::char_("*/") >> power))[make_binary_op_seq];
const auto assign_def = (term >> *(bp::char_("+-") >> term))[make_binary_op_seq];
const auto expr_def = (assign >> *(bp::char_("=") >> assign))[make_binary_op_seq];

BOOST_PARSER_DEFINE_RULES(number, variable, expr, assign, term, power, factor, unary_op);

using Function = double();

class ParsedFormula : public Formula
{
public:
    ParsedFormula(std::shared_ptr<Node> ast) :
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

    double interpret() override;
    bool compile() override;
    double run() override;

private:
    bool init_code_holder(asmjit::CodeHolder &code);

    EmitterState m_state;
    std::shared_ptr<Node> m_ast;
    Function *m_function{};
    asmjit::JitRuntime m_runtime;
    asmjit::FileLogger m_logger{stdout};
};

double ParsedFormula::interpret()
{
    return m_ast->interpret(m_state.symbols);
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

bool ParsedFormula::compile()
{
    asmjit::CodeHolder code;
    if (!init_code_holder(code))
    {
        return false;
    }
    asmjit::x86::Compiler comp(&code);
    comp.addFunc(asmjit::FuncSignature::build<double>());
    asmjit::x86::Xmm result = comp.newXmmSd();
    if (!m_ast->compile(comp, m_state, result))
    {
        std::cerr << "Failed to compile AST\n";
        return false;
    }
    comp.ret(result);
    comp.endFunc();
    emit_data_section(comp, m_state);
    comp.finalize();

    if (const asmjit::Error err = m_runtime.add(&m_function, &code); err || !m_function)
    {
        std::cerr << "Failed to compile formula: " << asmjit::DebugUtils::errorAsString(err) << '\n';
        return false;
    }

    return true;
}

double ParsedFormula::run()
{
    return m_function ? m_function() : 0.0;
}

} // namespace

std::shared_ptr<Formula> parse(std::string_view text)
{
    Expr ast;

    try
    {
        if (auto success = bp::parse(text, expr, bp::ws, ast /*, bp::trace::on*/); success && ast)
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
