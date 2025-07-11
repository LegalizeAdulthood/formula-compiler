// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/formula.h>

#include "function-call.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <iostream>
#include <iterator>

using namespace testing;

TEST(TestFormulaInterpret, one)
{
    ASSERT_EQ(1.0, formula::parse("1")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, two)
{
    ASSERT_EQ(2.0, formula::parse("2")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, add)
{
    ASSERT_EQ(2.0, formula::parse("1+1")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, unaryMinusNegativeOne)
{
    ASSERT_EQ(1.0, formula::parse("--1")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, multiply)
{
    ASSERT_EQ(6.0, formula::parse("2*3")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, divide)
{
    ASSERT_EQ(3.0, formula::parse("6/2")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, addMultiply)
{
    const auto formula{formula::parse("1+3*2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(7.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, multiplyAdd)
{
    const auto formula{formula::parse("3*2+1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(7.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, addAddAdd)
{
    const auto formula{formula::parse("1+1+1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, mulMulMul)
{
    const auto formula{formula::parse("2*2*2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(8.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, twoPi)
{
    const auto formula{formula::parse("2*pi")};
    ASSERT_TRUE(formula);

    ASSERT_NEAR(6.28318, formula->interpret(formula::ITERATE).re, 1e-5);
}

TEST(TestFormulaInterpret, unknownIdentifierIsZero)
{
    const auto formula{formula::parse("a")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, setComplexValue)
{
    const auto formula{formula::parse("0")};
    ASSERT_TRUE(formula);
    formula->set_value("a", {1.0, 2.0});

    const formula::Complex result{formula->get_value("a")};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(2.0, result.im);
}

TEST(TestFormulaInterpret, setSymbolValue)
{
    const auto formula{formula::parse("a*a + b*b")};
    ASSERT_TRUE(formula);
    formula->set_value("a", {2.0, 0.0});
    formula->set_value("b", {3.0, 0.0});

    ASSERT_NEAR(13.0, formula->interpret(formula::ITERATE).re, 1e-5);
}

TEST(TestFormulaInterpret, power)
{
    const auto formula{formula::parse("2^3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(8.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, chainedPower)
{
    // TODO: is power operator left associative or right associative?
    const auto formula{formula::parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);

    ASSERT_EQ(64.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, powerPrecedence)
{
    const auto formula{formula::parse("2*3^2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(18.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, getValue)
{
    const auto formula{formula::parse("1")};
    EXPECT_TRUE(formula);

    EXPECT_EQ(std::exp(1.0), formula->get_value("e").re);
    EXPECT_EQ(0.0, formula->get_value("a").re); // unknown identifier should return 0.0
}

TEST(TestFormulaInterpret, assignment)
{
    const auto formula{formula::parse("z=4+2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(6.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(6.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, assignmentParens)
{
    const auto formula{formula::parse("(z=4)+2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(6.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, chainedAssignment)
{
    const auto formula{formula::parse("z1=z2=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(3.0, formula->get_value("z1").re);
    ASSERT_EQ(3.0, formula->get_value("z2").re);
}

TEST(TestFormulaInterpret, modulus)
{
    const auto formula{formula::parse("|-3.0 + flip(-2)|")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_EQ(13.0, result.re);
    ASSERT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpret, conjugate)
{
    const auto formula{formula::parse("conj(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(-4.0, result.im);
}

TEST(TestFormulaInterpret, identity)
{
    const auto formula{formula::parse("ident(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(4.0, result.im);
}

TEST(TestFormulaInterpret, compareLessFalse)
{
    const auto formula{formula::parse("4<3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpret, compareLessTrue)
{
    const auto formula{formula::parse("3<4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpret, compareLessPrecedence)
{
    const auto formula{formula::parse("3<z=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, compareLessEqualTrueEquality)
{
    const auto formula{formula::parse("3<=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, compareLessEqualTrueLess)
{
    const auto formula{formula::parse("3<=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, compareLessEqualFalse)
{
    const auto formula{formula::parse("3<=2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, compareAssociatesLeft)
{
    const auto formula{formula::parse("4<3<4")}; // (4 < 3) < 4
    ASSERT_TRUE(formula);

    ASSERT_EQ(
        1.0, formula->interpret(formula::ITERATE).re); // (4 < 3) is false (0.0), (0 < 4) is true, so the result is 1.0
}

TEST(TestFormulaInterpret, compareGreaterFalse)
{
    const auto formula{formula::parse("3>4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpret, compareGreaterTrue)
{
    const auto formula{formula::parse("4>3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpret, compareGreaterEqualTrueEquality)
{
    const auto formula{formula::parse("3>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, compareGreaterEqualTrueGreater)
{
    const auto formula{formula::parse("4>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, compareGreaterEqualFalse)
{
    const auto formula{formula::parse("2>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, compareEqualTrue)
{
    const auto formula{formula::parse("3==3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpret, compareEqualFalse)
{
    const auto formula{formula::parse("3==4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpret, compareNotEqualTrue)
{
    const auto formula{formula::parse("3!=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpret, compareNotEqualFalse)
{
    const auto formula{formula::parse("3!=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpret, logicalAndTrue)
{
    const auto formula{formula::parse("1&&1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpret, logicalAndFalse)
{
    const auto formula{formula::parse("1&&0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpret, logicalAndPrecedence)
{
    const auto formula{formula::parse("1+2&&3+4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // (1+2) && (3+4) is true
}

TEST(TestFormulaInterpret, logicalAndShortCircuitTrue)
{
    const auto formula{formula::parse("0&&z=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // 0 is false, so the second part is not evaluated

    ASSERT_EQ(0.0, formula->get_value("z").re);              // z should not be set
}

TEST(TestFormulaInterpret, logicalOrTrue)
{
    const auto formula{formula::parse("1||0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, logicalOrFalse)
{
    const auto formula{formula::parse("0||0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, logicalOrPrecedence)
{
    const auto formula{formula::parse("1+2||3+4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // (1+2) || (3+4) is true
}

TEST(TestFormulaInterpret, logicalOrShortCircuitTrue)
{
    const auto formula{formula::parse("1||z=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(0.0, formula->get_value("z").re); // z should not be set
}

TEST(TestFormulaInterpret, statements)
{
    const auto formula{formula::parse("3\n"
                                      "4\n")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, assignmentStatements)
{
    const auto formula{formula::parse("q=3\n"
                                      "z=4\n")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(3.0, formula->get_value("q").re);
    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, formulaInitialize)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {100.0, 0.0});

    ASSERT_EQ(4.4, formula->interpret(formula::INITIALIZE).re);

    ASSERT_EQ(4.4, formula->get_value("pixel").re);
    ASSERT_EQ(4.4, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, formulaIterate)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {2.0, 0.0});

    ASSERT_EQ(8.4, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.4, formula->get_value("pixel").re);
    ASSERT_EQ(8.4, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, formulaBailoutFalse)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {2.0, 0.0});

    ASSERT_EQ(0.0, formula->interpret(formula::BAILOUT).re);

    ASSERT_EQ(2.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, formulaBailoutTrue)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {8.0, 0.0});

    ASSERT_EQ(1.0, formula->interpret(formula::BAILOUT).re);

    ASSERT_EQ(8.0, formula->get_value("z").re);
}

namespace
{

class InterpretFunctionCall : public TestWithParam<FunctionCallParam>
{
};

} // namespace

TEST_P(InterpretFunctionCall, interpret)
{
    const FunctionCallParam &param{GetParam()};
    const auto formula{formula::parse(param.expr)};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_NEAR(param.result.re, result.re, 1e-8);
    ASSERT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, InterpretFunctionCall, ValuesIn(g_calls));

TEST(TestFormulaInterpret, ifStatementEmptyBodyTrue)
{
    const auto formula{formula::parse("if(5)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, ifStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(5<4)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, ifStatementBodyTrue)
{
    const auto formula{formula::parse("if(5)\n"
                                      "z=3\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(3.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, ifStatementBodyFalse)
{
    const auto formula{formula::parse("if(5<4)\n"
                                      "z=3\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {5.0, 0.0});

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(5.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, ifThenElseComplexBodyConditionFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "x=1\n"
                                      "y=2\n"
                                      "else\n"
                                      "z=3\n"
                                      "q=4\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("x", {0.0, 0.0});
    formula->set_value("y", {0.0, 0.0});
    formula->set_value("z", {0.0, 0.0});
    formula->set_value("q", {0.0, 0.0});

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(0.0, formula->get_value("x").re);
    ASSERT_EQ(0.0, formula->get_value("y").re);
    ASSERT_EQ(3.0, formula->get_value("z").re);
    ASSERT_EQ(4.0, formula->get_value("q").re);
}

TEST(TestFormulaInterpret, ifThenElseComplexBodyConditionTrue)
{
    const auto formula{formula::parse("if(1)\n"
                                      "x=1\n"
                                      "y=2\n"
                                      "else\n"
                                      "z=3\n"
                                      "q=4\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("x", {0.0, 0.0});
    formula->set_value("y", {0.0, 0.0});
    formula->set_value("z", {0.0, 0.0});
    formula->set_value("q", {0.0, 0.0});

    ASSERT_EQ(2.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(1.0, formula->get_value("x").re);
    ASSERT_EQ(2.0, formula->get_value("y").re);
    ASSERT_EQ(0.0, formula->get_value("z").re);
    ASSERT_EQ(0.0, formula->get_value("q").re);
}

TEST(TestFormulaInterpret, ifElseIfStatementEmptyBodyTrue)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(1)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, ifElseIfStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(0)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, ifElseIfElseStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(0)\n"
                                      "else\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpret, ifElseIfStatementBodyTrue)
{
    const auto formula{formula::parse("if(0)\n"
                                      "z=1\n"
                                      "elseif(1)\n"
                                      "z=4\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, ifElseIfStatementBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "z=1\n"
                                      "elseif(0)\n"
                                      "z=3\n"
                                      "else\n"
                                      "z=4\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, ifMultipleElseIfStatementBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "z=1\n"
                                      "elseif(0)\n"
                                      "z=3\n"
                                      "elseif(1)\n"
                                      "z=4\n"
                                      "else\n"
                                      "z=5\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpret, flip)
{
    const auto formula{formula::parse("flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(1.0, result.im); // flip(1) should return 0 + i
}

TEST(TestFormulaInterpret, complexAdd)
{
    const auto formula{formula::parse("1+flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(1.0, result.im);
}

TEST(TestFormulaInterpret, complexSubtract)
{
    const auto formula{formula::parse("1-flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(-1.0, result.im);
}

TEST(TestFormulaInterpret, complexMultiply)
{
    const auto formula{formula::parse("flip(1)*flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpret, complexDivideScalar)
{
    const auto formula{formula::parse("(1+flip(1))/2")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(0.5, result.re);
    EXPECT_EQ(0.5, result.im);
}

TEST(TestFormulaInterpret, complexDivide)
{
    const auto formula{formula::parse("(1+flip(1))/(2+flip(2))")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(0.5, result.re);
    EXPECT_EQ(0.0, result.im); // (1+i)/(2+2i) = 0.5 + 0.0i
}

TEST(TestFormulaInterpret, commaSequencesStatements)
{
    const auto formula{formula::parse("z=4,z=6+flip(3)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(3.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(6.0, z.re);
    EXPECT_EQ(3.0, z.im);
}
