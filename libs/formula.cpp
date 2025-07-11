// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "formula/formula.h"

#include "ast.h"
#include "functions.h"

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

using namespace boost::parser;           // for double_, char_, lit_, etc.
using namespace boost::parser::literals; // for operator""_l, _attr, etc.

namespace formula
{

namespace
{

const auto make_number = [](auto &ctx)
{
    return std::make_shared<ast::NumberNode>(_attr(ctx));
};

const auto make_identifier = [](auto &ctx)
{
    return std::make_shared<ast::IdentifierNode>(_attr(ctx));
};

const auto make_function_call = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    return std::make_shared<ast::FunctionCallNode>(std::get<0>(attr), std::get<1>(attr));
};

const auto make_unary_op = [](auto &ctx)
{
    return std::make_shared<ast::UnaryOpNode>(std::get<0>(_attr(ctx)), std::get<1>(_attr(ctx)));
};

const auto make_binary_op_seq = [](auto &ctx)
{
    auto left = std::get<0>(_attr(ctx));
    for (const auto &op : std::get<1>(_attr(ctx)))
    {
        left = std::make_shared<ast::BinaryOpNode>(left, std::get<0>(op), std::get<1>(op));
    }
    return left;
};

const auto make_assign = [](auto &ctx)
{
    auto variable_seq = std::get<0>(_attr(ctx));
    auto rhs = std::get<1>(_attr(ctx));
    for (const auto &variable : variable_seq)
    {
        rhs = std::make_shared<ast::AssignmentNode>(variable, rhs);
    }
    return rhs;
};

const auto make_statement_seq = [](auto &ctx)
{
    return std::make_shared<ast::StatementSeqNode>(_attr(ctx));
};

const auto make_if_statement = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    return std::make_shared<ast::IfStatementNode>(std::get<0>(attr), std::get<1>(attr), std::get<2>(attr));
};

// Terminal parsers
const auto alpha = char_('a', 'z') | char_('A', 'Z');
const auto digit = char_('0', '9');
const auto alnum = alpha | digit | char_('_');
const auto identifier = lexeme[alpha >> *alnum];
const auto reserved_variable = lexeme[                                     //
    ("p1"_l | "p2"_l | "p3"_l | "p4"_l | "p5"_l |                          //
        "pixel"_l | "lastsqr"_l | "rand"_l | "pi"_l | "e"_l |              //
        "maxit"_l | "scrnmax"_l | "scrnpix"_l | "whitesq"_l | "ismand"_l | //
        "center"_l | "magxmag"_l | "rotskew"_l)                            //
    >> !alnum];                                                            //
const auto reserved_function = lexeme[                                     //
    ("sinh"_p | "cosh"_p | "cosxx"_p | "sin"_p | "cos"_p |                 //
        "cotanh"_p | "cotan"_p | "tanh"_p | "tan"_p | "sqrt"_p |           //
        "log"_p | "exp"_p | "abs"_p | "conj"_p | "real"_p |                //
        "imag"_p | "flip"_p | "fn1"_p | "fn2"_p | "fn3"_p |                //
        "fn4"_p | "srand"_p | "asinh"_p | "acosh"_p | "asin"_p |           //
        "acos"_p | "atanh"_p | "atan"_p | "cabs"_p | "sqr"_p |             //
        "floor"_p | "ceil"_p | "trunc"_p | "round"_p | "ident"_p |         //
        "one"_p | "zero"_p)                                                //
    >> !alnum];                                                            //
const auto reserved_word = lexeme[("if"_l | "elseif"_l | "else"_l | "endif"_l) >> !alnum];
const auto user_variable = identifier - reserved_function - reserved_variable - reserved_word;
const auto rel_op = "<="_p | ">="_p | "<"_p | ">"_p | "=="_p | "!="_p;
const auto logical_op = "&&"_p | "||"_p;
const auto skipper = blank | char_(';') >> *(char_ - eol) | char_('\\') >> eol;

// Grammar rules
rule<struct NumberTag, ast::Expr> number = "number";
rule<struct IdentifierTag, ast::Expr> variable = "variable";
rule<struct FunctionCallTag, ast::Expr> function_call = "function call";
rule<struct UnaryOpTag, ast::Expr> unary_op = "unary operator";
rule<struct FactorTag, ast::Expr> factor = "additive factor";
rule<struct PowerTag, ast::Expr> power = "exponentiation";
rule<struct TermTag, ast::Expr> term = "multiplicative term";
rule<struct AdditiveTag, ast::Expr> additive = "additive expression";
rule<struct AssignmentTag, ast::Expr> assignment = "assignment statement";
rule<struct ExprTag, ast::Expr> expr = "expression";
rule<struct ComparativeTag, ast::Expr> comparative = "comparative expression";
rule<struct ConjunctiveTag, ast::Expr> conjunctive = "conjunctive expression";
rule<struct IfStatementTag, ast::Expr> if_statement = "if statement";
rule<struct ElseIfStatementTag, ast::Expr> elseif_statement = "elseif statement";
rule<struct ElseBlockTag, ast::Expr> else_block = "else block";
rule<struct StatementTag, ast::Expr> statement = "statement";
rule<struct StatementSequenceTag, ast::Expr> statement_seq = "statement sequence";
rule<struct FormulaDefinitionTag, ast::FormulaDefinition> formula = "formula definition";

const auto number_def = double_[make_number];
const auto variable_def = (identifier - reserved_function - reserved_word)[make_identifier];
const auto function_call_def = (reserved_function >> '(' >> expr >> ')')[make_function_call];
const auto unary_op_def = (char_("-+") >> factor)[make_unary_op] | (char_('|') >> expr >> '|')[make_unary_op];
const auto factor_def = number | function_call | variable | '(' >> expr >> ')' | unary_op;
const auto power_def = (factor >> *(char_('^') >> factor))[make_binary_op_seq];
const auto term_def = (power >> *(char_("*/") >> power))[make_binary_op_seq];
const auto additive_def = (term >> *(char_("+-") >> term))[make_binary_op_seq];
const auto assignment_def = (+(user_variable >> '=') >> additive)[make_assign];
const auto expr_def = assignment | additive;
const auto comparative_def = (expr >> *(rel_op >> expr))[make_binary_op_seq];
const auto conjunctive_def = (comparative >> *(logical_op >> comparative))[make_binary_op_seq];
const auto condition = '('_l >> conjunctive >> ')' >> +eol;
const auto empty_block = attr<ast::Expr>(nullptr);
const auto block = statement_seq | empty_block;
const auto else_statement =                                                  //
    "else"_l >> +eol                                                         //
    >> block;                                                                //
const auto else_block_def = elseif_statement | else_statement | empty_block; //
const auto elseif_statement_def =                                            //
    ("elseif"_l >> condition                                                 //
        >> block                                                             //
        >> else_block                                                        //
        )[make_if_statement];                                                //
const auto if_statement_def =                                                //
    ("if"_l >> condition                                                     //
        >> block                                                             //
        >> else_block                                                        //
        >> "endif")[make_if_statement];
const auto statement_def = if_statement | conjunctive;
const auto statement_seq_def = (statement % (','_l | +eol))[make_statement_seq] >> *eol;
const auto formula_def = (statement_seq >> lit(':') >> statement_seq >> lit(',') >> statement_seq) //
    | (attr<ast::Expr>(nullptr) >> statement_seq >> attr<ast::Expr>(nullptr));

BOOST_PARSER_DEFINE_RULES(number, variable, function_call, unary_op,           //
    factor, power, term, additive, assignment, expr, comparative, conjunctive, //
    else_block, elseif_statement, if_statement, statement, statement_seq,      //
    formula);

using Function = double();

class ParsedFormula : public Formula
{
public:
    ParsedFormula(ast::FormulaDefinition ast) :
        m_ast(ast)
    {
        m_state.symbols["e"] = {std::exp(1.0), 0.0};
        m_state.symbols["pi"] = {std::atan2(0.0, -1.0), 0.0};
        m_state.symbols["_result"] = {0.0, 0.0};
    }
    ~ParsedFormula() override = default;

    void set_value(std::string_view name, Complex value) override
    {
        m_state.symbols[std::string{name}] = value;
    }
    Complex get_value(std::string_view name) const override
    {
        if (auto it = m_state.symbols.find(std::string{name}); it != m_state.symbols.end())
        {
            return it->second;
        }
        return {};
    }

    Complex interpret(Part part) override;
    bool compile() override;
    Complex run(Part part) override;

private:
    bool init_code_holder(asmjit::CodeHolder &code);
    bool compile_part(asmjit::x86::Compiler &comp, ast::Expr node, asmjit::Label &label);

    ast::EmitterState m_state;
    ast::FormulaDefinition m_ast;
    Function *m_initialize{};
    Function *m_iterate{};
    Function *m_bailout{};
    asmjit::JitRuntime m_runtime;
    asmjit::FileLogger m_logger{stdout};
};

Complex ParsedFormula::interpret(Part part)
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

void update_symbols(
    asmjit::x86::Compiler &comp, ast::SymbolTable &symbols, ast::SymbolBindings &bindings, asmjit::x86::Xmm result)
{
    asmjit::x86::Gp tmp{comp.newUIntPtr()};
    {
        double *_result{&symbols["_result"].re};
        auto dest = asmjit::x86::ptr(std::uintptr_t(_result));
        comp.movq(tmp, result);
        comp.mov(dest, tmp);
        dest = asmjit::x86::ptr(std::uintptr_t(_result + 1));
        comp.shufpd(result, result, 1); // result = result.yx
        comp.movq(tmp,result);
        comp.mov(dest, tmp);
    }
    for (auto &[name, binding] : bindings)
    {
        auto src = asmjit::x86::ptr(binding.label);
        double *real{&symbols[name].re};
        auto dest = asmjit::x86::ptr(std::uintptr_t(real));
        comp.mov(tmp, src);
        comp.mov(dest, tmp);
        src = asmjit::x86::ptr(binding.label, sizeof(double));
        dest = asmjit::x86::ptr(std::uintptr_t(real + 1));
        comp.mov(tmp, src);
        comp.mov(dest, tmp);
    }
}

bool ParsedFormula::compile_part(asmjit::x86::Compiler &comp, ast::Expr node, asmjit::Label &label)
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
    update_symbols(comp, m_state.symbols, m_state.data.symbols, result);
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

void emit_data_section(asmjit::x86::Compiler &comp, ast::EmitterState &state)
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
            comp.embedDouble(it->second.re);
            comp.embedDouble(it->second.im);
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
        comp.embedDouble(value.re);
        comp.embedDouble(value.im);
    }
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

Complex ParsedFormula::run(Part part)
{
    auto result = [this](Function *fn)
    {
        if (fn == nullptr)
        {
            return Complex{0.0, 0.0};
        }
        fn();
        return m_state.symbols["_result"];
    };
    switch (part)
    {
    case INITIALIZE:
        return result(m_initialize);
    case ITERATE:
        return result(m_iterate);
    case BAILOUT:
        return result(m_bailout);
    }
    throw std::runtime_error("Invalid part for run");
}

} // namespace

std::shared_ptr<Formula> parse(std::string_view text)
{
    ast::FormulaDefinition ast;

    try
    {
        bool debug{};
        if (auto success = parse(text, formula, skipper, ast, debug ? trace::on : trace::off); success && ast.iterate)
        {
            return std::make_shared<ParsedFormula>(ast);
        }
    }
    catch (const parse_error<std::string_view::const_iterator> &e)
    {
        std::cerr << "Parse error: " << e.what() << '\n';
    }
    return {};
}

} // namespace formula
