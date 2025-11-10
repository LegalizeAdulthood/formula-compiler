// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "NodeFormatter.h"

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

class ReadOnlyVariables : public TestWithParam<std::string>
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

class SingleSections : public TestWithParam<SingleSectionParam>
{
};

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

class DefaultSection : public TestWithParam<DefaultSectionParam>
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
    {"constant", "1"},
    {"identifier", "z2"},
    {"parenExpr", "(z)"},
    {"add", "1+2"},
    {"subtract", "1-2"},
    {"multiply", "1*2"},
    {"divide", "1/2"},
    {"multiplyAdd", "1*2+4"},
    {"parenthesisExpr", "1*(2+4)"},
    {"unaryMinus", "-(1)"},
    {"unaryPlus", "+(1)"},
    {"unaryMinusNegativeOne", "--1"},
    {"addAddAdd", "1+1+1"},
    {"capitalLetterInIdentifier", "A"},
    {"numberInIdentifier", "a1"},
    {"underscoreInIdentifier", "A_1"},
    {"power", "2^3"},
    {"powerLeftAssociative", "2^2^2"},
    {"assignment", "z=4"},
    {"assignmentLongVariable", "this_is_another4_variable_name2=4"},
    {"modulus", "|-3.0|"},
    {"compareLess", "4<3"},
    {"compareLessPrecedence", "3<z=4"},
    {"compareLessEqual", "4<=3"},
    {"compareGreater", "4>3"},
    {"compareAssociatesLeft", "4>3<4"},
    {"compareGreaterEqual", "4>=3"},
    {"compareEqual", "4==3"},
    {"compareNotEqual", "4!=3"},
    {"logicalAnd", "4==3&&5==6"},
    {"logicalOr", "4==3||5==6"},
    {"ignoreComments", "3; z=6 oh toodlee doo"},
    {"ignoreCommentsLF", "3; z=6 oh toodlee doo\n"},
    {"ignoreCommentsCRLF", "3; z=6 oh toodlee doo\r\n"},
    {"reservedVariablePrefixToUserVariable", "e2=1"},
    {"reservedFunctionPrefixToUserVariable", "sine=1"},
    {"reservedKeywordPrefixToUserVariable", "if1=1"},
    {"ifEmptyBody", "if(0)\nendif"},
    {"ifBlankLinesBody", "if(0)\n\nendif"},
    {"ifThenBody", "if(0)\n1\nendif"},
    {"ifThenComplexBody", "if(0)\nx=1\ny=2\nendif"},
    {"ifComparisonCondition", "if(1<2)\nendif"},
    {"ifConjunctiveCondition", "if(1&&2)\nendif"},
    {"ifElseEmptyBody", "if(0)\nelse\nendif"},
    {"ifElseBlankLinesBody", "if(0)\n\nelse\n\nendif"},
    {"ifThenElseBody", "if(0)\nelse\n1\nendif"},
    {"ifThenBodyElse", "if(0)\n1\nelse\nendif"},
    {"ifThenElseComplexBody", "if(0)\nx=1\ny=2\nelse\nz=3\nq=4\nendif"},
    {"ifElseIfEmptyBody", "if(0)\nelseif(1)\nelse\nendif"},
    {"ifMultipleElseIfEmptyBody", "if(0)\nelseif(0)\nelseif(0)\nelse\nendif"},
    {"ifElseIfBlankLinesBody", "if(0)\n\nelseif(0)\n\nelse\n\nendif"},
    {"ifThenElseIfBody", "if(0)\nelseif(0)\n1\nendif"},
    {"ifThenBodyElseIf", "if(0)\n1\nelseif(0)\nendif"},
    {"ifThenElseIfComplexBody", "if(0)\nx=1\ny=2\nelseif(1)\nz=3\nq=4\nendif"},
    {"backslashContinuesStatement", "if(\\\n0)\nendif"},
};

TEST_P(SimpleExpressions, parse)
{
    const SimpleExpressionParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text)};

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->get_section(Section::INITIALIZE));
    EXPECT_FALSE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, SimpleExpressions, ValuesIn(s_simple_expressions));

TEST(TestFormulaParse, invalidIdentifier)
{
    const FormulaPtr result1{create_formula("1a")};

    EXPECT_FALSE(result1);
}

static MultiStatementParam s_multi_statements[]{
    {"statements", "3\n4\n"},
    {"assignmentStatements", "z=3\nz=4\n"},
    {"assignmentWithComments", "z=3; comment\nz=4; another comment\n"},
    {"assignmentWithBlankLines", "z=3; comment\n\r\n\nz=4; another comment\n"},
    {"commaSeparatedStatements", "3,4"},
    {"commaSeparatedAssignmentStatements", "z=3,z=4"},
    {"mixedNewlineAndCommaSeparators", "z=3,z=4\nz=5"},
    {"commaWithSpaces", "3 , 4"},
};

TEST_P(MultiStatements, parse)
{
    const MultiStatementParam &param{GetParam()};
    const FormulaPtr result{create_formula(param.text)};

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, MultiStatements, ValuesIn(s_multi_statements));

TEST(TestFormulaParse, initializeIterateBailout)
{
    const FormulaPtr result{create_formula("z=pixel:z=z*z+pixel,|z|>4")};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::INITIALIZE));
    EXPECT_TRUE(result->get_section(Section::ITERATE));
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

TEST(TestFormulaParse, statementSequenceInitialize)
{
    const FormulaPtr result{create_formula("z=pixel,d=0:z=z*z+pixel,|z|>4")};

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

TEST_P(ReadOnlyVariables, notAssignable)
{
    const FormulaPtr result{create_formula(GetParam() + "=1")};

    ASSERT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ReadOnlyVariables, ValuesIn(s_read_only_vars));

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

TEST_P(Functions, notAssignable)
{
    const FormulaPtr result{create_formula(GetParam() + "=1")};

    ASSERT_FALSE(result);
}

TEST_P(Functions, functionOne)
{
    const FormulaPtr result{create_formula(GetParam() + "(1)")};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(Section::BAILOUT));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, Functions, ValuesIn(s_functions));

TEST_P(ReservedWords, notAssignable)
{
    const FormulaPtr result1{create_formula(GetParam() + "=1")};
    const FormulaPtr result2{create_formula(GetParam() + "2=1")};

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
    const FormulaPtr result{create_formula(param.text)};

    EXPECT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ParseFailures, ValuesIn(s_parse_failures));

static SingleSectionParam s_single_sections[]{
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
    const SingleSectionParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text)};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->get_section(param.section));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, SingleSections, ValuesIn(s_single_sections));

TEST(TestFormulaParse, builtinSectionMandelbrot)
{
    const FormulaPtr result{create_formula("builtin:\n"
                                           "type=1\n")};

    ASSERT_TRUE(result);
    const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=1\n", to_string(builtin));
}

TEST(TestFormulaParse, builtinSectionJulia)
{
    const FormulaPtr result{create_formula("builtin:\n"
                                           "type=2\n")};

    ASSERT_TRUE(result);
    const ast::Expr &builtin{result->get_section(Section::BUILTIN)};
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=2\n", to_string(builtin));
}

static DefaultSectionParam s_default_values[]{
    {"angle", "default:\nangle=0\n", "setting:angle=0\n"},                  //
    {"center", "default:\ncenter=(-0.5,0)\n", "setting:center=(-0.5,0)\n"}, //
    {"helpFile", "default:\nhelpfile=\"HelpFile.html\"\n",
        R"(setting:helpfile="HelpFile.html")"
        "\n"},
    {"helpTopic", "default:\nhelptopic=\"DivideBrot5\"\n",
        R"(setting:helptopic="DivideBrot5")"
        "\n"},
    {"magn", "default:\nmagn=4.5\n", "setting:magn=4.5\n"},                            //
    {"maxIter", "default:\nmaxiter=256\n", "setting:maxiter=256\n"},                   //
    {"methodGuessing", "default:\nmethod=guessing\n", "setting:method=guessing\n"},    //
    {"methodMultiPass", "default:\nmethod=multipass\n", "setting:method=multipass\n"}, //
    {"methodOnePass", "default:\nmethod=onepass\n", "setting:method=onepass\n"},       //
    {"periodicity0", "default:\nperiodicity=0\n", "setting:periodicity=0\n"},          //
    {"periodicity1", "default:\nperiodicity=1\n", "setting:periodicity=1\n"},          //
    {"periodicity2", "default:\nperiodicity=2\n", "setting:periodicity=2\n"},          //
    {"periodicity3", "default:\nperiodicity=3\n", "setting:periodicity=3\n"},          //
    {"perturbFalse", "default:\nperturb=false\n", "setting:perturb=false\n"},          //
    {"perturbTrue", "default:\nperturb=true\n", "setting:perturb=true\n"},             //
    {"perturbExpr", "default:\nperturb=power==2 || power == 3\n",
        "setting:perturb={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "number:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "number:3\n"
        "}\n"},
    {"precisionNumber", "default:\nprecision=30\n", "setting:precision={\nnumber:30\n}\n"}, //
    {"precisionExpr", "default:\nprecision = log(fracmagn) / log(10)\n",
        "setting:precision={\n"
        "binary_op:/\n"
        "function_call:log(\n"
        "identifier:fracmagn\n"
        ")\n"
        "function_call:log(\n"
        "number:10\n"
        ")\n"
        "}\n"},
    {"ratingRecommended", "default:\nrating=recommended\n", "setting:rating=recommended\n"},             //
    {"ratingAverage", "default:\nrating=average\n", "setting:rating=average\n"},                         //
    {"ratingNotRecommended", "default:\nrating=notRecommended\n", "setting:rating=notRecommended\n"},    //
    {"renderTrue", "default:\nrender=true\n", "setting:render=true\n"},                                  //
    {"renderFalse", "default:\nrender=false\n", "setting:render=false\n"},                               //
    {"skew", "default:\nskew=-4.5\n", "setting:skew=-4.5\n"},                                            //
    {"stretch", "default:\nstretch=4.5\n", "setting:stretch=4.5\n"},                                     //
    {"title", "default:\ntitle=\"This is a fancy one!\"\n", "setting:title=\"This is a fancy one!\"\n"}, //
    {"boolParamBlock",
        "default:\n"
        "bool param foo\n"
        "endparam\n",
        "param_block:bool,foo {\n"
        "}\n"}, //
};

TEST_P(DefaultSection, parse)
{
    const DefaultSectionParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text)};

    ASSERT_TRUE(result);
    const ast::Expr &section{result->get_section(Section::DEFAULT)};
    EXPECT_TRUE(section);
    EXPECT_EQ(param.expected, to_string(section));
}

INSTANTIATE_TEST_SUITE_P(TestFormualParse, DefaultSection, ValuesIn(s_default_values));

static InvalidSectionParam s_invalid_sections[]{
    {"unknownSection",
        "global:\n"
        "1\n"
        "unknown:\n"
        "1\n"},
    {"globalSectionFirst",
        "builtin:\n"
        "1\n"
        "global:\n"
        "1\n"},
    {"builtinBeforeInit",
        "init:\n"
        "1\n"
        "builtin:\n"
        "1\n"},
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
};

class InvalidSectionOrdering : public TestWithParam<InvalidSectionParam>
{
};

TEST_P(InvalidSectionOrdering, parse)
{
    const InvalidSectionParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text)};

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
                                           "type=\"Julia\"\n")};

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
                                           "type=\"Julia\"\n")};

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
        "type=1\n"
        "perturbinit:\n"
        "1\n"
        "perturbloop:\n"
        "1\n"
        "default:\n"
        "angle=0\n"
        "switch:\n"
        "1\n"},
    {"init",
        "builtin:\n"
        "type=1\n"
        "init:\n"
        "1\n"
        "perturbinit:\n"
        "1\n"
        "perturbloop:\n"
        "1\n"
        "default:\n"
        "angle=0\n"
        "switch:\n"
        "1\n"},
    {"loop",
        "builtin:\n"
        "type=1\n"
        "loop:\n"
        "1\n"
        "perturbinit:\n"
        "1\n"
        "perturbloop:\n"
        "1\n"
        "default:\n"
        "angle=0\n"
        "switch:\n"
        "1\n"},
    {"bailout",
        "builtin:\n"
        "type=1\n"
        "bailout:\n"
        "1\n"
        "perturbinit:\n"
        "1\n"
        "perturbloop:\n"
        "1\n"
        "default:\n"
        "angle=0\n"
        "switch:\n"
        "1\n"},
};

TEST_P(BuiltinDisallows, parse)
{
    const BuiltinDisallowsParam &param{GetParam()};

    const FormulaPtr result{create_formula(param.text)};

    ASSERT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, BuiltinDisallows, ValuesIn(s_builtin_disallows));

} // namespace formula::test
