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
