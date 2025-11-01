// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "functions.h"

#include <formula/Compiler.h>
#include <formula/Interpreter.h>
#include <formula/Node.h>

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

const auto make_formula = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    ast::FormulaSections result{};
    result.initialize = std::get<0>(attr);
    result.iterate = std::get<1>(attr);
    result.bailout = std::get<2>(attr);
    return result;
};

const auto make_simple_formula = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    ast::FormulaSections result{};
    result.iterate = attr;
    return result;
};

template <size_t N, typename T>
ast::Expr attr_or_null(T &ctx)
{
    const auto &attr{std::get<N>(_attr(ctx))};
    return attr ? attr.value() : nullptr;
}

const auto make_section_formula = [](auto &ctx)
{
    return ast::FormulaSections{attr_or_null<0>(ctx), attr_or_null<1>(ctx), //
        attr_or_null<2>(ctx), attr_or_null<3>(ctx), attr_or_null<4>(ctx),   //
        attr_or_null<5>(ctx), attr_or_null<6>(ctx),                         //
        attr_or_null<7>(ctx), attr_or_null<8>(ctx)};
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
const auto section_name = lexeme[("global"_l | "builtin"_l               //
                                     | "init"_l | "loop"_l | "bailout"_l //
                                     | "perturbinit"_l | "perturbloop"_l //
                                     | "default"_l | "switch"_l)         //
    >> !alnum];
const auto user_variable = identifier - reserved_function - reserved_variable - reserved_word;
const auto rel_op = "<="_p | ">="_p | "<"_p | ">"_p | "=="_p | "!="_p;
const auto logical_op = "&&"_p | "||"_p;
const auto skipper = blank | char_(';') >> *(char_ - eol) | char_('\\') >> eol;
const auto statement_separator = +eol | char_(',');

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
rule<struct FormulaPartTag, ast::Expr> formula_part = "formula part";
rule<struct FormulaDefinitionTag, ast::FormulaSections> formula = "formula definition";
rule<struct GlobalSectionTag, ast::Expr> global_section = "global section";
rule<struct BuiltinSectionTag, ast::Expr> builtin_section = "builtin section";
rule<struct InitSectionTag, ast::Expr> init_section = "initialize section";
rule<struct LoopSectionTag, ast::Expr> loop_section = "loop section";
rule<struct BailoutSectionTag, ast::Expr> bailout_section = "bailout section";
rule<struct PerturbInitSectionTag, ast::Expr> perturb_init_section = "perturbinit section";
rule<struct PerturbLoopSectionTag, ast::Expr> perturb_loop_section = "perturbloop section";
rule<struct DefaultSectionTag, ast::Expr> default_section = "default section";
rule<struct SwitchSectionTag, ast::Expr> switch_section = "switch section";
rule<struct SectionTag, ast::FormulaSections> section_formula = "section formula";

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
const auto statement_seq_def = (statement % statement_separator)[make_statement_seq] >> *eol;
const auto global_section_def = lit("global:") >> *eol >> statement_seq;
const auto builtin_section_def = lit("builtin:") >> *eol >> statement_seq;
const auto init_section_def = lit("init:") >> *eol >> statement_seq;
const auto loop_section_def = lit("loop:") >> *eol >> statement_seq;
const auto bailout_section_def = lit("bailout:") >> *eol >> statement_seq;
const auto perturb_init_section_def = lit("perturbinit:") >> *eol >> statement_seq;
const auto perturb_loop_section_def = lit("perturbloop:") >> *eol >> statement_seq;
const auto default_section_def = lit("default:") >> *eol >> statement_seq;
const auto switch_section_def = lit("switch:") >> *eol >> statement_seq;
const auto formula_part_def = (statement % +eol)[make_statement_seq] >> *eol;
const auto section_formula_def =     //
    (-global_section_def >>          //
        -builtin_section_def >>      //
        -init_section_def >>         //
        -loop_section_def >>         //
        -bailout_section_def >>      //
        -perturb_init_section_def >> //
        -perturb_loop_section_def >> //
        -default_section_def >>      //
        -switch_section_def)[make_section_formula];
const auto formula_def =                                                               //
    (statement_seq >> lit(':') >> formula_part >> lit(',') >> statement)[make_formula] //
    | statement_seq[make_simple_formula];

BOOST_PARSER_DEFINE_RULES(number, variable, function_call, unary_op,           //
    factor, power, term, additive, assignment, expr, comparative, conjunctive, //
    else_block, elseif_statement, if_statement, statement, statement_seq,      //
    formula_part, formula,                                                     //
    global_section, builtin_section,                                           //
    init_section, loop_section, bailout_section,                               //
    perturb_init_section, perturb_loop_section,                                //
    default_section, switch_section,                                           //
    section_formula);

using Function = double();

class ParsedFormula : public Formula
{
public:
    ParsedFormula(ast::FormulaSections ast) :
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
    const ast::Expr &get_section(Section section) const override;

    Complex interpret(Section part) override;
    bool compile() override;
    Complex run(Section part) override;

private:
    ast::CompileError init_code_holder(asmjit::CodeHolder &code);
    ast::CompileError compile_part(asmjit::x86::Compiler &comp, ast::Expr node, asmjit::Label &label);

    ast::EmitterState m_state;
    ast::FormulaSections m_ast;
    Function *m_initialize{};
    Function *m_iterate{};
    Function *m_bailout{};
    asmjit::JitRuntime m_runtime;
    asmjit::FileLogger m_logger{stdout};
};

const ast::Expr &ParsedFormula::get_section(Section section) const
{
    switch (section)
    {
    case Section::PER_IMAGE:
        return m_ast.per_image;
    case Section::BUILTIN:
        return m_ast.builtin;
    case Section::INITIALIZE:
        return m_ast.initialize;
    case Section::ITERATE:
        return m_ast.iterate;
    case Section::BAILOUT:
        return m_ast.bailout;
    case Section::PERTURB_INITIALIZE:
        return m_ast.perturb_initialize;
    case Section::PERTURB_ITERATE:
        return m_ast.perturb_iterate;
    case Section::DEFAULT:
        return m_ast.defaults;
    case Section::SWITCH:
        return m_ast.type_switch;

    default:
        throw std::runtime_error("Unknown section " + std::to_string(+section));
    }
}

Complex ParsedFormula::interpret(Section part)
{
    switch (part)
    {
    case Section::INITIALIZE:
        return ast::interpret(m_ast.initialize, m_state.symbols);
    case Section::ITERATE:
        return ast::interpret(m_ast.iterate, m_state.symbols);
    case Section::BAILOUT:
        return ast::interpret(m_ast.bailout, m_state.symbols);
    }
    throw std::runtime_error("Invalid part for interpreter");
}

ast::CompileError ParsedFormula::init_code_holder(asmjit::CodeHolder &code)
{
    ASMJIT_CHECK(code.init(m_runtime.environment(), m_runtime.cpuFeatures()));
    code.setLogger(&m_logger);
    if (asmjit::Error err =
            code.newSection(&m_state.data.data, ".data", SIZE_MAX, asmjit::SectionFlags::kNone, sizeof(double), 0))
    {
        std::cerr << "Failed to create data section: " << asmjit::DebugUtils::errorAsString(err) << '\n';
        return err;
    }
    return {};
}

ast::CompileError update_symbols(
    asmjit::x86::Compiler &comp, ast::SymbolTable &symbols, ast::SymbolBindings &bindings, asmjit::x86::Xmm result)
{
    asmjit::x86::Gp tmp{comp.newUIntPtr()};
    {
        double *_result{&symbols["_result"].re};
        auto dest = asmjit::x86::ptr(std::uintptr_t(_result));
        ASMJIT_CHECK(comp.movq(tmp, result));
        ASMJIT_CHECK(comp.mov(dest, tmp));
        dest = asmjit::x86::ptr(std::uintptr_t(_result + 1));
        ASMJIT_CHECK(comp.shufpd(result, result, 1)); // result = result.yx
        ASMJIT_CHECK(comp.movq(tmp, result));
        ASMJIT_CHECK(comp.mov(dest, tmp));
    }
    for (auto &[name, binding] : bindings)
    {
        auto src = asmjit::x86::ptr(binding.label);
        double *real{&symbols[name].re};
        auto dest = asmjit::x86::ptr(std::uintptr_t(real));
        ASMJIT_CHECK(comp.mov(tmp, src));
        ASMJIT_CHECK(comp.mov(dest, tmp));
        src = asmjit::x86::ptr(binding.label, sizeof(double));
        dest = asmjit::x86::ptr(std::uintptr_t(real + 1));
        ASMJIT_CHECK(comp.mov(tmp, src));
        ASMJIT_CHECK(comp.mov(dest, tmp));
    }
    return {};
}

ast::CompileError ParsedFormula::compile_part(asmjit::x86::Compiler &comp, ast::Expr node, asmjit::Label &label)
{
    label = comp.addFunc(asmjit::FuncSignature::build<double>())->label();
    asmjit::x86::Xmm result = comp.newXmmSd();
    if (true)
    {
        if (const ast::CompileError err = ast::compile(node, comp, m_state, result); err)
        {
            std::cerr << "Failed to compile AST\n" << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
            ;
            return err;
        }
    }
    if (const ast::CompileError err = update_symbols(comp, m_state.symbols, m_state.data.symbols, result); err)
    {
        return err;
    }
    ASMJIT_CHECK(comp.ret(result));
    ASMJIT_CHECK(comp.endFunc());
    return {};
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

ast::CompileError emit_data_section(asmjit::x86::Compiler &comp, ast::EmitterState &state)
{
    ASMJIT_CHECK(comp.section(state.data.data));
    for (auto &[name, binding] : state.data.symbols)
    {
        if (binding.bound)
        {
            continue;
        }
        binding.bound = true;
        ASMJIT_CHECK(comp.bind(binding.label));
        if (const auto it = state.symbols.find(name); it != state.symbols.end())
        {
            ASMJIT_CHECK(comp.embedDouble(it->second.re));
            ASMJIT_CHECK(comp.embedDouble(it->second.im));
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
        ASMJIT_CHECK(comp.bind(binding.label));
        ASMJIT_CHECK(comp.embedDouble(value.re));
        ASMJIT_CHECK(comp.embedDouble(value.im));
    }
    return {};
}

bool ParsedFormula::compile()
{
    asmjit::CodeHolder code;
    if (const ast::CompileError err = init_code_holder(code); err)
    {
        std::cerr << "Failed to initialize code holder:\n" << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
        return false;
    }
    asmjit::x86::Compiler comp(&code);

    asmjit::Label init_label{};
    asmjit::Label iterate_label{};
    asmjit::Label bailout_label{};
    const auto do_part = [&, this](const char *name, ast::Expr part, asmjit::Label &label)
    {
        if (!part)
        {
            return true; // Nothing to compile
        }

        if (const ast::CompileError err = compile_part(comp, part, label); err)
        {
            std::cerr << "Failed to compile part " << name << ":\n"
                      << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
            return false;
        }
        return true;
    };
    if (!do_part("initialize", m_ast.initialize, init_label) //
        || !do_part("iterate", m_ast.iterate, iterate_label) //
        || !do_part("bailout", m_ast.bailout, bailout_label))
    {
        return false;
    }
    if (const ast::CompileError err = emit_data_section(comp, m_state); err)
    {
        std::cerr << "Failed to emit data section:\n" << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
        return false;
    }
    ASMJIT_CHECK(comp.finalize());

    char *module{};
    if (const asmjit::Error err = m_runtime.add(&module, &code); err || !module)
    {
        std::cerr << "Failed to add formula:\n" << asmjit::DebugUtils::errorAsString(err) << '\n';
        return false;
    }
    m_initialize = function_cast<Function *>(code, module, init_label);
    m_iterate = function_cast<Function *>(code, module, iterate_label);
    m_bailout = function_cast<Function *>(code, module, bailout_label);

    return true;
}

Complex ParsedFormula::run(Section part)
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
    case Section::INITIALIZE:
        return result(m_initialize);
    case Section::ITERATE:
        return result(m_iterate);
    case Section::BAILOUT:
        return result(m_bailout);
    }
    throw std::runtime_error("Invalid part for run");
}

} // namespace

FormulaPtr parse(std::string_view text)
{
    ast::FormulaSections ast;

    try
    {
        bool debug{};
        if (auto success = parse(text, formula, skipper, ast, debug ? trace::on : trace::off); success)
        {
            if (ast.iterate)
            {
                return std::make_shared<ParsedFormula>(ast);
            }
        }
    }
    catch (const parse_error<std::string_view::const_iterator> &e)
    {
        std::cerr << "Parse error: " << e.what() << '\n';
    }
    return {};
}

} // namespace formula
