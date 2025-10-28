// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "function-call.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <iostream>
#include <iterator>

using namespace testing;

TEST(TestFormulaInterpreter, one)
{
    ASSERT_EQ(1.0, formula::parse("1")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, two)
{
    ASSERT_EQ(2.0, formula::parse("2")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, add)
{
    ASSERT_EQ(2.0, formula::parse("1+1")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, unaryMinusNegativeOne)
{
    ASSERT_EQ(1.0, formula::parse("--1")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, multiply)
{
    ASSERT_EQ(6.0, formula::parse("2*3")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, divide)
{
    ASSERT_EQ(3.0, formula::parse("6/2")->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, addMultiply)
{
    const auto formula{formula::parse("1+3*2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(7.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, multiplyAdd)
{
    const auto formula{formula::parse("3*2+1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(7.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, addAddAdd)
{
    const auto formula{formula::parse("1+1+1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, mulMulMul)
{
    const auto formula{formula::parse("2*2*2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(8.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, twoPi)
{
    const auto formula{formula::parse("2*pi")};
    ASSERT_TRUE(formula);

    ASSERT_NEAR(6.28318, formula->interpret(formula::ITERATE).re, 1e-5);
}

TEST(TestFormulaInterpreter, unknownIdentifierIsZero)
{
    const auto formula{formula::parse("a")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, setComplexValue)
{
    const auto formula{formula::parse("0")};
    ASSERT_TRUE(formula);
    formula->set_value("a", {1.0, 2.0});

    const formula::Complex result{formula->get_value("a")};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(2.0, result.im);
}

TEST(TestFormulaInterpreter, setSymbolValue)
{
    const auto formula{formula::parse("a*a + b*b")};
    ASSERT_TRUE(formula);
    formula->set_value("a", {2.0, 0.0});
    formula->set_value("b", {3.0, 0.0});

    ASSERT_NEAR(13.0, formula->interpret(formula::ITERATE).re, 1e-5);
}

TEST(TestFormulaInterpreter, power)
{
    const auto formula{formula::parse("2^3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(8.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, chainedPower)
{
    // TODO: is power operator left associative or right associative?
    const auto formula{formula::parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);

    ASSERT_EQ(64.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, powerPrecedence)
{
    const auto formula{formula::parse("2*3^2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(18.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, getValue)
{
    const auto formula{formula::parse("1")};
    EXPECT_TRUE(formula);

    EXPECT_EQ(std::exp(1.0), formula->get_value("e").re);
    EXPECT_EQ(0.0, formula->get_value("a").re); // unknown identifier should return 0.0
}

TEST(TestFormulaInterpreter, assignment)
{
    const auto formula{formula::parse("z=4+2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(6.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(6.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, assignmentParens)
{
    const auto formula{formula::parse("(z=4)+2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(6.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, chainedAssignment)
{
    const auto formula{formula::parse("z1=z2=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(3.0, formula->get_value("z1").re);
    ASSERT_EQ(3.0, formula->get_value("z2").re);
}

TEST(TestFormulaInterpreter, modulus)
{
    const auto formula{formula::parse("|-3.0 + flip(-2)|")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_EQ(13.0, result.re);
    ASSERT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, conjugate)
{
    const auto formula{formula::parse("conj(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(-4.0, result.im);
}

TEST(TestFormulaInterpreter, identity)
{
    const auto formula{formula::parse("ident(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(4.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessFalse)
{
    const auto formula{formula::parse("4<3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpreter, compareLessTrue)
{
    const auto formula{formula::parse("3<4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpreter, compareLessPrecedence)
{
    const auto formula{formula::parse("3<z=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, compareLessEqualTrueEquality)
{
    const auto formula{formula::parse("3<=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, compareLessEqualTrueLess)
{
    const auto formula{formula::parse("3<=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, compareLessEqualFalse)
{
    const auto formula{formula::parse("3<=2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, compareAssociatesLeft)
{
    const auto formula{formula::parse("4<3<4")}; // (4 < 3) < 4
    ASSERT_TRUE(formula);

    ASSERT_EQ(
        1.0, formula->interpret(formula::ITERATE).re); // (4 < 3) is false (0.0), (0 < 4) is true, so the result is 1.0
}

TEST(TestFormulaInterpreter, compareGreaterFalse)
{
    const auto formula{formula::parse("3>4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpreter, compareGreaterTrue)
{
    const auto formula{formula::parse("4>3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpreter, compareGreaterEqualTrueEquality)
{
    const auto formula{formula::parse("3>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, compareGreaterEqualTrueGreater)
{
    const auto formula{formula::parse("4>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, compareGreaterEqualFalse)
{
    const auto formula{formula::parse("2>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, compareEqualTrue)
{
    const auto formula{formula::parse("3==3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpreter, compareEqualFalse)
{
    const auto formula{formula::parse("3==4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpreter, compareNotEqualTrue)
{
    const auto formula{formula::parse("3!=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpreter, compareNotEqualFalse)
{
    const auto formula{formula::parse("3!=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpreter, logicalAndTrue)
{
    const auto formula{formula::parse("1&&1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // true is 1.0
}

TEST(TestFormulaInterpreter, logicalAndFalse)
{
    const auto formula{formula::parse("1&&0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // false is 0.0
}

TEST(TestFormulaInterpreter, logicalAndPrecedence)
{
    const auto formula{formula::parse("1+2&&3+4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // (1+2) && (3+4) is true
}

TEST(TestFormulaInterpreter, logicalAndShortCircuitTrue)
{
    const auto formula{formula::parse("0&&z=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re); // 0 is false, so the second part is not evaluated

    ASSERT_EQ(0.0, formula->get_value("z").re);              // z should not be set
}

TEST(TestFormulaInterpreter, logicalOrTrue)
{
    const auto formula{formula::parse("1||0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, logicalOrFalse)
{
    const auto formula{formula::parse("0||0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, logicalOrPrecedence)
{
    const auto formula{formula::parse("1+2||3+4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re); // (1+2) || (3+4) is true
}

TEST(TestFormulaInterpreter, logicalOrShortCircuitTrue)
{
    const auto formula{formula::parse("1||z=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(0.0, formula->get_value("z").re); // z should not be set
}

TEST(TestFormulaInterpreter, statements)
{
    const auto formula{formula::parse("3\n"
                                      "4\n")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, assignmentStatements)
{
    const auto formula{formula::parse("q=3\n"
                                      "z=4\n")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(3.0, formula->get_value("q").re);
    ASSERT_EQ(4.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, formulaInitialize)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {100.0, 0.0});

    ASSERT_EQ(4.4, formula->interpret(formula::INITIALIZE).re);

    ASSERT_EQ(4.4, formula->get_value("pixel").re);
    ASSERT_EQ(4.4, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, formulaIterate)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {2.0, 0.0});

    ASSERT_EQ(8.4, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(4.4, formula->get_value("pixel").re);
    ASSERT_EQ(8.4, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, formulaBailoutFalse)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {2.0, 0.0});

    ASSERT_EQ(0.0, formula->interpret(formula::BAILOUT).re);

    ASSERT_EQ(2.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, formulaBailoutTrue)
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

class InterpreterFunctionCall : public TestWithParam<FunctionCallParam>
{
};

} // namespace

TEST_P(InterpreterFunctionCall, interpret)
{
    const FunctionCallParam &param{GetParam()};
    const auto formula{formula::parse(param.expr)};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    ASSERT_NEAR(param.result.re, result.re, 1e-8);
    ASSERT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, InterpreterFunctionCall, ValuesIn(g_calls));

TEST(TestFormulaInterpreter, ifStatementEmptyBodyTrue)
{
    const auto formula{formula::parse("if(5)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, ifStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(5<4)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, ifStatementBodyTrue)
{
    const auto formula{formula::parse("if(5)\n"
                                      "z=3\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(3.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, ifStatementBodyFalse)
{
    const auto formula{formula::parse("if(5<4)\n"
                                      "z=3\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {5.0, 0.0});

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);

    ASSERT_EQ(5.0, formula->get_value("z").re);
}

TEST(TestFormulaInterpreter, ifThenElseComplexBodyConditionFalse)
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

TEST(TestFormulaInterpreter, ifThenElseComplexBodyConditionTrue)
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

TEST(TestFormulaInterpreter, ifElseIfStatementEmptyBodyTrue)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(1)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, ifElseIfStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(0)\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, ifElseIfElseStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(0)\n"
                                      "else\n"
                                      "endif")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE).re);
}

TEST(TestFormulaInterpreter, ifElseIfStatementBodyTrue)
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

TEST(TestFormulaInterpreter, ifElseIfStatementBodyFalse)
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

TEST(TestFormulaInterpreter, ifMultipleElseIfStatementBodyFalse)
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

TEST(TestFormulaInterpreter, flip)
{
    const auto formula{formula::parse("flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(1.0, result.im); // flip(1) should return 0 + i
}

TEST(TestFormulaInterpreter, complexAdd)
{
    const auto formula{formula::parse("1+flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(1.0, result.im);
}

TEST(TestFormulaInterpreter, complexSubtract)
{
    const auto formula{formula::parse("1-flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(-1.0, result.im);
}

TEST(TestFormulaInterpreter, complexMultiply)
{
    const auto formula{formula::parse("flip(1)*flip(1)")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, complexDivideScalar)
{
    const auto formula{formula::parse("(1+flip(1))/2")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(0.5, result.re);
    EXPECT_EQ(0.5, result.im);
}

TEST(TestFormulaInterpreter, complexDivide)
{
    const auto formula{formula::parse("(1+flip(1))/(2+flip(2))")};
    ASSERT_TRUE(formula);

    const formula::Complex result{formula->interpret(formula::ITERATE)};

    EXPECT_EQ(0.5, result.re);
    EXPECT_EQ(0.0, result.im); // (1+i)/(2+2i) = 0.5 + 0.0i
}
