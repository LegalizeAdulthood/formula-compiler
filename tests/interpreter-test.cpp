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

using namespace formula;
using namespace testing;

TEST(TestFormulaInterpreter, one)
{
    const Complex result{parse("1")->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, two)
{
    const Complex result{parse("2")->interpret(Section::ITERATE)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, add)
{
    const Complex result{parse("1+1")->interpret(Section::ITERATE)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, unaryMinusNegativeOne)
{
    const Complex result{parse("--1")->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, multiply)
{
    const Complex result{parse("2*3")->interpret(Section::ITERATE)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, divide)
{
    const Complex result{parse("6/2")->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, addMultiply)
{
    const auto formula{parse("1+3*2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(7.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, multiplyAdd)
{
    const auto formula{parse("3*2+1")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(7.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, addAddAdd)
{
    const auto formula{parse("1+1+1")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, mulMulMul)
{
    const auto formula{parse("2*2*2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(8.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, twoPi)
{
    const auto formula{parse("2*pi")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_NEAR(6.28318, result.re, 1e-5);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, unknownIdentifierIsZero)
{
    const auto formula{parse("a")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, setComplexValue)
{
    const auto formula{parse("0")};
    ASSERT_TRUE(formula);
    formula->set_value("a", {1.0, 2.0});

    const Complex a = formula->get_value("a");

    EXPECT_EQ(1.0, a.re);
    EXPECT_EQ(2.0, a.im);
}

TEST(TestFormulaInterpreter, setSymbolValue)
{
    const auto formula{parse("a*a + b*b")};
    ASSERT_TRUE(formula);
    formula->set_value("a", {2.0, 0.0});
    formula->set_value("b", {3.0, 0.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_NEAR(13.0, result.re, 1e-5);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, power)
{
    const auto formula{parse("2^3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(8.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, chainedPower)
{
    // TODO: is power operator left associative or right associative?
    const auto formula{parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(64.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, powerPrecedence)
{
    const auto formula{parse("2*3^2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(18.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, getValue)
{
    const auto formula{parse("1")};
    EXPECT_TRUE(formula);

    const Complex e = formula->get_value("e");

    EXPECT_EQ(std::exp(1.0), e.re);
    EXPECT_EQ(0.0, e.im);
}

TEST(TestFormulaInterpreter, getValueUnknown)
{
    const auto formula{parse("1")};
    EXPECT_TRUE(formula);

    const Complex a = formula->get_value("a"); // unknown identifier

    EXPECT_EQ(0.0, a.re); // unknown identifier should return 0.0
    EXPECT_EQ(0.0, a.im);
}

TEST(TestFormulaInterpreter, assignment)
{
    const auto formula{parse("z=4+2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(6.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, assignmentParens)
{
    const auto formula{parse("(z=4)+2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ((formula::Complex{4.0, 0.0}), z);
}

TEST(TestFormulaInterpreter, chainedAssignment)
{
    const auto formula{parse("z1=z2=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z1 = formula->get_value("z1");
    const Complex z2 = formula->get_value("z2");
    EXPECT_EQ((formula::Complex{3.0, 0.0}), z1);
    EXPECT_EQ((formula::Complex{3.0, 0.0}), z2);
}

TEST(TestFormulaInterpreter, modulus)
{
    const auto formula{parse("|-3.0 + flip(-2)|")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(13.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, conjugate)
{
    const auto formula{parse("conj(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(-4.0, result.im);
}

TEST(TestFormulaInterpreter, identity)
{
    const auto formula{parse("ident(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(4.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessFalse)
{
    const auto formula{parse("4<3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re); // false is 0.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessTrue)
{
    const auto formula{parse("3<4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // true is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessPrecedence)
{
    const auto formula{parse("3<z=4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, compareLessEqualTrueEquality)
{
    const auto formula{parse("3<=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessEqualTrueLess)
{
    const auto formula{parse("3<=4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessEqualFalse)
{
    const auto formula{parse("3<=2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareAssociatesLeft)
{
    const auto formula{parse("4<3<4")}; // (4 < 3) < 4
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // (4 < 3) is false (0.0), (0 < 4) is true, so the result is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareGreaterFalse)
{
    const auto formula{parse("3>4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re); // false is 0.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareGreaterTrue)
{
    const auto formula{parse("4>3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // true is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareGreaterEqualTrueEquality)
{
    const auto formula{parse("3>=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareGreaterEqualTrueGreater)
{
    const auto formula{parse("4>=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareGreaterEqualFalse)
{
    const auto formula{parse("2>=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareEqualTrue)
{
    const auto formula{parse("3==3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // true is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareEqualFalse)
{
    const auto formula{parse("3==4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re); // false is 0.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareNotEqualTrue)
{
    const auto formula{parse("3!=4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // true is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, compareNotEqualFalse)
{
    const auto formula{parse("3!=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re); // false is 0.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalAndTrue)
{
    const auto formula{parse("1&&1")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // true is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalAndFalse)
{
    const auto formula{parse("1&&0")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re); // false is 0.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalAndPrecedence)
{
    const auto formula{parse("1+2&&3+4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // (1+2) && (3+4) is true
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalAndShortCircuitTrue)
{
    const auto formula{parse("0&&z=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re); // 0 is false, so the second part is not evaluated
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(0.0, z.re); // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, logicalOrTrue)
{
    const auto formula{parse("1||0")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalOrFalse)
{
    const auto formula{parse("0||0")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalOrPrecedence)
{
    const auto formula{parse("1+2||3+4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re); // (1+2) || (3+4) is true
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, logicalOrShortCircuitTrue)
{
    const auto formula{parse("1||z=3")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(0.0, z.re); // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, statements)
{
    const auto formula{parse("3\n"
                             "4\n")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, commaSeparatedStatements)
{
    const auto formula{parse("3,4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, assignmentStatements)
{
    const auto formula{parse("q=3\n"
                             "z=4\n")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q = formula->get_value("q");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, commaSeparatedAssignmentStatements)
{
    const auto formula{parse("q=3,z=4")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q = formula->get_value("q");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, formulaInitialize)
{
    const auto formula{parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {100.0, 0.0});

    const Complex result{formula->interpret(Section::INITIALIZE)};

    EXPECT_EQ(4.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex pixel = formula->get_value("pixel");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    EXPECT_EQ(4.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, formulaIterate)
{
    const auto formula{parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {2.0, 0.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(8.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex pixel = formula->get_value("pixel");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    EXPECT_EQ(8.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, formulaBailoutFalse)
{
    const auto formula{parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {2.0, 0.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(2.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, formulaBailoutTrue)
{
    const auto formula{parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {8.0, 0.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(8.0, z.re);
    EXPECT_EQ(0.0, z.im);
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
    const auto formula{parse(param.expr)};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_NEAR(param.result.re, result.re, 1e-8);
    EXPECT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, InterpreterFunctionCall, ValuesIn(g_calls));

TEST(TestFormulaInterpreter, ifStatementEmptyBodyTrue)
{
    const auto formula{parse("if(5)\n"
                             "endif")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifStatementEmptyBodyFalse)
{
    const auto formula{parse("if(5<4)\n"
                             "endif")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifStatementBodyTrue)
{
    const auto formula{parse("if(5)\n"
                             "z=3\n"
                             "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifStatementBodyFalse)
{
    const auto formula{parse("if(5<4)\n"
                             "z=3\n"
                             "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {5.0, 0.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(5.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifThenElseComplexBodyConditionFalse)
{
    const auto formula{parse("if(0)\n"
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

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex x = formula->get_value("x");
    const Complex y = formula->get_value("y");
    const Complex z = formula->get_value("z");
    const Complex q = formula->get_value("q");
    EXPECT_EQ(0.0, x.re);
    EXPECT_EQ(0.0, x.im);
    EXPECT_EQ(0.0, y.re);
    EXPECT_EQ(0.0, y.im);
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
    EXPECT_EQ(4.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestFormulaInterpreter, ifThenElseComplexBodyConditionTrue)
{
    const auto formula{parse("if(1)\n"
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

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex x = formula->get_value("x");
    const Complex y = formula->get_value("y");
    const Complex z = formula->get_value("z");
    const Complex q = formula->get_value("q");
    EXPECT_EQ(1.0, x.re);
    EXPECT_EQ(0.0, x.im);
    EXPECT_EQ(2.0, y.re);
    EXPECT_EQ(0.0, y.im);
    EXPECT_EQ(0.0, z.re);
    EXPECT_EQ(0.0, z.im);
    EXPECT_EQ(0.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementEmptyBodyTrue)
{
    const auto formula{parse("if(0)\n"
                             "elseif(1)\n"
                             "endif")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementEmptyBodyFalse)
{
    const auto formula{parse("if(0)\n"
                             "elseif(0)\n"
                             "endif")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifElseIfElseStatementEmptyBodyFalse)
{
    const auto formula{parse("if(0)\n"
                             "elseif(0)\n"
                             "else\n"
                             "endif")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementBodyTrue)
{
    const auto formula{parse("if(0)\n"
                             "z=1\n"
                             "elseif(1)\n"
                             "z=4\n"
                             "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);

    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementBodyFalse)
{
    const auto formula{parse("if(0)\n"
                             "z=1\n"
                             "elseif(0)\n"
                             "z=3\n"
                             "else\n"
                             "z=4\n"
                             "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifMultipleElseIfStatementBodyFalse)
{
    const auto formula{parse("if(0)\n"
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

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, flip)
{
    const auto formula{parse("flip(1)")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(1.0, result.im); // flip(1) should return 0 + i
}

TEST(TestFormulaInterpreter, complexAdd)
{
    const auto formula{parse("1+flip(1)")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(1.0, result.im);
}

TEST(TestFormulaInterpreter, complexSubtract)
{
    const auto formula{parse("1-flip(1)")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(-1.0, result.im);
}

TEST(TestFormulaInterpreter, complexMultiply)
{
    const auto formula{parse("flip(1)*flip(1)")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, complexDivideScalar)
{
    const auto formula{parse("(1+flip(1))/2")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.5, result.re);
    EXPECT_EQ(0.5, result.im);
}

TEST(TestFormulaInterpreter, complexDivide)
{
    const auto formula{parse("(1+flip(1))/(2+flip(2))")};
    ASSERT_TRUE(formula);

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(0.5, result.re);
    EXPECT_EQ(0.0, result.im); // (1+i)/(2+2i) = 0.5 + 0.0i
}
