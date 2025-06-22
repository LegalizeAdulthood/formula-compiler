#include <formula/formula.h>

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

using namespace testing;

TEST(TestFormulaParse, constant)
{
    ASSERT_TRUE(formula::parse("1"));
}

TEST(TestFormulaParse, identifier)
{
    ASSERT_TRUE(formula::parse("z2"));
}

TEST(TestFormulaParse, parenExpr)
{
    ASSERT_TRUE(formula::parse("(z)"));
}

TEST(TestFormulaParse, add)
{
    ASSERT_TRUE(formula::parse("1+2"));
}

TEST(TestFormulaParse, subtract)
{
    ASSERT_TRUE(formula::parse("1-2"));
}

TEST(TestFormulaParse, multiply)
{
    ASSERT_TRUE(formula::parse("1*2"));
}

TEST(TestFormulaParse, divide)
{
    ASSERT_TRUE(formula::parse("1/2"));
}

TEST(TestFormulaParse, multiplyAdd)
{
    ASSERT_TRUE(formula::parse("1*2+4"));
}

TEST(TestFormulaParse, parenthesisExpr)
{
    ASSERT_TRUE(formula::parse("1*(2+4)"));
}

TEST(TestFormulaParse, unaryMinus)
{
    ASSERT_TRUE(formula::parse("-(1)"));
}

TEST(TestFormulaParse, unaryPlus)
{
    ASSERT_TRUE(formula::parse("+(1)"));
}

TEST(TestFormulaParse, unaryMinusNegativeOne)
{
    ASSERT_TRUE(formula::parse("--1"));
}

TEST(TestFormulaParse, addAddAdd)
{
    ASSERT_TRUE(formula::parse("1+1+1"));
}

TEST(TestFormulaParse, capitalLetterInIdentifier)
{
    ASSERT_TRUE(formula::parse("A"));
}

TEST(TestFormulaParse, numberInIdentifier)
{
    ASSERT_TRUE(formula::parse("a1"));
}

TEST(TestFormulaParse, underscoreInIdentifier)
{
    ASSERT_TRUE(formula::parse("A_1"));
}

TEST(TestFormulaParse, invalidIdentifier)
{
    EXPECT_FALSE(formula::parse("1a"));
    EXPECT_FALSE(formula::parse("_a"));
}

TEST(TestFormulaParse, power)
{
    EXPECT_TRUE(formula::parse("2^3"));
}

TEST(TestFormulaParse, chainedPower)
{
    EXPECT_TRUE(formula::parse("2^2^2"));
}

TEST(TestFormulaParse, assignment)
{
    EXPECT_TRUE(formula::parse("z=4"));
}

TEST(TestFormulaParse, assignmentLongVariable)
{
    EXPECT_TRUE(formula::parse("this_is_another4_variable_name2=4"));
}

TEST(TestFormulaParse, modulus)
{
    EXPECT_TRUE(formula::parse("|-3.0|"));
}

TEST(TestFormulaParse, compareLess)
{
    EXPECT_TRUE(formula::parse("4<3"));
}

TEST(TestFormulaParse, compareLessPrecedence)
{
    EXPECT_TRUE(formula::parse("3<z=4"));
}

TEST(TestFormulaParse, compareLessEqual)
{
    EXPECT_TRUE(formula::parse("4<=3"));
}

TEST(TestFormulaParse, compareGreater)
{
    EXPECT_TRUE(formula::parse("4>3"));
}

TEST(TestFormulaParse, compareAssociatesLeft)
{
    EXPECT_TRUE(formula::parse("4>3<4")); // This is equivalent to (4 > 3) < 4
}

TEST(TestFormulaParse, compareGreaterEqual)
{
    EXPECT_TRUE(formula::parse("4>=3"));
}

TEST(TestFormulaParse, compareEqual)
{
    EXPECT_TRUE(formula::parse("4==3"));
}

TEST(TestFormulaParse, compareNotEqual)
{
    EXPECT_TRUE(formula::parse("4!=3"));
}

TEST(TestFormulaParse, logicalAnd)
{
    EXPECT_TRUE(formula::parse("4==3&&5==6"));
}

TEST(TestFormulaParse, logicalOr)
{
    EXPECT_TRUE(formula::parse("4==3||5==6"));
}

TEST(TestFormulaParse, ignoreComments)
{
    EXPECT_TRUE(formula::parse("3; z=6 oh toodlee doo"));
}

TEST(TestFormulaParse, ignoreCommentsLF)
{
    EXPECT_TRUE(formula::parse("3; z=6 oh toodlee doo\n"));
}

TEST(TestFormulaParse, ignoreCommentsCRLF)
{
    EXPECT_TRUE(formula::parse("3; z=6 oh toodlee doo\r\n"));
}

TEST(TestFormulaParse, statements)
{
    EXPECT_TRUE(formula::parse("3\n"
                               "4\n"));
}

TEST(TestFormulaParse, assignmentStatements)
{
    EXPECT_TRUE(formula::parse("z=3\n"
                               "z=4\n"));
}

TEST(TestFormulaParse, assignmentWithComments)
{
    EXPECT_TRUE(formula::parse("z=3; comment\n"
                               "z=4; another comment\n"));
}

TEST(TestFormulaParse, assignmentWithBlankLines)
{
    EXPECT_TRUE(formula::parse("z=3; comment\n"
                               "\r\n"
                               "\n"
                               "z=4; another comment\n"));
}

TEST(TestFormulaParse, initializeIterateBailout)
{
    EXPECT_TRUE(formula::parse("z=pixel:z=z*z+pixel,|z|>4"));
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
    ASSERT_FALSE(formula::parse(GetParam() + "=1"));
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
    ASSERT_FALSE(formula::parse(GetParam() + "=1"));
}

TEST_P(Functions, functionOne)
{
    ASSERT_TRUE(formula::parse(GetParam() + "(1)"));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, Functions, ValuesIn(s_functions));

class ReservedWords : public TestWithParam<std::string>
{
};

TEST_P(ReservedWords, notAssignable)
{
    ASSERT_FALSE(formula::parse(GetParam() + "=1"));
    ASSERT_TRUE(formula::parse(GetParam() + "2=1"));
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
    EXPECT_TRUE(formula::parse("e2=1"));
}

TEST(TestFormulaParse, reservedFunctionPrefixToUserVariable)
{
    EXPECT_TRUE(formula::parse("sine=1"));
}

TEST(TestFormulaParse, reservedKeywordPrefixToUserVariable)
{
    EXPECT_TRUE(formula::parse("if1=1"));
}

TEST(TestFormulaParse, ifWithoutEndIf)
{
    EXPECT_FALSE(formula::parse("if(1)"));
}

TEST(TestFormulaParse, ifEmptyBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "endif"));
}

TEST(TestFormulaParse, ifBlankLinesBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "1\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenComplexBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "x=1\n"
                               "y=2\n"
                               "endif"));
}

TEST(TestFormulaParse, ifComparisonCondition)
{
    EXPECT_TRUE(formula::parse("if(1<2)\n"
                               "endif"));
}

TEST(TestFormulaParse, ifConjunctiveCondition)
{
    EXPECT_TRUE(formula::parse("if(1&&2)\n"
                               "endif"));
}

TEST(TestFormulaParse, ifElseWithoutEndIf)
{
    EXPECT_FALSE(formula::parse("if(1)\n"
                                "else"));
}

TEST(TestFormulaParse, ifElseEmptyBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "else\n"
                               "endif"));
}

TEST(TestFormulaParse, ifElseBlankLinesBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "\n"
                               "else\n"
                               "\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenElseBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "else\n"
                               "1\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenBodyElse)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "1\n"
                               "else\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenElseComplexBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "x=1\n"
                               "y=2\n"
                               "else\n"
                               "z=3\n"
                               "q=4\n"
                               "endif"));
}

TEST(TestFormulaParse, ifElseIfWithoutEndIf)
{
    EXPECT_FALSE(formula::parse("if(1)\n"
                                "elseif(0)"));
}

TEST(TestFormulaParse, ifElseIfElseWithoutEndIf)
{
    EXPECT_FALSE(formula::parse("if(1)\n"
                                "elseif(0)\n"
                                "else"));
}

TEST(TestFormulaParse, ifElseIfEmptyBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "elseif(1)\n"
                               "else\n"
                               "endif"));
}

TEST(TestFormulaParse, ifMultipleElseIfEmptyBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "elseif(0)\n"
                               "elseif(0)\n"
                               "else\n"
                               "endif"));
}

TEST(TestFormulaParse, ifElseIfBlankLinesBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "\n"
                               "elseif(0)\n"
                               "\n"
                               "else\n"
                               "\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenElseIfBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "elseif(0)\n"
                               "1\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenBodyElseIf)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "1\n"
                               "elseif(0)\n"
                               "endif"));
}

TEST(TestFormulaParse, ifThenElseIfComplexBody)
{
    EXPECT_TRUE(formula::parse("if(0)\n"
                               "x=1\n"
                               "y=2\n"
                               "elseif(1)\n"
                               "z=3\n"
                               "q=4\n"
                               "endif"));
}
