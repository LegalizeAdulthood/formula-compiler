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
    {"constant", "1", "number:1"}, {"identifier", "z2", "identifier:z2"}, {"parenExpr", "(z)", "identifier:z"},
    {"add", "1+2", "binary_op:+ number:1 number:2"}, {"subtract", "1-2", "binary_op:- number:1 number:2"},
    {"multiply", "1*2", "binary_op:* number:1 number:2"}, {"divide", "1/2", "binary_op:/ number:1 number:2"},
    {"multiplyAdd", "1*2+4", "binary_op:+ binary_op:* number:1 number:2 number:4"},
    {"parenthesisExpr", "1*(2+4)", "binary_op:* number:1 binary_op:+ number:2 number:4"},
    {"unaryMinus", "-(1)", "unary_op:- number:1"}, {"unaryPlus", "+(1)", "unary_op:+ number:1"},
    {"unaryMinusNegativeOne", "--1", "unary_op:- unary_op:- number:1"},
    {"addAddAdd", "1+2+3", "binary_op:+ binary_op:+ number:1 number:2 number:3"},
    {"capitalLetterInIdentifier", "A", "identifier:A"}, {"numberInIdentifier", "a1", "identifier:a1"},
    {"underscoreInIdentifier", "A_1", "identifier:A_1"}, {"power", "2^3", "binary_op:^ number:2 number:3"},
    {"chainedPower", "1^2^3", "binary_op:^ number:1 binary_op:^ number:2 number:3"},
    {"assignment", "z=4", "assignment:z number:4"},
    {"assignmentLongVariable", "this_is_another4_variable_name2=4",
        "assignment:this_is_another4_variable_name2 number:4"},
    {"modulus", "|-3.0|", "unary_op:| unary_op:- number:3"}, {"compareLess", "4<3", "binary_op:< number:4 number:3"},
    {"compareLessPrecedence", "3<z=4", "binary_op:< number:3 assignment:z number:4"},
    {"compareLessEqual", "4<=3", "binary_op:<= number:4 number:3"},
    {"compareGreater", "4>3", "binary_op:> number:4 number:3"},
    {"compareAssociatesLeft", "4>3<4", "binary_op:< binary_op:> number:4 number:3 number:4"},
    {"compareGreaterEqual", "4>=3", "binary_op:>= number:4 number:3"},
    {"compareEqual", "4==3", "binary_op:== number:4 number:3"},
    {"compareNotEqual", "4!=3", "binary_op:!= number:4 number:3"},
    {"logicalAnd", "4==3&&5==6", "binary_op:&& binary_op:== number:4 number:3 binary_op:== number:5 number:6"},
    {"logicalOr", "4==3||5==6", "binary_op:|| binary_op:== number:4 number:3 binary_op:== number:5 number:6"},
    {"ignoreComments", "3; z=6 oh toodlee doo", "number:3"},
    {"ignoreCommentsLF", "3; z=6 oh toodlee doo\n", "number:3"},
    {"ignoreCommentsCRLF", "3; z=6 oh toodlee doo\r\n", "number:3"},
    {"reservedVariablePrefixToUserVariable", "e2=1", "assignment:e2 number:1"},
    {"reservedFunctionPrefixToUserVariable", "sine=1", "assignment:sine number:1"},
    {"reservedKeywordPrefixToUserVariable", "if1=1", "assignment:if1 number:1"},
    {"ifEmptyBody", "if(0)\nendif", "if_statement:( number:0 ) { } endif"},
    //{"ifBlankLinesBody", "if(0)\n\nendif"},
    //{"ifThenBody", "if(0)\n1\nendif"},
    //{"ifThenComplexBody", "if(0)\nx=1\ny=2\nendif"},
    //{"ifComparisonCondition", "if(1<2)\nendif"},
    //{"ifConjunctiveCondition", "if(1&&2)\nendif"},
    //{"ifElseEmptyBody", "if(0)\nelse\nendif"},
    //{"ifElseBlankLinesBody", "if(0)\n\nelse\n\nendif"},
    //{"ifThenElseBody", "if(0)\nelse\n1\nendif"},
    //{"ifThenBodyElse", "if(0)\n1\nelse\nendif"},
    //{"ifThenElseComplexBody", "if(0)\nx=1\ny=2\nelse\nz=3\nq=4\nendif"},
    //{"ifElseIfEmptyBody", "if(0)\nelseif(1)\nelse\nendif"},
    //{"ifMultipleElseIfEmptyBody", "if(0)\nelseif(0)\nelseif(0)\nelse\nendif"},
    //{"ifElseIfBlankLinesBody", "if(0)\n\nelseif(0)\n\nelse\n\nendif"},
    //{"ifThenElseIfBody", "if(0)\nelseif(0)\n1\nendif"},
    //{"ifThenBodyElseIf", "if(0)\n1\nelseif(0)\nendif"},
    //{"ifThenElseIfComplexBody", "if(0)\nx=1\ny=2\nelseif(1)\nz=3\nq=4\nendif"},
    //{"backslashContinuesStatement", "if(\\\n0)\nendif"},
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

// struct MultiStatementParam
//{
//     std::string_view name;
//     std::string_view text;
// };
//
// inline void PrintTo(const MultiStatementParam &param, std::ostream *os)
//{
//     *os << param.name;
// }
//
// static MultiStatementParam s_multi_statements[]{
//     {"statements", "3\n4\n"},
//     {"assignmentStatements", "z=3\nz=4\n"},
//     {"assignmentWithComments", "z=3; comment\nz=4; another comment\n"},
//     {"assignmentWithBlankLines", "z=3; comment\n\r\n\nz=4; another comment\n"},
//     {"commaSeparatedStatements", "3,4"},
//     {"commaSeparatedAssignmentStatements", "z=3,z=4"},
//     {"mixedNewlineAndCommaSeparators", "z=3,z=4\nz=5"},
//     {"commaWithSpaces", "3 , 4"},
// };
//
// class MultiStatements : public TestWithParam<MultiStatementParam>
//{
// };
//
// TEST_P(MultiStatements, parse)
//{
//     const MultiStatementParam &param{GetParam()};
//     const FormulaPtr result{create_descent_formula(param.text)};
//
//     ASSERT_TRUE(result);
//     EXPECT_FALSE(result->get_section(Section::INITIALIZE));
//     EXPECT_TRUE(result->get_section(Section::ITERATE));
//     EXPECT_TRUE(result->get_section(Section::BAILOUT));
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, MultiStatements, ValuesIn(s_multi_statements));
//
// TEST(TestDescentParse, initializeIterateBailout)
//{
//     const FormulaPtr result{create_descent_formula("z=pixel:z=z*z+pixel,|z|>4")};
//
//     ASSERT_TRUE(result);
//     EXPECT_TRUE(result->get_section(Section::INITIALIZE));
//     EXPECT_TRUE(result->get_section(Section::ITERATE));
//     EXPECT_TRUE(result->get_section(Section::BAILOUT));
// }
//
// TEST(TestDescentParse, statementSequenceInitialize)
//{
//     const FormulaPtr result{create_descent_formula("z=pixel,d=0:z=z*z+pixel,|z|>4")};
//
//     ASSERT_TRUE(result);
//     EXPECT_TRUE(result->get_section(Section::INITIALIZE));
//     EXPECT_TRUE(result->get_section(Section::ITERATE));
//     EXPECT_TRUE(result->get_section(Section::BAILOUT));
// }
//
// static std::vector<std::string> s_read_only_vars{
//     "p1", "p2", "p3", "p4", "p5",                       //
//     "pixel", "lastsqr", "rand", "pi", "e",              //
//     "maxit", "scrnmax", "scrnpix", "whitesq", "ismand", //
//     "center", "magxmag", "rotskew",                     //
// };
//
// class ReadOnlyVariables : public TestWithParam<std::string>
//{
// };
//
// TEST_P(ReadOnlyVariables, notAssignable)
//{
//     const FormulaPtr result{create_descent_formula(GetParam() + "=1")};
//
//     ASSERT_FALSE(result);
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, ReadOnlyVariables, ValuesIn(s_read_only_vars));
//
// static std::vector<std::string> s_functions{
//     "sin", "cos", "sinh", "cosh", "cosxx",      //
//     "tan", "cotan", "tanh", "cotanh", "sqr",    //
//     "log", "exp", "abs", "conj", "real",        //
//     "imag", "flip", "fn1", "fn2", "fn3",        //
//     "fn4", "srand", "asin", "acos", "asinh",    //
//     "acosh", "atan", "atanh", "sqrt", "cabs",   //
//     "floor", "ceil", "trunc", "round", "ident", //
//     "one", "zero",                              //
// };
//
// class Functions : public TestWithParam<std::string>
//{
// };
//
// TEST_P(Functions, notAssignable)
//{
//     const FormulaPtr result{create_descent_formula(GetParam() + "=1")};
//
//     ASSERT_FALSE(result);
// }
//
// TEST_P(Functions, functionOne)
//{
//     const FormulaPtr result{create_descent_formula(GetParam() + "(1)")};
//
//     ASSERT_TRUE(result);
//     EXPECT_TRUE(result->get_section(Section::BAILOUT));
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, Functions, ValuesIn(s_functions));
//
// class ReservedWords : public TestWithParam<std::string>
//{
// };
//
// TEST_P(ReservedWords, notAssignable)
//{
//     const FormulaPtr result1{create_descent_formula(GetParam() + "=1")};
//     const FormulaPtr result2{create_descent_formula(GetParam() + "2=1")};
//
//     ASSERT_FALSE(result1);
//     ASSERT_TRUE(result2);
//     ASSERT_TRUE(result2->get_section(Section::BAILOUT));
// }
//
// static std::string s_reserved_words[]{
//     "if",
//     "else",
//     "elseif",
//     "endif",
// };
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, ReservedWords, ValuesIn(s_reserved_words));
//
// struct ParseFailureParam
//{
//     std::string_view name;
//     std::string_view text;
// };
//
// inline void PrintTo(const ParseFailureParam &param, std::ostream *os)
//{
//     *os << param.name;
// }
//
// static ParseFailureParam s_parse_failures[]{
//     {"ifWithoutEndIf", "if(1)"},
//     {"ifElseWithoutEndIf", "if(1)\nelse"},
//     {"ifElseIfWithoutEndIf", "if(1)\nelseif(0)"},
//     {"ifElseIfElseWithoutEndIf", "if(1)\nelseif(0)\nelse"},
//     {"builtinSectionBogus", "builtin:type=0"},
// };
//
// class ParseFailures : public TestWithParam<ParseFailureParam>
//{
// };
//
// TEST_P(ParseFailures, parse)
//{
//     const ParseFailureParam &param{GetParam()};
//     const FormulaPtr result{create_descent_formula(param.text)};
//
//     EXPECT_FALSE(result);
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, ParseFailures, ValuesIn(s_parse_failures));
//
// struct SingleSectionParam
//{
//     std::string_view name;
//     std::string_view text;
//     Section section;
// };
//
// inline void PrintTo(const SingleSectionParam &param, std::ostream *os)
//{
//     *os << param.name;
// }
//
// static SingleSectionParam s_single_sections[]{
//     {"globalSection", "global:1", Section::PER_IMAGE},
//     {"initSection", "init:1", Section::INITIALIZE},
//     {"loopSection", "loop:1", Section::ITERATE},
//     {"bailoutSection", "bailout:1", Section::BAILOUT},
//     {"perturbInitSection", "perturbinit:1", Section::PERTURB_INITIALIZE},
//     {"perturbLoopSection", "perturbloop:1", Section::PERTURB_ITERATE},
//     {"defaultSection", "default:angle=0", Section::DEFAULT},
//     {"switchSection", "switch:1", Section::SWITCH},
// };
//
// class SingleSections : public TestWithParam<SingleSectionParam>
//{
// };
//
// TEST_P(SingleSections, parse)
//{
//     const SingleSectionParam &param{GetParam()};
//
//     const FormulaPtr result{create_descent_formula(param.text)};
//
//     ASSERT_TRUE(result);
//     EXPECT_TRUE(result->get_section(param.section));
// }
//
// INSTANTIATE_TEST_SUITE_P(TestDescentParse, SingleSections, ValuesIn(s_single_sections));
//
// TEST(TestDescentParse, builtinSectionMandelbrot)
//{
//     const FormulaPtr result{create_descent_formula("builtin:type=1")};
//
//     ASSERT_TRUE(result);
//     const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
//     EXPECT_TRUE(builtin);
//     EXPECT_EQ("setting:type=1\n", to_string(builtin));
// }
//
// TEST(TestDescentParse, builtinSectionJulia)
//{
//     const FormulaPtr result{create_descent_formula("builtin:type=2")};
//
//     ASSERT_TRUE(result);
//     const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
//     EXPECT_TRUE(builtin);
//     EXPECT_EQ("setting:type=2\n", to_string(builtin));
// }
//
// struct DefaultSectionParam
//{
//     std::string_view name;
//     std::string_view text;
//     std::string_view expected;
// };
//
// inline void PrintTo(const DefaultSectionParam &param, std::ostream *os)
//{
//     *os << param.name;
// }
//
// static DefaultSectionParam s_default_values[]{
//     {"angle", "default:angle=0", "setting:angle=0\n"},                  //
//     {"center", "default:center=(-0.5,0)", "setting:center=(-0.5,0)\n"}, //
//     {"helpFile", R"(default:helpfile="HelpFile.html")",
//         R"(setting:helpfile="HelpFile.html")"
//         "\n"},
//     {"helpTopic", R"(default:helptopic="DivideBrot5")",
//         R"(setting:helptopic="DivideBrot5")"
//         "\n"},
//     {"magn", "default:magn=4.5", "setting:magn=4.5\n"},                            //
//     {"maxIter", "default:maxiter=256", "setting:maxiter=256\n"},                   //
//     {"methodGuessing", "default:method=guessing", "setting:method=guessing\n"},    //
//     {"methodMultiPass", "default:method=multipass", "setting:method=multipass\n"}, //
//     {"methodOnePass", "default:method=onepass", "setting:method=onepass\n"},       //
//     {"periodicity0", "default:periodicity=0", "setting:periodicity=0\n"},          //
//     {"periodicity1", "default:periodicity=1", "setting:periodicity=1\n"},          //
//     {"periodicity2", "default:periodicity=2", "setting:periodicity=2\n"},          //
//     {"periodicity3", "default:periodicity=3", "setting:periodicity=3\n"},          //
//     {"perturbFalse", "default:perturb=false", "setting:perturb=\"false\"\n"},      //
//     {"perturbTrue", "default:perturb=true", "setting:perturb=\"true\"\n"},         //
//     {"perturbExpr", "default:perturb=@power==(2,0) || @power == (3,0) || @power == (4,0)",
//         "setting:perturb=\"@power==(2,0) || @power == (3,0) || @power == (4,0)\"\n"}, //
//     {"precisionNumber", "default:precision=30", "setting:precision=\"30\"\n"},        //
//     {"precisionExpr", "default:precision = round(log(@fracmagn) / log(10))",
//         "setting:precision=\"round(log(@fracmagn) / log(10))\"\n"},                                   //
//     {"ratingRecommended", "default:rating=recommended", "setting:rating=recommended\n"},              //
//     {"ratingAverage", "default:rating=average", "setting:rating=average\n"},                          //
//     {"ratingNotRecommended", "default:rating=notRecommended", "setting:rating=notRecommended\n"},     //
//     {"renderTrue", "default:render=true", "setting:render=true\n"},                                   //
//     {"renderFalse", "default:render=false", "setting:render=false\n"},                                //
//     {"skew", "default:skew=-4.5", "setting:skew=-4.5\n"},                                             //
//     {"stretch", "default:stretch=4.5", "setting:stretch=4.5\n"},                                      //
//     {"title", R"(default:title="This is a fancy one!")", "setting:title=\"This is a fancy one!\"\n"}, //
//     {"boolParamBlock",
//         "default:bool param foo\n"
//         "endparam",
//         "param_block:bool,foo {\n"
//         "}\n"}, //
// };
//
// class DefaultSection : public TestWithParam<DefaultSectionParam>
//{
// };
//
// TEST_P(DefaultSection, parse)
//{
//     const DefaultSectionParam &param{GetParam()};
//
//     const FormulaPtr result{create_descent_formula(param.text)};
//
//     ASSERT_TRUE(result);
//     const ast::Expr &section{result->get_section(Section::DEFAULT)};
//     EXPECT_TRUE(section);
//     EXPECT_EQ(param.expected, to_string(section));
// }
//
// INSTANTIATE_TEST_SUITE_P(TestFormualParse, DefaultSection, ValuesIn(s_default_values));
//
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
