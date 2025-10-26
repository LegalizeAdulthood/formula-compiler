// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/formula.h>

#include "function-call.h"

#include <gtest/gtest.h>

#include <cmath>
#include <complex>

using namespace testing;

TEST(TestCompiledFormulaRun, one)
{
    const auto formula{formula::parse("1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, two)
{
    const auto formula{formula::parse("2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, identifier)
{
    const auto formula{formula::parse("e")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(std::exp(1.0), result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, identifierComplex)
{
    const auto formula{formula::parse("z")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile(false));
    formula->set_value("z", {2.0, 4.0});

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(2.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
}

TEST(TestCompiledFormulaRun, unknownIdentifierIsZero)
{
    const auto formula{formula::parse("a")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, add)
{
    const auto formula{formula::parse("1.2+1.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(2.4, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, addComplex)
{
    const auto formula{formula::parse("z+q")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {2.0, 4.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(6.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const formula::Complex q{formula->get_value("q")};
    EXPECT_EQ(2.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, subtract)
{
    const auto formula{formula::parse("1.5-2.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(-0.7, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, subtractComplex)
{
    const auto formula{formula::parse("z-q")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {2.0, 4.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(-2.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const formula::Complex q{formula->get_value("q")};
    EXPECT_EQ(2.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, multiply)
{
    const auto formula{formula::parse("2.2*3.1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(6.82, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

// (a + bi)(c + di) = (ac - bd) + (ad + bc)i
TEST(TestCompiledFormulaRun, multiplyComplex)
{
    const auto formula{formula::parse("z*q")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {3.0, 4.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    const std::complex<double> expected{std::complex<double>{1.0, 2.0} * std::complex<double>{3.0, 4.0}};
    EXPECT_EQ(expected.real(), result.re);
    EXPECT_EQ(expected.imag(), result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const formula::Complex q{formula->get_value("q")};
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, divide)
{
    const auto formula{formula::parse("13.76/4.3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(3.2, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, divideComplex)
{
    const auto formula{formula::parse("w/z")};
    ASSERT_TRUE(formula);
    formula->set_value("w", {1.0, 2.0});
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    const std::complex<double> expected{std::complex<double>{1.0, 2.0} / std::complex<double>{3.0, 4.0}};
    EXPECT_EQ(expected.real(), result.re);
    EXPECT_EQ(expected.imag(), result.im);
    const formula::Complex w{formula->get_value("w")};
    EXPECT_EQ(1.0, w.re);
    EXPECT_EQ(2.0, w.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(4.0, z.im);
}

TEST(TestCompiledFormulaRun, avogadrosNumberDivide)
{
    const auto formula{formula::parse("6.02e23/2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(3.01e23, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, unaryNegate)
{
    const auto formula{formula::parse("--1.6")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(1.6, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, unaryNegateComplex)
{
    const auto formula{formula::parse("-z")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(-2.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
}

TEST(TestCompiledFormulaRun, addAddadd)
{
    const auto formula{formula::parse("1.1+2.2+3.3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(6.6, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, mulMulMul)
{
    const auto formula{formula::parse("2.2*2.2*2.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(10.648, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, addMulAdd)
{
    const auto formula{formula::parse("1.1+2.2*3.3+4.4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(12.76, result.re, 1e-6);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, power)
{
    const auto formula{formula::parse("2^3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(8.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, chainedPower)
{
    const auto formula{formula::parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(64.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, powerPrecedence)
{
    const auto formula{formula::parse("2*3^2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(18.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, assignment)
{
    const auto formula{formula::parse("z=4+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(6.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, assignmentParens)
{
    const auto formula{formula::parse("(z=4)+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, chainedAssignment)
{
    const auto formula{formula::parse("z1=z2=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z1{formula->get_value("z1")};
    EXPECT_EQ(3.0, z1.re);
    EXPECT_EQ(0.0, z1.im);
    const formula::Complex z2{formula->get_value("z2")};
    EXPECT_EQ(3.0, z2.re);
    EXPECT_EQ(0.0, z2.im);
}

TEST(TestCompiledFormulaRun, modulus)
{
    const auto formula{formula::parse("|z|")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {-3.0, -2.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(13.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, conjugate)
{
    const auto formula{formula::parse("conj(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(-4.0, result.im);
}

TEST(TestCompiledFormulaRun, identity)
{
    const auto formula{formula::parse("ident(z)")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(4.0, result.im);
}

TEST(TestCompiledFormulaRun, compareLessFalse)
{
    const auto formula{formula::parse("4<3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re); // false is 0.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareLessTrue)
{
    const auto formula{formula::parse("3<4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re); // true is 1.0
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareLessPrecedence)
{
    const auto formula{formula::parse("3<z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, compareLessEqualTrueEquality)
{
    const auto formula{formula::parse("3<=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareLessEqualTrueLess)
{
    const auto formula{formula::parse("3<=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareLessEqualFalse)
{
    const auto formula{formula::parse("3<=2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareGreaterFalse)
{
    const auto formula{formula::parse("3>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareGreaterTrue)
{
    const auto formula{formula::parse("4>3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareGreaterEqualTrueEquality)
{
    const auto formula{formula::parse("3>=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareGreaterEqualTrueGreater)
{
    const auto formula{formula::parse("4>=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareEqualTrue)
{
    const auto formula{formula::parse("3==3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareEqualFalse)
{
    const auto formula{formula::parse("3==4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareNotEqualTrue)
{
    const auto formula{formula::parse("3!=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, compareNotEqualFalse)
{
    const auto formula{formula::parse("3!=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, logicalAndTrue)
{
    const auto formula{formula::parse("1&&1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, logicalAndFalse)
{
    const auto formula{formula::parse("1&&0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, logicalAndShortCircuitTrue)
{
    const auto formula{formula::parse("0&&z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);                         // 0 is false, so the second part is not evaluated
    EXPECT_EQ(0.0, result.im);                         //
    const formula::Complex z{formula->get_value("z")}; //
    EXPECT_EQ(0.0, z.re);                              // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, logicalOrTrue)
{
    const auto formula{formula::parse("1||0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, logicalOrFalse)
{
    const auto formula{formula::parse("0||0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, logicalOrShortCircuit)
{
    const auto formula{formula::parse("1||z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);                         // 1 is true, so the second part is not evaluated
    EXPECT_EQ(0.0, result.im);                         //
    const formula::Complex z{formula->get_value("z")}; //
    EXPECT_EQ(0.0, z.re);                              // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, statements)
{
    const auto formula{formula::parse("3\n"
                                      "4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, assignmentStatements)
{
    const auto formula{formula::parse("q=3\n"
                                      "z=4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex q{formula->get_value("q")};
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaInitialize)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {100.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::INITIALIZE)};

    EXPECT_EQ(4.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex pixel{formula->get_value("pixel")};
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaIterate)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {2.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(8.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex pixel{formula->get_value("pixel")};
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(8.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaBailoutFalse)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {2.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(2.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaBailoutTrue)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {8.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(8.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

namespace
{

class RunFunctionCall : public TestWithParam<FunctionCallParam>
{
};

} // namespace

TEST_P(RunFunctionCall, run)
{
    const FunctionCallParam &param{GetParam()};
    const auto formula{formula::parse(param.expr)};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_NEAR(param.result.re, result.re, 1e-8);
    EXPECT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, RunFunctionCall, ValuesIn(g_calls));

TEST(TestCompiledFormulaRun, ifStatementEmptyBodyTrue)
{
    const auto formula{formula::parse("if(5)\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, ifStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(5<4)\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, ifStatementBodyTrue)
{
    const auto formula{formula::parse("if(5)\n"
                                      "z=3\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifStatementBodyFalse)
{
    const auto formula{formula::parse("if(5<4)\n"
                                      "z=3\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {5.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(5.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifThenElseComplexBodyConditionFalse)
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
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex x{formula->get_value("x")};
    EXPECT_EQ(0.0, x.re);
    EXPECT_EQ(0.0, x.im);
    const formula::Complex y{formula->get_value("y")};
    EXPECT_EQ(0.0, y.re);
    EXPECT_EQ(0.0, y.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
    const formula::Complex q{formula->get_value("q")};
    EXPECT_EQ(4.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, ifThenElseComplexBodyConditionTrue)
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
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex x{formula->get_value("x")};
    EXPECT_EQ(1.0, x.re);
    EXPECT_EQ(0.0, x.im);
    const formula::Complex y{formula->get_value("y")};
    EXPECT_EQ(2.0, y.re);
    EXPECT_EQ(0.0, y.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(0.0, z.re);
    EXPECT_EQ(0.0, z.im);
    const formula::Complex q{formula->get_value("q")};
    EXPECT_EQ(0.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementEmptyBodyTrue)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(1)\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(0)\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, ifElseIfElseStatementEmptyBodyFalse)
{
    const auto formula{formula::parse("if(0)\n"
                                      "elseif(0)\n"
                                      "else\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementBodyTrue)
{
    const auto formula{formula::parse("if(0)\n"
                                      "z=1\n"
                                      "elseif(1)\n"
                                      "z=4\n"
                                      "endif")};
    ASSERT_TRUE(formula);
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementBodyFalse)
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
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifMultipleElseIfStatementBodyFalse)
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
    ASSERT_TRUE(formula->compile(false));

    const formula::Complex result{formula->run(formula::ITERATE)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const formula::Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}
