// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include <formula/ParseOptions.h>

#include "NodeFormatter.h"
#include "trim_ws.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

using namespace testing;

namespace formula::test
{

namespace
{

struct SimpleExpressionParam
{
    std::string_view name;
    std::string_view text;
    std::string_view expected;
};

inline void PrintTo(const SimpleExpressionParam &param, std::ostream *os)
{
    *os << param.name;
}

class SimpleExpressions : public TestWithParam<SimpleExpressionParam>
{
};

struct MultiStatementParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const MultiStatementParam &param, std::ostream *os)
{
    *os << param.name;
}

class MultiStatements : public TestWithParam<MultiStatementParam>
{
};

class BuiltinVariables : public TestWithParam<std::string>
{
};

class ReservedWords : public TestWithParam<std::string>
{
};

class Functions : public TestWithParam<std::string>
{
};

struct ParseFailureParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const ParseFailureParam &param, std::ostream *os)
{
    *os << param.name;
}

class ParseFailures : public TestWithParam<ParseFailureParam>
{
};

struct ParseParam
{
    std::string_view name;
    std::string_view text;
    Section section;
};

inline void PrintTo(const ParseParam &param, std::ostream *os)
{
    *os << param.name;
}

class SingleSections : public TestWithParam<ParseParam>
{
};

class DefaultSection : public TestWithParam<SimpleExpressionParam>
{
};

struct InvalidSectionParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const InvalidSectionParam &param, std::ostream *os)
{
    *os << param.name;
}

struct BuiltinDisallowsParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const BuiltinDisallowsParam &param, std::ostream *os)
{
    *os << param.name;
}

class BuiltinDisallows : public TestWithParam<BuiltinDisallowsParam>
{
};

} // namespace

static SimpleExpressionParam s_simple_expressions[]{
    {"intLiteral", "1", "literal:1"},                                                                               //
    {"floatLiteral", "1.5", "literal:1.5"},                                                                         //
    {"complexLiteral", "(1,2)", "literal:(1,2)"},                                                                   //
    {"identifier", "z2", "identifier:z2"},                                                                          //
    {"parenExpr", "(z)", "identifier:z"},                                                                           //
    {"add", "1+2", "binary_op:+ literal:1 literal:2"},                                                              //
    {"subtract", "1-2", "binary_op:- literal:1 literal:2"},                                                         //
    {"multiply", "1*2", "binary_op:* literal:1 literal:2"},                                                         //
    {"divide", "1/2", "binary_op:/ literal:1 literal:2"},                                                           //
    {"multiplyAdd", "1*2+4", "binary_op:+ binary_op:* literal:1 literal:2 literal:4"},                              //
    {"parenthesisExpr", "1*(2+4)", "binary_op:* literal:1 binary_op:+ literal:2 literal:4"},                        //
    {"unaryMinus", "-(1)", "unary_op:- literal:1"},                                                                 //
    {"unaryPlus", "+(1)", "unary_op:+ literal:1"},                                                                  //
    {"unaryMinusNegativeOne", "--1", "unary_op:- unary_op:- literal:1"},                                            //
    {"addAddAdd", "1+2+3", "binary_op:+ binary_op:+ literal:1 literal:2 literal:3"},                                //
    {"capitalLetterInIdentifier", "A", "identifier:a"},                                                             //
    {"numberInIdentifier", "a1", "identifier:a1"},                                                                  //
    {"underscoreInIdentifier", "A_1", "identifier:a_1"},                                                            //
    {"power", "2^3", "binary_op:^ literal:2 literal:3"},                                                            //
    {"powerLeftAssociative", "1^2^3", "binary_op:^ binary_op:^ literal:1 literal:2 literal:3"},                     //
    {"assignment", "z=4", "assignment:z literal:4"},                                                                //
    {"assignmentLongVariable", "this_is_another4_variable_name2=4",                                                 //
        "assignment:this_is_another4_variable_name2 literal:4"},                                                    //
    {"modulus", "|-3.0|", "unary_op:| unary_op:- literal:3"},                                                       //
    {"compareLess", "4<3", "binary_op:< literal:4 literal:3"},                                                      //
    {"compareLessPrecedence", "3<z=4", "binary_op:< literal:3 assignment:z literal:4"},                             //
    {"compareLessEqual", "4<=3", "binary_op:<= literal:4 literal:3"},                                               //
    {"compareGreater", "4>3", "binary_op:> literal:4 literal:3"},                                                   //
    {"compareAssociatesLeft", "4>3<4", "binary_op:< binary_op:> literal:4 literal:3 literal:4"},                    //
    {"compareGreaterEqual", "4>=3", "binary_op:>= literal:4 literal:3"},                                            //
    {"compareEqual", "4==3", "binary_op:== literal:4 literal:3"},                                                   //
    {"compareNotEqual", "4!=3", "binary_op:!= literal:4 literal:3"},                                                //
    {"logicalAnd", "4==3&&5==6", "binary_op:&& binary_op:== literal:4 literal:3 binary_op:== literal:5 literal:6"}, //
    {"logicalOr", "4==3||5==6", "binary_op:|| binary_op:== literal:4 literal:3 binary_op:== literal:5 literal:6"},  //
    {"ignoreComments", "3; z=6 oh toodlee doo", "literal:3"},                                                       //
    {"ignoreCommentsLF", "3; z=6 oh toodlee doo\n", "literal:3"},                                                   //
    {"ignoreCommentsCRLF", "3; z=6 oh toodlee doo\r\n", "literal:3"},                                               //
    {"reservedVariablePrefixToUserVariable", "e2=1", "assignment:e2 literal:1"},                                    //
    {"reservedFunctionPrefixToUserVariable", "sine=1", "assignment:sine literal:1"},                                //
    {"reservedKeywordPrefixToUserVariable", "if1=1", "assignment:if1 literal:1"},                                   //
    {"builtinVariableP1", "p1", "identifier:p1"},                                                                   //
    {"builtinVariableP2", "p2", "identifier:p2"},                                                                   //
    {"builtinVariableP3", "p3", "identifier:p3"},                                                                   //
    {"builtinVariableP4", "p4", "identifier:p4"},                                                                   //
    {"builtinVariableP5", "p5", "identifier:p5"},                                                                   //
    {"builtinVariablePixel", "pixel", "identifier:pixel"},                                                          //
    {"builtinVariableLastsqr", "lastsqr", "identifier:lastsqr"},                                                    //
    {"builtinVariableRand", "rand", "identifier:rand"},                                                             //
    {"builtinVariablePi", "pi", "identifier:pi"},                                                                   //
    {"builtinVariableE", "e", "identifier:e"},                                                                      //
    {"builtinVariableMaxit", "maxit", "identifier:maxit"},                                                          //
    {"builtinVariableScrnmax", "scrnmax", "identifier:scrnmax"},                                                    //
    {"builtinVariableScrnpix", "scrnpix", "identifier:scrnpix"},                                                    //
    {"builtinVariableWhitesq", "whitesq", "identifier:whitesq"},                                                    //
    {"builtinVariableIsmand", "ismand", "identifier:ismand"},                                                       //
    {"builtinVariableCenter", "center", "identifier:center"},                                                       //
    {"builtinVariableMagxmag", "magxmag", "identifier:magxmag"},                                                    //
    {"builtinVariableRotskew", "rotskew", "identifier:rotskew"},                                                    //
    {"ifEmptyBody",
        "if(0)\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifBlankLinesBody",
        "if(0)\n"
        "\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifThenBody",
        "if(0)\n"
        "1\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } endif"},
    {"ifThenComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } } endif"},
    {"ifComparisonCondition",
        "if(1<2)\n"
        "endif",
        "if_statement:( binary_op:< literal:1 literal:2 ) { } endif"},
    {"ifConjunctiveCondition",
        "if(1&&2)\n"
        "endif",
        "if_statement:( binary_op:&& literal:1 literal:2 ) { } endif"},
    {"ifElseEmptyBody",
        "if(0)\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifElseBlankLinesBody",
        "if(0)\n"
        "\n"
        "else\n"
        "\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifThenElseBody",
        "if(0)\n"
        "else\n"
        "1\n"
        "endif",
        "if_statement:( literal:0 ) { } else { literal:1 } endif"},
    {"ifThenBodyElse",
        "if(0)\n"
        "1\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } endif"},
    {"ifThenElseComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "else\n"
        "3\n"
        "4\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } "
        "} else { statement_seq:2 { literal:3 literal:4 } } endif"},
    {"ifElseIfEmptyBody",
        "if(0)\n"
        "elseif(1)\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { } else { if_statement:( literal:1 ) { } endif } endif"},
    {"ifMultipleElseIfEmptyBody",
        "if(0)\n"
        "elseif(0)\n"
        "elseif(0)\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { } "
        "endif } "
        "endif } "
        "endif"},
    {"ifElseIfBlankLinesBody",
        "if(0)\n"
        "\n"
        "elseif(0)\n"
        "\n"
        "else\n"
        "\n"
        "endif",
        "if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { } "
        "endif } "
        "endif"},
    {"ifThenElseIfBody",
        "if(0)\n"
        "elseif(0)\n"
        "1\n"
        "endif",
        "if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { literal:1 } "
        "endif } "
        "endif"},
    {"ifThenBodyElseIf",
        "if(0)\n"
        "1\n"
        "elseif(0)\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } "
        "else { if_statement:( literal:0 ) { } "
        "endif } "
        "endif"},
    {"ifThenElseIfComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "elseif(1)\n"
        "3\n"
        "4\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } } "
        "else { if_statement:( literal:1 ) { statement_seq:2 { literal:3 literal:4 } } "
        "endif } "
        "endif"},
    {"chainedElseIf",
        "if(0)\n"
        "1\n"
        "elseif(2)\n"
        "3\n"
        "elseif(4)\n"
        "5\n"
        "elseif(6)\n"
        "7\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } "
        "else { if_statement:( literal:2 ) { literal:3 } "
        "else { if_statement:( literal:4 ) { literal:5 } "
        "else { if_statement:( literal:6 ) { literal:7 } "
        "endif } "
        "endif } "
        "endif } "
        "endif"},
    {"backslashContinuesStatement",
        "if(\\\n"
        "0)\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifSequence",
        "if(0)\n"
        "1,2\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } } endif"},
    {"elseSequence",
        "if(0)\n"
        "else\n"
        "1,2\n"
        "endif",
        "if_statement:( literal:0 ) { } else { statement_seq:2 { literal:1 literal:2 } } endif"},
    {"elseIfSequence",
        "if(0)\n"
        "elseif(3)\n"
        "1,2\n"
        "endif",
        "if_statement:( literal:0 ) { } else { "
        "if_statement:( literal:3 ) { statement_seq:2 { literal:1 literal:2 } } "
        "endif } "
        "endif"},
};

TEST_P(SimpleExpressions, parse)
{
    const SimpleExpressionParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text, ParseOptions{})};

    ASSERT_TRUE(result);
    const ast::Expr bailout = result->get_section(Section::BAILOUT);
    ASSERT_TRUE(bailout);
    EXPECT_EQ(param.expected, trim_ws(to_string(bailout)));
    const ast::Expr init = result->get_section(Section::INITIALIZE);
    ASSERT_TRUE(init);
    EXPECT_EQ("statement_seq:0 { }", trim_ws(to_string(init)));
    const ast::Expr iter = result->get_section(Section::ITERATE);
    ASSERT_TRUE(iter);
    EXPECT_EQ("statement_seq:0 { }", trim_ws(to_string(iter)));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, SimpleExpressions, ValuesIn(s_simple_expressions));

TEST(TestFormulaParse, invalidIdentifier)
{
    const FormulaPtr result{create_formula("1a", ParseOptions{})};

    EXPECT_FALSE(result);
}

static MultiStatementParam s_multi_statements[]{
    {"sequence", "3,4"}, //
    {"statements", "3\n4\n"},
    {"assignmentStatements", "z=3\nz=4\n"},
    {"assignmentWithComments", "z=3; comment\nz=4; another comment\n"},
    {"assignmentWithBlankLines", "z=3; comment\n\r\n\nz=4; another comment\n"},
    {"commaSeparatedStatements", "3,4"},
    {"commaSeparatedAssignmentStatements", "z=3,z=4"},
    {"mixedNewlineAndCommaSeparators", "z=3,z=4\nz=5"},
    {"commaWithSpaces", "3 , 4"},
    {"compactIf", "if(1),1,elseif(2),2,else,0,endif"},
};

TEST_P(MultiStatements, parse)
{
    const MultiStatementParam &param{GetParam()};
    const FormulaPtr result{create_formula(param.text, ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
    const ast::Expr init = result->get_section(Section::INITIALIZE);
    ASSERT_TRUE(init);
    EXPECT_EQ("statement_seq:0 { }", trim_ws(to_string(init)));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, MultiStatements, ValuesIn(s_multi_statements));

TEST(TestFormulaParse, initializeIterateBailout)
{
    const FormulaPtr result{create_formula("z=pixel:z=z*z+pixel,|z|>4", ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

TEST(TestFormulaParse, statementSequenceInitialize)
{
    const FormulaPtr result{create_formula("z=pixel,d=0:z=z*z+pixel,|z|>4", ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

static std::vector<std::string> s_builtin_vars{
    "p1", "p2", "p3", "p4", "p5",                       //
    "pixel", "lastsqr", "rand", "pi", "e",              //
    "maxit", "scrnmax", "scrnpix", "whitesq", "ismand", //
    "center", "magxmag", "rotskew",                     //
};

TEST_P(BuiltinVariables, notAssignable)
{
    const FormulaPtr result{create_formula(GetParam() + "=1", ParseOptions{false})};

    ASSERT_FALSE(result);
}

TEST_P(BuiltinVariables, assignable)
{
    const FormulaPtr result{create_formula(GetParam() + "=1", ParseOptions{true})};

    ASSERT_TRUE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, BuiltinVariables, ValuesIn(s_builtin_vars));

static std::vector<std::string> s_functions{
    "sin", "cos", "sinh", "cosh", "cosxx",      //
    "tan", "cotan", "tanh", "cotanh", "sqr",    //
    "log", "exp", "abs", "conj", "real",        //
    "imag", "flip", "fn1", "fn2", "fn3",        //
    "fn4", "srand", "asin", "acos", "asinh",    //
    "acosh", "atan", "atanh", "sqrt", "cabs",   //
    "floor", "ceil", "trunc", "round", "ident", //
};


TEST_P(Functions, notAssignable)
{
    const FormulaPtr result{create_formula(GetParam() + "=1", ParseOptions{})};

    ASSERT_FALSE(result);
}

TEST_P(Functions, functionOne)
{
    const FormulaPtr result{create_formula(GetParam() + "(1)", ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, Functions, ValuesIn(s_functions));

TEST_P(ReservedWords, notAssignable)
{
    const FormulaPtr result1{create_formula(GetParam() + "=1", ParseOptions{})};
    const FormulaPtr result2{create_formula(GetParam() + "2=1", ParseOptions{})};

    ASSERT_FALSE(result1);
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result2->get_section(Section::BAILOUT));
}

static std::string s_reserved_words[]{
    "if",
    "else",
    "elseif",
    "endif",
};

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ReservedWords, ValuesIn(s_reserved_words));

static ParseFailureParam s_parse_failures[]{
    {"ifWithoutEndIf", "if(1)"},
    {"ifElseWithoutEndIf", "if(1)\nelse"},
    {"ifElseIfWithoutEndIf", "if(1)\nelseif(0)"},
    {"ifElseIfElseWithoutEndIf", "if(1)\nelseif(0)\nelse"},
    {"builtinSectionBogus", "builtin:type=0"},
};

TEST_P(ParseFailures, parse)
{
    const ParseFailureParam &param{GetParam()};
    const FormulaPtr result{create_formula(param.text, ParseOptions{})};

    EXPECT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ParseFailures, ValuesIn(s_parse_failures));

static ParseParam s_single_sections[]{
    {"globalSection",
        "global:\n"
        "1\n",
        Section::PER_IMAGE},
    {"initSection",
        "init:\n"
        "1\n",
        Section::INITIALIZE},
    {"loopSection",
        "loop:\n"
        "1\n",
        Section::ITERATE},
    {"bailoutSection",
        "bailout:\n"
        "1\n",
        Section::BAILOUT},
    {"perturbInitSection",
        "perturbinit:\n"
        "1\n",
        Section::PERTURB_INITIALIZE},
    {"perturbLoopSection",
        "perturbloop:\n"
        "1\n",
        Section::PERTURB_ITERATE},
    {"defaultSection",
        "default:\n"
        "angle=0\n",
        Section::DEFAULT},
    {"switchSection",
        "switch:\n"
        "type=\"Julia\"\n",
        Section::SWITCH},
};

TEST_P(SingleSections, parse)
{
    const ParseParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text, ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(param.section));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, SingleSections, ValuesIn(s_single_sections));

TEST(TestFormulaParse, builtinSectionMandelbrot)
{
    const FormulaPtr result{create_formula("builtin:\n"
                                           "type=1\n",
        ParseOptions{})};

    ASSERT_TRUE(result);
    const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=1\n", to_string(builtin));
}

TEST(TestFormulaParse, builtinSectionJulia)
{
    const FormulaPtr result{create_formula("builtin:\n"
                                           "type=2\n",
        ParseOptions{})};

    ASSERT_TRUE(result);
    const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=2\n", to_string(builtin));
}

static SimpleExpressionParam s_default_values[]{
    {"angle", "angle=0", "setting:angle=0\n"},                  //
    {"center", "center=(-0.5,0)", "setting:center=(-0.5,0)\n"}, //
    {"helpFile", R"(helpfile="HelpFile.html")",
        R"(setting:helpfile="HelpFile.html")"
        "\n"},
    {"helpTopic", R"(helptopic="DivideBrot5")",
        R"(setting:helptopic="DivideBrot5")"
        "\n"},
    {"magn", "magn=4.5", "setting:magn=4.5\n"},                            //
    {"maxIter", "maxiter=256", "setting:maxiter=256\n"},                   //
    {"methodGuessing", "method=guessing", "setting:method=guessing\n"},    //
    {"methodMultiPass", "method=multipass", "setting:method=multipass\n"}, //
    {"methodOnePass", "method=onepass", "setting:method=onepass\n"},       //
    {"periodicity0", "periodicity=0", "setting:periodicity=0\n"},          //
    {"periodicity1", "periodicity=1", "setting:periodicity=1\n"},          //
    {"periodicity2", "periodicity=2", "setting:periodicity=2\n"},          //
    {"periodicity3", "periodicity=3", "setting:periodicity=3\n"},          //
    {"perturbFalse", "perturb=false", "setting:perturb=false\n"},          //
    {"perturbTrue", "perturb=true", "setting:perturb=true\n"},             //
    {"perturbExpr", "perturb=power==2 || power == 3",
        "setting:perturb={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:3\n"
        "}\n"},
    {"precisionNumber", "precision=30",
        "setting:precision={\n"
        "literal:30\n"
        "}\n"}, //
    {"precisionExpr", "precision=log(fracmagn) / log(10)",
        "setting:precision={\n"
        "binary_op:/\n"
        "function_call:log(\n"
        "identifier:fracmagn\n"
        ")\n"
        "function_call:log(\n"
        "literal:10\n"
        ")\n"
        "}\n"},
    {"ratingRecommended", "rating=recommended", "setting:rating=recommended\n"},              //
    {"ratingAverage", "rating=average", "setting:rating=average\n"},                          //
    {"ratingNotRecommended", "rating=notRecommended", "setting:rating=notRecommended\n"},     //
    {"renderTrue", "render=true", "setting:render=true\n"},                                   //
    {"renderFalse", "render=false", "setting:render=false\n"},                                //
    {"skew", "skew=-4.5", "setting:skew=-4.5\n"},                                             //
    {"stretch", "stretch=4.5", "setting:stretch=4.5\n"},                                      //
    {"title", R"(title="This is a fancy one!")", "setting:title=\"This is a fancy one!\"\n"}, //
    {"emptyBoolParamBlock",
        "bool param foo\n"
        "endparam",
        "param_block:bool,foo {\n"
        "}\n"}, //
    {"emptyIntParamBlock",
        "int param foo\n"
        "endparam",
        "param_block:int,foo {\n"
        "}\n"}, //
    {"emptyFloatParamBlock",
        "float param foo\n"
        "endparam",
        "param_block:float,foo {\n"
        "}\n"}, //
    {"emptyComplexParamBlock",
        "complex param foo\n"
        "endparam",
        "param_block:complex,foo {\n"
        "}\n"}, //
    {"emptyColorParamBlock",
        "color param foo\n"
        "endparam",
        "param_block:color,foo {\n"
        "}\n"}, //
    {"captionParamBlock",
        "bool param foo\n"
        "caption=\"My parameter\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:caption=\"My parameter\"\n"
        "}\n"},
    {"typelessParamBlock",
        "param foo\n"
        "endparam",
        "param_block:foo {\n"
        "}\n"},
    {"defaultBoolParamBlock",
        "bool param foo\n"
        "default=true\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:default=true\n"
        "}\n"},
    {"defaultIntParamBlock",
        "int param foo\n"
        "default=1964\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:default=1964\n"
        "}\n"},
    {"defaultFloatParamBlock",
        "float param foo\n"
        "default=4.5\n"
        "endparam",
        "param_block:float,foo {\n"
        "setting:default=4.5\n"
        "}\n"},
    {"defaultComplexParamBlock",
        "complex param foo\n"
        "default=(4,5)\n"
        "endparam",
        "param_block:complex,foo {\n"
        "setting:default=(4,5)\n"
        "}\n"},
    {"enabledParamBlock",
        "bool param foo\n"
        "enabled=power==2 || power == 3\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:enabled={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:3\n"
        "}\n"
        "}\n"},
    {"enumParamBlock",
        "int param foo\n"
        "enum = \"Choice 1\" \"Choice 2\"\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:enum={\n"
        "\"Choice 1\"\n"
        "\"Choice 2\"\n"
        "}\n"
        "}\n"},
    {"expandedParamBlock",
        "int param foo\n"
        "expanded = false\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:expanded=false\n"
        "}\n"},
    {"exponentialParamBlock",
        "int param foo\n"
        "exponential = true\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:exponential=true\n"
        "}\n"},
    {"hintParamBlock",
        "bool param foo\n"
        "hint=\"look behind you\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:hint=\"look behind you\"\n"
        "}\n"},
    {"minIntParamBlock",
        "int param foo\n"
        "min=10\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:min=10\n"
        "}\n"},
    {"minFloatParamBlock",
        "float param foo\n"
        "min=4.5\n"
        "endparam",
        "param_block:float,foo {\n"
        "setting:min=4.5\n"
        "}\n"},
    {"minComplexParamBlock",
        "complex param foo\n"
        "min=(4.5,4.5)\n"
        "endparam",
        "param_block:complex,foo {\n"
        "setting:min=(4.5,4.5)\n"
        "}\n"},
    {"maxIntParamBlock",
        "int param foo\n"
        "max=10\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:max=10\n"
        "}\n"},
    {"maxFloatParamBlock",
        "float param foo\n"
        "max=4.5\n"
        "endparam",
        "param_block:float,foo {\n"
        "setting:max=4.5\n"
        "}\n"},
    {"maxComplexParamBlock",
        "complex param foo\n"
        "max=(4.5,4.5)\n"
        "endparam",
        "param_block:complex,foo {\n"
        "setting:max=(4.5,4.5)\n"
        "}\n"},
    {"selectableParamBlock",
        "bool param foo\n"
        "selectable=false\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:selectable=false\n"
        "}\n"},
    {"textParamBlock",
        "bool param foo\n"
        "text=\"Well hello there, pardner.\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:text=\"Well hello there, pardner.\"\n"
        "}\n"},
    {"visibleParamBlock",
        "bool param foo\n"
        "visible=power == 2 || power == 3\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:visible={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:3\n"
        "}\n"
        "}\n"},
};

TEST_P(DefaultSection, parse)
{
    const SimpleExpressionParam &param{GetParam()};
    std::string text{"default:\n"};
    text.append(param.text);
    if (text.back() != '\n')
    {
        text.append(1, '\n');
    }

    const FormulaPtr result{create_formula(text, ParseOptions{})};

    ASSERT_TRUE(result);
    const ast::Expr &section{result->get_section(Section::DEFAULT)};
    EXPECT_TRUE(section);
    EXPECT_EQ(param.expected, to_string(section));
}

INSTANTIATE_TEST_SUITE_P(TestFormualParse, DefaultSection, ValuesIn(s_default_values));

TEST(TestParse, switchParameter)
{
    const FormulaPtr result{create_formula("switch:\n"
                                           "seed=pixel\n",
        ParseOptions{})};

    ASSERT_TRUE(result);
    const ast::Expr &section{result->get_section(Section::SWITCH)};
    ASSERT_TRUE(section);
    EXPECT_EQ("setting:seed=pixel\n", to_string(section));
}

static InvalidSectionParam s_invalid_sections[]{
    {"unknownSection",
        "global:\n"
        "1\n"
        "unknown:\n"
        "1\n"},
    {"globalSectionFirst",
        "builtin:\n"
        "type=1\n"
        "global:\n"
        "1\n"},
    {"builtinBeforeInit",
        "init:\n"
        "1\n"
        "builtin:\n"
        "type=1\n"},
    {"initAfterLoop",
        "loop:\n"
        "1\n"
        "init:\n"
        "1\n"},
    {"initAfterBailout",
        "bailout:\n"
        "1\n"
        "init:\n"
        "1\n"},
    {"loopAfterBailout",
        "bailout:\n"
        "1\n"
        "loop:\n"
        "1\n"},
    {"bailoutAfterPerturbInit",
        "perturbinit:\n"
        "1\n"
        "bailout:\n"
        "1\n"},
    {"perturbInitAfterPerturbLoop",
        "perturbloop:\n"
        "1\n"
        "perturbinit:\n"
        "1\n"},
    {"perturbLoopAfterDefault",
        "default:\n"
        "angle=0\n"
        "perturbloop:\n"
        "1\n"},
    {"defaultAfterSwitch",
        "switch:\n"
        "1\n"
        "default:\n"
        "angle=0\n"},
    {"globalTwice",
        "global:\n"
        "1\n"
        "global:\n"
        "1\n"},
    {"builtinTwice",
        "builtin:\n"
        "type=1\n"
        "builtin:\n"
        "type=1\n"},
    {"initTwice",
        "init:\n"
        "1\n"
        "init:\n"
        "1\n"},
    {"loopTwice",
        "loop:\n"
        "1\n"
        "loop:\n"
        "1\n"},
    {"bailoutTwice",
        "bailout:\n"
        "1\n"
        "bailout:\n"
        "1\n"},
    {"perturbInitTwice",
        "perturbinit:\n"
        "1\n"
        "perturbinit:\n"
        "1\n"},
    {"perturbLoopTwice",
        "perturbloop:\n"
        "1\n"
        "perturbloop:\n"
        "1\n"},
    {"defaultTwice",
        "default:\n"
        "angle=120\n"
        "default:\n"
        "angle=120\n"},
    {"switchTwice",
        "switch:\n"
        "foo=foo\n"
        "switch:\n"
        "foo=foo\n"},
    {"invalidMethod",
        "default:\n"
        "method=junk\n"},
};

class InvalidSectionOrdering : public TestWithParam<InvalidSectionParam>
{
};

TEST_P(InvalidSectionOrdering, parse)
{
    const InvalidSectionParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text, ParseOptions{})};

    ASSERT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, InvalidSectionOrdering, ValuesIn(s_invalid_sections));

TEST(TestFormulaParse, maximumSections)
{
    const FormulaPtr result{create_formula("global:\n"
                                           "1\n"
                                           "init:\n"
                                           "1\n"
                                           "loop:\n"
                                           "1\n"
                                           "bailout:\n"
                                           "1\n"
                                           "perturbinit:\n"
                                           "1\n"
                                           "perturbloop:\n"
                                           "1\n"
                                           "default:\n"
                                           "angle=0\n"
                                           "switch:\n"
                                           "type=\"Julia\"\n",
        ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::PER_IMAGE));
    EXPECT_FALSE(result->get_section(Section::BUILTIN));
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
    EXPECT_TRUE(result->get_section(Section::PERTURB_INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::PERTURB_ITERATE));
    EXPECT_TRUE(result->get_section(Section::DEFAULT));
    EXPECT_TRUE(result->get_section(Section::SWITCH));
}

TEST(TestFormulaParse, builtinSections)
{
    const FormulaPtr result{create_formula("builtin:\n"
                                           "type=1\n"
                                           "perturbinit:\n"
                                           "1\n"
                                           "perturbloop:\n"
                                           "1\n"
                                           "default:\n"
                                           "angle=0\n"
                                           "switch:\n"
                                           "type=\"Julia\"\n",
        ParseOptions{})};

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->get_section(Section::PER_IMAGE));
    ast::Expr builtin = result->get_section(Section::BUILTIN);
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=1\n", to_string(builtin));
    EXPECT_FALSE(result->get_section(Section::INITIALIZE));
    EXPECT_FALSE(result->get_section(Section::ITERATE));
    EXPECT_FALSE(result->get_section(Section::BAILOUT));
    EXPECT_TRUE(result->get_section(Section::PERTURB_INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::PERTURB_ITERATE));
    EXPECT_TRUE(result->get_section(Section::DEFAULT));
    EXPECT_TRUE(result->get_section(Section::SWITCH));
}

static BuiltinDisallowsParam s_builtin_disallows[]{
    {"global",
        "global:\n"
        "1\n"
        "builtin:\n"
        "type=1\n"},
    {"init",
        "builtin:\n"
        "type=1\n"
        "init:\n"
        "1\n"},
    {"loop",
        "builtin:\n"
        "type=1\n"
        "loop:\n"
        "1\n"},
    {"bailout",
        "builtin:\n"
        "type=1\n"
        "bailout:\n"
        "1\n"},
};

TEST_P(BuiltinDisallows, parse)
{
    const BuiltinDisallowsParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text, ParseOptions{})};

    ASSERT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, BuiltinDisallows, ValuesIn(s_builtin_disallows));

TEST(TestParse, emptyInit)
{
    const FormulaPtr result{create_formula(":|imag(pixel)|<p1", ParseOptions{})};

    ASSERT_TRUE(result) << "parse failed";
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

} // namespace formula::test
