// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

using namespace formula;
using namespace testing;

TEST(TestFormulaParse, constant)
{
    const FormulaPtr result{parse("1")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, identifier)
{
    const FormulaPtr result{parse("z2")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, parenExpr)
{
    const FormulaPtr result{parse("(z)")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, add)
{
    const FormulaPtr result{parse("1+2")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, subtract)
{
    const FormulaPtr result{parse("1-2")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, multiply)
{
    const FormulaPtr result{parse("1*2")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, divide)
{
    const FormulaPtr result{parse("1/2")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, multiplyAdd)
{
    const FormulaPtr result{parse("1*2+4")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, parenthesisExpr)
{
    const FormulaPtr result{parse("1*(2+4)")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, unaryMinus)
{
    const FormulaPtr result{parse("-(1)")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, unaryPlus)
{
    const FormulaPtr result{parse("+(1)")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, unaryMinusNegativeOne)
{
    const FormulaPtr result{parse("--1")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, addAddAdd)
{
    const FormulaPtr result{parse("1+1+1")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, capitalLetterInIdentifier)
{
    const FormulaPtr result{parse("A")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, numberInIdentifier)
{
    const FormulaPtr result{parse("a1")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, underscoreInIdentifier)
{
    const FormulaPtr result{parse("A_1")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, invalidIdentifier)
{
    const FormulaPtr result1{parse("1a")};
    const FormulaPtr result2{parse("_a")};

    EXPECT_FALSE(result1);
    EXPECT_FALSE(result2);
}

TEST(TestFormulaParse, power)
{
    const FormulaPtr result{parse("2^3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, chainedPower)
{
    const FormulaPtr result{parse("2^2^2")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, assignment)
{
    const FormulaPtr result{parse("z=4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, assignmentLongVariable)
{
    const FormulaPtr result{parse("this_is_another4_variable_name2=4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, modulus)
{
    const FormulaPtr result{parse("|-3.0|")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareLess)
{
    const FormulaPtr result{parse("4<3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareLessPrecedence)
{
    const FormulaPtr result{parse("3<z=4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareLessEqual)
{
    const FormulaPtr result{parse("4<=3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareGreater)
{
    const FormulaPtr result{parse("4>3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareAssociatesLeft)
{
    const FormulaPtr result{parse("4>3<4")}; // This is equivalent to (4 > 3) < 4

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareGreaterEqual)
{
    const FormulaPtr result{parse("4>=3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareEqual)
{
    const FormulaPtr result{parse("4==3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, compareNotEqual)
{
    const FormulaPtr result{parse("4!=3")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, logicalAnd)
{
    const FormulaPtr result{parse("4==3&&5==6")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, logicalOr)
{
    const FormulaPtr result{parse("4==3||5==6")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ignoreComments)
{
    const FormulaPtr result{parse("3; z=6 oh toodlee doo")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ignoreCommentsLF)
{
    const FormulaPtr result{parse("3; z=6 oh toodlee doo\n")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ignoreCommentsCRLF)
{
    const FormulaPtr result{parse("3; z=6 oh toodlee doo\r\n")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, statements)
{
    const FormulaPtr result{parse("3\n"
                                  "4\n")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, assignmentStatements)
{
    const FormulaPtr result{parse("z=3\n"
                                  "z=4\n")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, assignmentWithComments)
{
    const FormulaPtr result{parse("z=3; comment\n"
                                  "z=4; another comment\n")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, assignmentWithBlankLines)
{
    const FormulaPtr result{parse("z=3; comment\n"
                                  "\r\n"
                                  "\n"
                                  "z=4; another comment\n")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, initializeIterateBailout)
{
    const FormulaPtr result{parse("z=pixel:z=z*z+pixel,|z|>4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

static std::vector<std::string> s_read_only_vars{
    "p1", "p2", "p3", "p4", "p5",                       //
    "pixel", "lastsqr", "rand", "pi", "e",              //
    "maxit", "scrnmax", "scrnpix", "whitesq", "ismand", //
    "center", "magxmag", "rotskew",                     //
};

class ReadOnlyVariables : public TestWithParam<std::string>
{
};

TEST_P(ReadOnlyVariables, notAssignable)
{
    const FormulaPtr result{parse(GetParam() + "=1")};

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

class Functions : public TestWithParam<std::string>
{
};

TEST_P(Functions, notAssignable)
{
    const FormulaPtr result{parse(GetParam() + "=1")};

    ASSERT_FALSE(result);
}

TEST_P(Functions, functionOne)
{
    const FormulaPtr result{parse(GetParam() + "(1)")};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, Functions, ValuesIn(s_functions));

class ReservedWords : public TestWithParam<std::string>
{
};

TEST_P(ReservedWords, notAssignable)
{
    const FormulaPtr result1{parse(GetParam() + "=1")};
    const FormulaPtr result2{parse(GetParam() + "2=1")};

    ASSERT_FALSE(result1);
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result2->get_iterate());
}

static std::string s_reserved_words[]{
    "if",
    "else",
    "elseif",
    "endif",
};

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ReservedWords, ValuesIn(s_reserved_words));

TEST(TestFormulaParse, reservedVariablePrefixToUserVariable)
{
    const FormulaPtr result{parse("e2=1")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, reservedFunctionPrefixToUserVariable)
{
    const FormulaPtr result{parse("sine=1")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, reservedKeywordPrefixToUserVariable)
{
    const FormulaPtr result{parse("if1=1")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifWithoutEndIf)
{
    const FormulaPtr result{parse("if(1)")};

    EXPECT_FALSE(result);
}

TEST(TestFormulaParse, ifEmptyBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifBlankLinesBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "1\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenComplexBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "x=1\n"
                                  "y=2\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifComparisonCondition)
{
    const FormulaPtr result{parse("if(1<2)\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifConjunctiveCondition)
{
    const FormulaPtr result{parse("if(1&&2)\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifElseWithoutEndIf)
{
    const FormulaPtr result{parse("if(1)\n"
                                  "else")};

    EXPECT_FALSE(result);
}

TEST(TestFormulaParse, ifElseEmptyBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "else\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifElseBlankLinesBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "\n"
                                  "else\n"
                                  "\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenElseBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "else\n"
                                  "1\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenBodyElse)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "1\n"
                                  "else\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenElseComplexBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "x=1\n"
                                  "y=2\n"
                                  "else\n"
                                  "z=3\n"
                                  "q=4\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifElseIfWithoutEndIf)
{
    const FormulaPtr result{parse("if(1)\n"
                                  "elseif(0)")};

    EXPECT_FALSE(result);
}

TEST(TestFormulaParse, ifElseIfElseWithoutEndIf)
{
    const FormulaPtr result{parse("if(1)\n"
                                  "elseif(0)\n"
                                  "else")};

    EXPECT_FALSE(result);
}

TEST(TestFormulaParse, ifElseIfEmptyBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "elseif(1)\n"
                                  "else\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifMultipleElseIfEmptyBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "elseif(0)\n"
                                  "elseif(0)\n"
                                  "else\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifElseIfBlankLinesBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "\n"
                                  "elseif(0)\n"
                                  "\n"
                                  "else\n"
                                  "\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenElseIfBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "elseif(0)\n"
                                  "1\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenBodyElseIf)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "1\n"
                                  "elseif(0)\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, ifThenElseIfComplexBody)
{
    const FormulaPtr result{parse("if(0)\n"
                                  "x=1\n"
                                  "y=2\n"
                                  "elseif(1)\n"
                                  "z=3\n"
                                  "q=4\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, backslashContinuesStatement)
{
    const FormulaPtr result{parse("if(\\\n"
                                  "0)\n"
                                  "endif")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, commaSeparatedStatements)
{
    const FormulaPtr result{parse("3,4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, commaSeparatedAssignmentStatements)
{
    const FormulaPtr result{parse("z=3,z=4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, mixedNewlineAndCommaSeparators)
{
    const FormulaPtr result{parse("z=3,z=4\n"
                                  "z=5")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}

TEST(TestFormulaParse, commaWithSpaces)
{
    const FormulaPtr result{parse("3 , 4")};

    EXPECT_TRUE(result);
    ASSERT_TRUE(result->get_iterate());
}
