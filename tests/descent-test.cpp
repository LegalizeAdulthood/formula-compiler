// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "NodeFormatter.h"
#include "trim_ws.h"

#include <gtest/gtest.h>

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

class DescentSimpleExpressions : public TestWithParam<SimpleExpressionParam>
{
};

} // namespace

static SimpleExpressionParam s_simple_expressions[]{
    {"constant", "1", "number:1"},                                                        //
    {"identifier", "z2", "identifier:z2"},                                                //
    {"parenExpr", "(z)", "identifier:z"},                                                 //
    {"add", "1+2", "binary_op:+ number:1 number:2"},                                      //
    {"subtract", "1-2", "binary_op:- number:1 number:2"},                                 //
    {"multiply", "1*2", "binary_op:* number:1 number:2"},                                 //
    {"divide", "1/2", "binary_op:/ number:1 number:2"},                                   //
    {"multiplyAdd", "1*2+4", "binary_op:+ binary_op:* number:1 number:2 number:4"},       //
    {"parenthesisExpr", "1*(2+4)", "binary_op:* number:1 binary_op:+ number:2 number:4"}, //
    {"unaryMinus", "-(1)", "unary_op:- number:1"},                                        //
    {"unaryPlus", "+(1)", "unary_op:+ number:1"},                                         //
    {"unaryMinusNegativeOne", "--1", "unary_op:- unary_op:- number:1"},                   //
    {"addAddAdd", "1+2+3", "binary_op:+ binary_op:+ number:1 number:2 number:3"},         //
    {"capitalLetterInIdentifier", "A", "identifier:A"},                                   //
    {"numberInIdentifier", "a1", "identifier:a1"},                                        //
    {"underscoreInIdentifier", "A_1", "identifier:A_1"},                                  //
    {"power", "2^3", "binary_op:^ number:2 number:3"},                                    //
    {"chainedPower", "1^2^3", "binary_op:^ number:1 binary_op:^ number:2 number:3"},      //
    {"assignment", "z=4", "assignment:z number:4"},                                       //
    {"assignmentLongVariable", "this_is_another4_variable_name2=4",
        "assignment:this_is_another4_variable_name2 number:4"},                                                 //
    {"modulus", "|-3.0|", "unary_op:| unary_op:- number:3"},                                                    //
    {"compareLess", "4<3", "binary_op:< number:4 number:3"},                                                    //
    {"compareLessPrecedence", "3<z=4", "binary_op:< number:3 assignment:z number:4"},                           //
    {"compareLessEqual", "4<=3", "binary_op:<= number:4 number:3"},                                             //
    {"compareGreater", "4>3", "binary_op:> number:4 number:3"},                                                 //
    {"compareAssociatesLeft", "4>3<4", "binary_op:< binary_op:> number:4 number:3 number:4"},                   //
    {"compareGreaterEqual", "4>=3", "binary_op:>= number:4 number:3"},                                          //
    {"compareEqual", "4==3", "binary_op:== number:4 number:3"},                                                 //
    {"compareNotEqual", "4!=3", "binary_op:!= number:4 number:3"},                                              //
    {"logicalAnd", "4==3&&5==6", "binary_op:&& binary_op:== number:4 number:3 binary_op:== number:5 number:6"}, //
    {"logicalOr", "4==3||5==6", "binary_op:|| binary_op:== number:4 number:3 binary_op:== number:5 number:6"},  //
    {"ignoreComments", "3; z=6 oh toodlee doo", "number:3"},                                                    //
    {"ignoreCommentsLF", "3; z=6 oh toodlee doo\n", "number:3"},                                                //
    {"ignoreCommentsCRLF", "3; z=6 oh toodlee doo\r\n", "number:3"},                                            //
    {"reservedVariablePrefixToUserVariable", "e2=1", "assignment:e2 number:1"},                                 //
    {"reservedFunctionPrefixToUserVariable", "sine=1", "assignment:sine number:1"},                             //
    {"reservedKeywordPrefixToUserVariable", "if1=1", "assignment:if1 number:1"},                                //
    {"builtinVariableP1", "p1", "identifier:p1"},                                                               //
    {"builtinVariableP2", "p2", "identifier:p2"},                                                               //
    {"builtinVariableP3", "p3", "identifier:p3"},                                                               //
    {"builtinVariableP4", "p4", "identifier:p4"},                                                               //
    {"builtinVariableP5", "p5", "identifier:p5"},                                                               //
    {"builtinVariablePixel", "pixel", "identifier:pixel"},                                                      //
    {"builtinVariableLastsqr", "lastsqr", "identifier:lastsqr"},                                                //
    {"builtinVariableRand", "rand", "identifier:rand"},                                                         //
    {"builtinVariablePi", "pi", "identifier:pi"},                                                               //
    {"builtinVariableE", "e", "identifier:e"},                                                                  //
    {"builtinVariableMaxit", "maxit", "identifier:maxit"},                                                      //
    {"builtinVariableScrnmax", "scrnmax", "identifier:scrnmax"},                                                //
    {"builtinVariableScrnpix", "scrnpix", "identifier:scrnpix"},                                                //
    {"builtinVariableWhitesq", "whitesq", "identifier:whitesq"},                                                //
    {"builtinVariableIsmand", "ismand", "identifier:ismand"},                                                   //
    {"builtinVariableCenter", "center", "identifier:center"},                                                   //
    {"builtinVariableMagxmag", "magxmag", "identifier:magxmag"},                                                //
    {"builtinVariableRotskew", "rotskew", "identifier:rotskew"},                                                //
    {"ifEmptyBody",
        "if(0)\n"
        "endif",
        "if_statement:( number:0 ) { } endif"},
    {"ifBlankLinesBody",
        "if(0)\n"
        "\n"
        "endif",
        "if_statement:( number:0 ) { } endif"},
    {"ifThenBody",
        "if(0)\n"
        "1\n"
        "endif",
        "if_statement:( number:0 ) { number:1 } endif"},
    {"ifThenComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "endif",
        "if_statement:( number:0 ) { statement_seq:2 { number:1 number:2 } } endif"},
    {"ifComparisonCondition",
        "if(1<2)\n"
        "endif",
        "if_statement:( binary_op:< number:1 number:2 ) { } endif"},
    {"ifConjunctiveCondition",
        "if(1&&2)\n"
        "endif",
        "if_statement:( binary_op:&& number:1 number:2 ) { } endif"},
    {"ifElseEmptyBody",
        "if(0)\n"
        "else\n"
        "endif",
        "if_statement:( number:0 ) { } endif"},
    {"ifElseBlankLinesBody",
        "if(0)\n"
        "\n"
        "else\n"
        "\n"
        "endif",
        "if_statement:( number:0 ) { } endif"},
    {"ifThenElseBody",
        "if(0)\n"
        "else\n"
        "1\n"
        "endif",
        "if_statement:( number:0 ) { } else { number:1 } endif"},
    {"ifThenBodyElse",
        "if(0)\n"
        "1\n"
        "else\n"
        "endif",
        "if_statement:( number:0 ) { number:1 } endif"},
    {"ifThenElseComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "else\n"
        "3\n"
        "4\n"
        "endif",
        "if_statement:( number:0 ) { statement_seq:2 { number:1 number:2 } "
        "} else { statement_seq:2 { number:3 number:4 } } endif"},
    {"ifElseIfEmptyBody",
        "if(0)\n"
        "elseif(1)\n"
        "else\n"
        "endif",
        "if_statement:( number:0 ) { } else { if_statement:( number:1 ) { } endif } endif"},
    {"ifMultipleElseIfEmptyBody",
        "if(0)\n"
        "elseif(0)\n"
        "elseif(0)\n"
        "else\n"
        "endif",
        "if_statement:( number:0 ) { } "
        "else { if_statement:( number:0 ) { } "
        "else { if_statement:( number:0 ) { } "
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
        "if_statement:( number:0 ) { } "
        "else { if_statement:( number:0 ) { } "
        "endif } "
        "endif"},
    {"ifThenElseIfBody",
        "if(0)\n"
        "elseif(0)\n"
        "1\n"
        "endif",
        "if_statement:( number:0 ) { } "
        "else { if_statement:( number:0 ) { number:1 } "
        "endif } "
        "endif"},
    {"ifThenBodyElseIf",
        "if(0)\n"
        "1\n"
        "elseif(0)\n"
        "endif",
        "if_statement:( number:0 ) { number:1 } "
        "else { if_statement:( number:0 ) { } "
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
        "if_statement:( number:0 ) { statement_seq:2 { number:1 number:2 } } "
        "else { if_statement:( number:1 ) { statement_seq:2 { number:3 number:4 } } "
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
        "if_statement:( number:0 ) { number:1 } "
        "else { if_statement:( number:2 ) { number:3 } "
        "else { if_statement:( number:4 ) { number:5 } "
        "else { if_statement:( number:6 ) { number:7 } "
        "endif } "
        "endif } "
        "endif } "
        "endif"},
    {"backslashContinuesStatement",
        "if(\\\n"
        "0)\n"
        "endif",
        "if_statement:( number:0 ) { } endif"},
    {"ifSequence",
        "if(0)\n"
        "1,2\n"
        "endif",
        "if_statement:( number:0 ) { statement_seq:2 { number:1 number:2 } } endif"},
    {"elseSequence",
        "if(0)\n"
        "else\n"
        "1,2\n"
        "endif",
        "if_statement:( number:0 ) { } else { statement_seq:2 { number:1 number:2 } } endif"},
    {"elseIfSequence",
        "if(0)\n"
        "elseif(3)\n"
        "1,2\n"
        "endif",
        "if_statement:( number:0 ) { } else { "
        "if_statement:( number:3 ) { statement_seq:2 { number:1 number:2 } } "
        "endif } "
        "endif"},
};

TEST_P(DescentSimpleExpressions, parse)
{
    const SimpleExpressionParam &param{GetParam()};

    const FormulaPtr result{create_descent_formula(param.text)};

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->get_section(Section::INITIALIZE));
    EXPECT_FALSE(result->get_section(Section::ITERATE));
    const ast::Expr bailout = result->get_section(Section::BAILOUT);
    ASSERT_TRUE(bailout);
    EXPECT_EQ(param.expected, trim_ws(to_string(bailout)));
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DescentSimpleExpressions, ValuesIn(s_simple_expressions));

TEST(TestDescentParse, invalidIdentifier)
{
    const FormulaPtr result{create_descent_formula("1a")};

    EXPECT_FALSE(result);
}

struct DMultiStatementParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const DMultiStatementParam &param, std::ostream *os)
{
    *os << param.name;
}

static DMultiStatementParam s_multi_statements[]{
    {"sequence", "3,4"},                                                        //
    {"statements", "3\n4\n"},                                                   //
    {"assignmentStatements", "z=3\nz=4\n"},                                     //
    {"assignmentWithComments", "z=3; comment\nz=4; another comment\n"},         //
    {"assignmentWithBlankLines", "z=3; comment\n\r\n\nz=4; another comment\n"}, //
    {"commaSeparatedStatements", "3,4"},                                        //
    {"commaSeparatedAssignmentStatements", "z=3,z=4"},                          //
    {"mixedNewlineAndCommaSeparators", "z=3,z=4\nz=5"},                         //
    {"commaWithSpaces", "3 , 4"},
};

class DMultiStatements : public TestWithParam<DMultiStatementParam>
{
};

TEST_P(DMultiStatements, parse)
{
    const DMultiStatementParam &param{GetParam()};
    const FormulaPtr result{create_descent_formula(param.text)};

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DMultiStatements, ValuesIn(s_multi_statements));

TEST(TestDescentParse, initializeIterateBailout)
{
    const FormulaPtr result{create_descent_formula("z=pixel:z=z*z+pixel,|z|>4")};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

TEST(TestDescentParse, statementSequenceInitialize)
{
    const FormulaPtr result{create_descent_formula("z=pixel,d=0:z=z*z+pixel,|z|>4")};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

static std::vector<std::string> s_read_only_vars{
    "p1", "p2", "p3", "p4", "p5",                       //
    "pixel", "lastsqr", "rand", "pi", "e",              //
    "maxit", "scrnmax", "scrnpix", "whitesq", "ismand", //
    "center", "magxmag", "rotskew",                     //
};

class DReadOnlyVariables : public TestWithParam<std::string>
{
};

TEST_P(DReadOnlyVariables, notAssignable)
{
    const FormulaPtr result{create_descent_formula(GetParam() + "=1")};

    ASSERT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DReadOnlyVariables, ValuesIn(s_read_only_vars));

static std::vector<std::string> s_functions{
    "sin", "cos", "sinh", "cosh", "cosxx",      //
    "tan", "cotan", "tanh", "cotanh", "sqr",    //
    "log", "exp", "abs", "conj", "real",        //
    "imag", "flip", "fn1", "fn2", "fn3",        //
    "fn4", "srand", "asin", "acos", "asinh",    //
    "acosh", "atan", "atanh", "sqrt", "cabs",   //
    "floor", "ceil", "trunc", "round", "ident", //
    "one", "zero",                              //
};

class DFunctions : public TestWithParam<std::string>
{
};

TEST_P(DFunctions, notAssignable)
{
    const FormulaPtr result{create_descent_formula(GetParam() + "=1")};

    ASSERT_FALSE(result);
}

TEST_P(DFunctions, functionOne)
{
    const FormulaPtr result{create_descent_formula(GetParam() + "(1)")};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DFunctions, ValuesIn(s_functions));

class DReservedWords : public TestWithParam<std::string>
{
};

TEST_P(DReservedWords, notAssignable)
{
    const FormulaPtr result1{create_descent_formula(GetParam() + "=1")};
    const FormulaPtr result2{create_descent_formula(GetParam() + "2=1")};

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

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DReservedWords, ValuesIn(s_reserved_words));

struct ParseFailureParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const ParseFailureParam &param, std::ostream *os)
{
    *os << param.name;
}

static ParseFailureParam s_parse_failures[]{
    {"ifWithoutEndIf", "if(1)"}, {"ifElseWithoutEndIf", "if(1)\nelse"}, {"ifElseIfWithoutEndIf", "if(1)\nelseif(0)"},
    {"ifElseIfElseWithoutEndIf", "if(1)\nelseif(0)\nelse"},
    //{"builtinSectionBogus", "builtin:type=0"},
};

class DParseFailures : public TestWithParam<ParseFailureParam>
{
};

TEST_P(DParseFailures, parse)
{
    const ParseFailureParam &param{GetParam()};
    const FormulaPtr result{create_descent_formula(param.text)};

    EXPECT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DParseFailures, ValuesIn(s_parse_failures));

struct SingleSectionParam
{
    std::string_view name;
    std::string_view text;
    Section section;
};

inline void PrintTo(const SingleSectionParam &param, std::ostream *os)
{
    *os << param.name;
}

static SingleSectionParam s_single_sections[]{
    {"globalSection", "global:1", Section::PER_IMAGE},
    {"initSection", "init:1", Section::INITIALIZE},
    {"loopSection", "loop:1", Section::ITERATE},
    {"bailoutSection", "bailout:1", Section::BAILOUT},
    {"perturbInitSection", "perturbinit:1", Section::PERTURB_INITIALIZE},
    {"perturbLoopSection", "perturbloop:1", Section::PERTURB_ITERATE},
    {"defaultSection", "default:angle=0", Section::DEFAULT},
    {"switchSection", "switch:1", Section::SWITCH},
};

class DSingleSections : public TestWithParam<SingleSectionParam>
{
};

TEST_P(DSingleSections, parse)
{
    const SingleSectionParam &param{GetParam()};

    const FormulaPtr result{create_descent_formula(param.text)};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(param.section));
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DSingleSections, ValuesIn(s_single_sections));

TEST(TestDescentParse, builtinSectionMandelbrot)
{
    const FormulaPtr result{create_descent_formula("builtin:type=1")};

    ASSERT_TRUE(result);
    const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=1\n", to_string(builtin));
}

TEST(TestDescentParse, builtinSectionJulia)
{
    const FormulaPtr result{create_descent_formula("builtin:type=2")};

    ASSERT_TRUE(result);
    const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=2\n", to_string(builtin));
}

struct DefaultSectionParam
{
    std::string_view name;
    std::string_view text;
    std::string_view expected;
};

inline void PrintTo(const DefaultSectionParam &param, std::ostream *os)
{
    *os << param.name;
}

static DefaultSectionParam s_default_values[]{
    {"angle", "default:angle=0", "setting:angle=0\n"},                  //
    {"center", "default:center=(-0.5,0)", "setting:center=(-0.5,0)\n"}, //
    {"helpFile", R"(default:helpfile="HelpFile.html")",
        R"(setting:helpfile="HelpFile.html")"
        "\n"},
    {"helpTopic", R"(default:helptopic="DivideBrot5")",
        R"(setting:helptopic="DivideBrot5")"
        "\n"},
    {"magn", "default:magn=4.5", "setting:magn=4.5\n"},                            //
    {"maxIter", "default:maxiter=256", "setting:maxiter=256\n"},                   //
    {"methodGuessing", "default:method=guessing", "setting:method=guessing\n"},    //
    {"methodMultiPass", "default:method=multipass", "setting:method=multipass\n"}, //
    {"methodOnePass", "default:method=onepass", "setting:method=onepass\n"},       //
    {"periodicity0", "default:periodicity=0", "setting:periodicity=0\n"},          //
    {"periodicity1", "default:periodicity=1", "setting:periodicity=1\n"},          //
    {"periodicity2", "default:periodicity=2", "setting:periodicity=2\n"},          //
    {"periodicity3", "default:periodicity=3", "setting:periodicity=3\n"},          //
    {"perturbFalse", "default:perturb=false", "setting:perturb=false\n"},          //
    {"perturbTrue", "default:perturb=true", "setting:perturb=true\n"},             //
    {"perturbExpr", "default:perturb=power==2 || power == 3 || power == 4",
        "setting:perturb={\n"
        "binary_op:||\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "number:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "number:3\n"
        "binary_op:==\n"
        "identifier:power\n"
        "number:4\n"
        "}\n"}, //
    {"precisionNumber", "default:precision=30",
        "setting:precision={\n"
        "number:30\n"
        "}\n"}, //
    {"precisionExpr", "default:precision=log(fracmagn) / log(10)",
        "setting:precision={\n"
        "binary_op:/\n"
        "function_call:log(\n"
        "identifier:fracmagn\n"
        ")\n"
        "function_call:log(\n"
        "number:10\n"
        ")\n"
        "}\n"}, //
    {"ratingRecommended", "default:rating=recommended", "setting:rating=recommended\n"},              //
    {"ratingAverage", "default:rating=average", "setting:rating=average\n"},                          //
    {"ratingNotRecommended", "default:rating=notRecommended", "setting:rating=notRecommended\n"},     //
    {"renderTrue", "default:render=true", "setting:render=true\n"},                                   //
    {"renderFalse", "default:render=false", "setting:render=false\n"},                                //
    {"skew", "default:skew=-4.5", "setting:skew=-4.5\n"},                                             //
    {"stretch", "default:stretch=4.5", "setting:stretch=4.5\n"},                                      //
    {"title", R"(default:title="This is a fancy one!")", "setting:title=\"This is a fancy one!\"\n"}, //
    {"emptyBoolParamBlock",
        "default:bool param foo\n"
        "endparam",
        "param_block:bool,foo {\n"
        "}\n"}, //
    {"emptyIntParamBlock",
        "default:int param foo\n"
        "endparam",
        "param_block:int,foo {\n"
        "}\n"}, //
    {"emptyFloatParamBlock",
        "default:float param foo\n"
        "endparam",
        "param_block:float,foo {\n"
        "}\n"}, //
    {"emptyComplexParamBlock",
        "default:complex param foo\n"
        "endparam",
        "param_block:complex,foo {\n"
        "}\n"}, //
    {"emptyColorParamBlock",
        "default:color param foo\n"
        "endparam",
        "param_block:color,foo {\n"
        "}\n"}, //
    {"captionParamBlock",
        "default:bool param foo\n"
        "caption=\"My parameter\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:caption=\"My parameter\"\n"
        "}\n"},
    {"typelessParamBlock",
        "default:param foo\n"
        "endparam",
        "param_block:foo {\n"
        "}\n"},
    {"defaultParamBlock",
        "default:bool param foo\n"
        "default=true\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:default=true\n"
        "}\n"},
};

class DDefaultSection : public TestWithParam<DefaultSectionParam>
{
};

TEST_P(DDefaultSection, parse)
{
    const DefaultSectionParam &param{GetParam()};

    const FormulaPtr result{create_descent_formula(param.text)};

    ASSERT_TRUE(result);
    const ast::Expr &section{result->get_section(Section::DEFAULT)};
    EXPECT_TRUE(section);
    EXPECT_EQ(param.expected, to_string(section));
}

INSTANTIATE_TEST_SUITE_P(TestDescentParse, DDefaultSection, ValuesIn(s_default_values));

// struct InvalidSectionParam
//{
//     std::string_view name;
//     std::string_view text;
// };
//
// inline void PrintTo(const InvalidSectionParam &param, std::ostream *os)
//{
//     *os << param.name;
// }
//
// static InvalidSectionParam s_invalid_sections[]{
//     {"unknownSection",
//         "global:1\n"
//         "unknown:1"},
//     {"globalSectionFirst",
//         "builtin:1\n"
//         "global:1"},
//     {"builtinBeforeInit",
//         "init:1\n"
//         "builtin:1"},
//     {"initAfterLoop",
//         "loop:1\n"
//         "init:1"},
//     {"initAfterBailout",
//         "bailout:1\n"
//         "init:1"},
//     {"loopAfterBailout",
//         "bailout:1\n"
//         "loop:1"},
//     {"bailoutAfterPerturbInit",
//         "perturbinit:1\n"
//         "bailout:1"},
//     {"perturbInitAfterPerturbLoop",
//         "perturbloop:1\n"
//         "perturbinit:1"},
//     {"perturbLoopAfterDefault",
//         "default:angle=0\n"
//         "perturbloop:1"},
//     {"defaultAfterSwitch",
//         "switch:1\n"
//         "default:angle=0"},
// };
//
// class InvalidSectionOrdering : public TestWithParam<InvalidSectionParam>
//{
// };
//
// TEST_P(InvalidSectionOrdering, parse)
//{
//     const InvalidSectionParam &param{GetParam()};
//
//     const FormulaPtr result{create_descent_formula(param.text)};
//
//     ASSERT_FALSE(result);
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, InvalidSectionOrdering, ValuesIn(s_invalid_sections));
//
// TEST(TestDescentParse, maximumSections)
//{
//     const FormulaPtr result{create_descent_formula("global:1\n"
//                                   "init:1\n"
//                                   "loop:1\n"
//                                   "bailout:1\n"
//                                   "perturbinit:1\n"
//                                   "perturbloop:1\n"
//                                   "default:angle=0\n"
//                                   "switch:1\n")};
//
//     ASSERT_TRUE(result);
//     EXPECT_TRUE(result->get_section(Section::PER_IMAGE));
//     EXPECT_FALSE(result->get_section(Section::BUILTIN));
//     EXPECT_TRUE(result->get_section(Section::INITIALIZE));
//     EXPECT_TRUE(result->get_section(Section::ITERATE));
//     EXPECT_TRUE(result->get_section(Section::BAILOUT));
//     EXPECT_TRUE(result->get_section(Section::PERTURB_INITIALIZE));
//     EXPECT_TRUE(result->get_section(Section::PERTURB_ITERATE));
//     EXPECT_TRUE(result->get_section(Section::DEFAULT));
//     EXPECT_TRUE(result->get_section(Section::SWITCH));
// }
//
// TEST(TestDescentParse, builtinSections)
//{
//     const FormulaPtr result{create_descent_formula("builtin:type=1\n"
//                                   "perturbinit:1\n"
//                                   "perturbloop:1\n"
//                                   "default:angle=0\n"
//                                   "switch:1\n")};
//
//     ASSERT_TRUE(result);
//     EXPECT_FALSE(result->get_section(Section::PER_IMAGE));
//     ast::Expr builtin = result->get_section(Section::BUILTIN);
//     EXPECT_TRUE(builtin);
//     EXPECT_EQ("setting:type=1\n", to_string(builtin));
//     EXPECT_FALSE(result->get_section(Section::INITIALIZE));
//     EXPECT_FALSE(result->get_section(Section::ITERATE));
//     EXPECT_FALSE(result->get_section(Section::BAILOUT));
//     EXPECT_TRUE(result->get_section(Section::PERTURB_INITIALIZE));
//     EXPECT_TRUE(result->get_section(Section::PERTURB_ITERATE));
//     EXPECT_TRUE(result->get_section(Section::DEFAULT));
//     EXPECT_TRUE(result->get_section(Section::SWITCH));
// }
//
// struct BuiltinDisallowsParam
//{
//     std::string_view name;
//     std::string_view text;
// };
//
// inline void PrintTo(const BuiltinDisallowsParam &param, std::ostream *os)
//{
//     *os << param.name;
// }
//
// static BuiltinDisallowsParam s_builtin_disallows[]{
//     {"global",
//         "global:1\n"
//         "builtin:type=1\n"
//         "perturbinit:1\n"
//         "perturbloop:1\n"
//         "default:angle=0\n"
//         "switch:1\n"},
//     {"init",
//         "builtin:type=1\n"
//         "init:1\n"
//         "perturbinit:1\n"
//         "perturbloop:1\n"
//         "default:angle=0\n"
//         "switch:1\n"},
//     {"loop",
//         "builtin:type=1\n"
//         "loop:1\n"
//         "perturbinit:1\n"
//         "perturbloop:1\n"
//         "default:angle=0\n"
//         "switch:1\n"},
//     {"bailout",
//         "builtin:type=1\n"
//         "bailout:1\n"
//         "perturbinit:1\n"
//         "perturbloop:1\n"
//         "default:angle=0\n"
//         "switch:1\n"},
// };
//
// class BuiltinDisallows : public TestWithParam<BuiltinDisallowsParam>
//{
// };
//
// TEST_P(BuiltinDisallows, parse)
//{
//     const BuiltinDisallowsParam &param{GetParam()};
//
//     const FormulaPtr result{create_descent_formula(param.text)};
//
//     ASSERT_FALSE(result);
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, BuiltinDisallows, ValuesIn(s_builtin_disallows));

} // namespace formula::test
