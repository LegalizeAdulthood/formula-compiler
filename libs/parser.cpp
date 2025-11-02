// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "parser.h"

#include <formula/Node.h>

#include <boost/parser/parser.hpp>

#include <memory>

using namespace formula::ast;
using namespace boost::parser;           // for double_, char_, lit_, etc.
using namespace boost::parser::literals; // for operator""_l, _attr, etc.

namespace formula
{

namespace
{

const auto make_number = [](auto &ctx)
{
    return std::make_shared<NumberNode>(_attr(ctx));
};

const auto make_identifier = [](auto &ctx)
{
    return std::make_shared<IdentifierNode>(_attr(ctx));
};

const auto make_function_call = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    return std::make_shared<FunctionCallNode>(std::get<0>(attr), std::get<1>(attr));
};

const auto make_unary_op = [](auto &ctx)
{
    return std::make_shared<UnaryOpNode>(std::get<0>(_attr(ctx)), std::get<1>(_attr(ctx)));
};

const auto make_binary_op_seq = [](auto &ctx)
{
    auto left = std::get<0>(_attr(ctx));
    for (const auto &op : std::get<1>(_attr(ctx)))
    {
        left = std::make_shared<BinaryOpNode>(left, std::get<0>(op), std::get<1>(op));
    }
    return left;
};

const auto make_assign = [](auto &ctx)
{
    auto variable_seq = std::get<0>(_attr(ctx));
    auto rhs = std::get<1>(_attr(ctx));
    for (const auto &variable : variable_seq)
    {
        rhs = std::make_shared<AssignmentNode>(variable, rhs);
    }
    return rhs;
};

const auto make_statement_seq = [](auto &ctx)
{
    return std::make_shared<StatementSeqNode>(_attr(ctx));
};

const auto make_if_statement = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    return std::make_shared<IfStatementNode>(std::get<0>(attr), std::get<1>(attr), std::get<2>(attr));
};

void split_iterate_bailout(FormulaSections &result, const Expr &expr)
{
    if (const auto *seq = dynamic_cast<StatementSeqNode *>(expr.get()); seq)
    {
        if (seq->statements().size() > 1)
        {
            std::vector<Expr> statements = seq->statements();
            result.bailout = statements.back();
            statements.pop_back();
            result.iterate = std::make_shared<StatementSeqNode>(statements);
        }
        else
        {
            result.bailout = expr;
        }
    }
    else
    {
        result.bailout = expr;
    }
}

const auto make_init_formula = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    FormulaSections result{};
    result.initialize = std::get<0>(attr);
    split_iterate_bailout(result, std::get<1>(attr));
    return result;
};

const auto make_simple_formula = [](auto &ctx)
{
    FormulaSections result{};
    split_iterate_bailout(result, _attr(ctx));
    return result;
};

template <size_t N, typename T>
Expr attr_or_null(T &ctx)
{
    const auto &attr{std::get<N>(_attr(ctx))};
    return attr ? attr.value() : nullptr;
}

const auto make_section_formula = [](auto &ctx)
{
    return FormulaSections{attr_or_null<0>(ctx), attr_or_null<1>(ctx), //
        attr_or_null<2>(ctx), attr_or_null<3>(ctx), attr_or_null<4>(ctx),   //
        attr_or_null<5>(ctx), attr_or_null<6>(ctx),                         //
        attr_or_null<7>(ctx), attr_or_null<8>(ctx)};
};

const auto make_setting_single = [](auto &ctx)
{
    const auto &attr = _attr(ctx);
    return std::make_shared<SettingNode>(std::get<0>(attr), std::get<1>(attr));
};

const auto make_setting_complex = [](auto &ctx)
{
    const auto &attr = _attr(ctx);
    return std::make_shared<SettingNode>(std::get<0>(attr), Complex{std::get<1>(attr), std::get<2>(attr)});
};

const auto make_setting_enum = [](auto &ctx)
{
    const auto &attr = _attr(ctx);
    return std::make_shared<SettingNode>(std::get<0>(attr), EnumName{std::get<1>(attr)});
};

const auto make_param_block = [](auto &ctx)
{
    const auto &attr{_attr(ctx)};
    return std::make_shared<ParamBlockNode>(std::get<0>(attr), std::get<1>(attr), std::get<2>(attr));
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
const auto user_variable = identifier - reserved_function - reserved_variable - reserved_word - section_name;
const auto rel_op = "<="_p | ">="_p | "<"_p | ">"_p | "=="_p | "!="_p;
const auto logical_op = "&&"_p | "||"_p;
const auto skipper = blank | char_(';') >> *(char_ - eol) | char_('\\') >> eol;
const auto statement_separator = +eol | char_(',');

// Grammar rules
rule<struct NumberTag, Expr> number = "number";
rule<struct IdentifierTag, Expr> variable = "variable";
rule<struct FunctionCallTag, Expr> function_call = "function call";
rule<struct UnaryOpTag, Expr> unary_op = "unary operator";
rule<struct FactorTag, Expr> factor = "additive factor";
rule<struct PowerTag, Expr> power = "exponentiation";
rule<struct TermTag, Expr> term = "multiplicative term";
rule<struct AdditiveTag, Expr> additive = "additive expression";
rule<struct AssignmentTag, Expr> assignment = "assignment statement";
rule<struct ExprTag, Expr> expr = "expression";
rule<struct ComparativeTag, Expr> comparative = "comparative expression";
rule<struct ConjunctiveTag, Expr> conjunctive = "conjunctive expression";
rule<struct IfStatementTag, Expr> if_statement = "if statement";
rule<struct ElseIfStatementTag, Expr> elseif_statement = "elseif statement";
rule<struct ElseBlockTag, Expr> else_block = "else block";
rule<struct StatementTag, Expr> statement = "statement";
rule<struct StatementSequenceTag, Expr> statement_seq = "statement sequence";
rule<struct FormulaPartTag, Expr> formula_part = "formula part";
rule<struct FormulaDefinitionTag, FormulaSections> formula = "formula definition";
rule<struct GlobalSectionTag, Expr> global_section = "global section";
rule<struct BuiltinSectionTag, Expr> builtin_section = "builtin section";
rule<struct InitSectionTag, Expr> init_section = "initialize section";
rule<struct LoopSectionTag, Expr> loop_section = "loop section";
rule<struct BailoutSectionTag, Expr> bailout_section = "bailout section";
rule<struct PerturbInitSectionTag, Expr> perturb_init_section = "perturbinit section";
rule<struct PerturbLoopSectionTag, Expr> perturb_loop_section = "perturbloop section";
rule<struct DefaultSectionTag, Expr> default_section = "default section";
rule<struct SwitchSectionTag, Expr> switch_section = "switch section";
rule<struct SectionTag, FormulaSections> section_formula = "section formula";
rule<struct BuiltinTypeTag, Expr> builtin_type = "builtin type";
rule<struct DefaultSettingTag, Expr> default_setting = "default setting";
rule<struct SettingBoolTag, Expr> setting_bool = "bool setting";
rule<struct SettingDoubleTag, Expr> setting_double = "double setting";
rule<struct SettingComplexTag, Expr> setting_complex = "complex number setting";
rule<struct SettingStringTag, Expr> setting_string = "string setting";
rule<struct SettingIntTag, Expr> setting_int = "int setting";
rule<struct SettingMethodTag, Expr> setting_method = "method setting";
rule<struct SettingPeriodicityTag, Expr> setting_periodicity = "periodicity setting";
rule<struct SettingTextTag, Expr> setting_text = "text setting";
rule<struct SettingRatingTag, Expr> setting_rating = "rating setting";
rule<struct ParamSettingTag, Expr> param_setting = "param setting";
rule<struct DefaultParamBlockTag, Expr> default_param_block = "default param block";

const auto number_def = double_[make_number];
const auto variable_def = (identifier - reserved_function - reserved_word - section_name)[make_identifier];
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
const auto empty_block = attr<Expr>(nullptr);
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
const auto statement_section = *eol >> statement_seq;
const auto global_section_def = lit("global:") >> statement_section;
const auto builtin_type_def = (string("type") >> lit('=') >> (int_(1) | int_(2)))[make_setting_single];
const auto builtin_section_def = lit("builtin:") >> *eol >> builtin_type >> *eol;
const auto init_section_def = lit("init:") >> statement_section;
const auto loop_section_def = lit("loop:") >> statement_section;
const auto bailout_section_def = lit("bailout:") >> statement_section;
const auto perturb_init_section_def = lit("perturbinit:") >> statement_section;
const auto perturb_loop_section_def = lit("perturbloop:") >> statement_section;
const auto setting_bool_def = (string("render") >> '=' >> bool_)[make_setting_single];
const auto setting_double_def = ((string("angle") | string("magn") | string("skew") | string("stretch")) //
    >> '=' >> double_)[make_setting_single];
const auto setting_complex_def =
    (string("center") >> '=' >> '(' >> double_ >> ',' >> double_ >> ')')[make_setting_complex];
const auto setting_string_def = ((string("helpfile") | string("helptopic") | string("title")) //
    >> '=' >> lexeme['"' >> *(char_ - '"') >> '"'])[make_setting_single];
const auto setting_method_def = (string("method") >> '=' //
    >> (string("guessing") | string("multipass") | string("onepass")))[make_setting_enum];
const auto setting_periodicity_def = (string("periodicity") >> '=' //
    >> (int_(0) | int_(1) | int_(2) | int_(3)))[make_setting_single];
const auto setting_int_def = (string("maxiter") >> '=' >> int_)[make_setting_single];
const auto setting_text_def = ((string("perturb") | string("precision")) >> '=' //
    >> lexeme[*(char_ - eol)])[make_setting_single];
const auto setting_rating_def = (string("rating") >> '=' //
    >> (string("recommended") | string("average") | string("notRecommended")))[make_setting_enum];
const auto default_param_block_def = (string("bool") >> lit("param") >> user_variable >> eol //
                                         >> attr(nullptr) >> *eol                            //
                                         >> lit("endparam"))[make_param_block] >>
    *eol;
const auto default_setting_def = setting_bool | setting_double | setting_complex | setting_string //
    | setting_int | setting_text | setting_method | setting_periodicity | setting_rating        //
    | default_param_block;
const auto default_section_def = lit("default:") >> *eol >> default_setting >> *eol;
const auto switch_section_def = lit("switch:") >> statement_section;
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
const auto formula_def =                                            //
    (statement_seq >> lit(':') >> statement_seq)[make_init_formula] //
    | statement_seq[make_simple_formula]                            //
    | &(section_name >> lit(':')) >> section_formula_def;

BOOST_PARSER_DEFINE_RULES(number, variable, function_call, unary_op,            //
    factor, power, term, additive, assignment, expr, comparative, conjunctive,  //
    else_block, elseif_statement, if_statement, statement, statement_seq,       //
    formula_part, formula,                                                      //
    global_section, builtin_section,                                            //
    init_section, loop_section, bailout_section,                                //
    perturb_init_section, perturb_loop_section,                                 //
    default_setting, default_section,                                           //
    switch_section,                                                             //
    builtin_type,                                                               //
    setting_bool, setting_double, setting_complex, setting_string, setting_int, //
    setting_text, setting_method, setting_periodicity, setting_rating,          //
    default_param_block,                                                        //
    section_formula);

bool valid_sections(const FormulaSections &ast)
{
    return ast.builtin ? !(ast.per_image || ast.initialize || ast.iterate || ast.bailout) : true;
}

} // namespace

FormulaSectionsPtr parse(std::string_view text)
{
    FormulaSections ast;

    bool debug{};
    if (auto success = parse(text, formula, skipper, ast, debug ? trace::on : trace::off); success)
    {
        return std::make_shared<FormulaSections>(ast);
    }
    return {};
}

} // namespace formula
