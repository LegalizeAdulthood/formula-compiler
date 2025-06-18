#include <formula/formula.h>

#include <gtest/gtest.h>

#include <cmath>

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
