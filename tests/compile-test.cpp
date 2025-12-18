// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "ExpressionParam.h"
#include "function-call.h"

#include <gtest/gtest.h>

#include <cmath>
#include <complex>

using namespace testing;

namespace formula::test
{

using CompileParam = ExpressionParam;

class CompiledFormulaRunSuite : public TestWithParam<CompileParam>
{
};

TEST_P(CompiledFormulaRunSuite, run)
{
    const CompileParam &param{GetParam()};
    const FormulaPtr formula{create_formula(param.text)};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(param.section));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(param.section)};

    EXPECT_NEAR(param.expected_re, result.re, 1e-6);
    EXPECT_NEAR(param.expected_im, result.im, 1e-6);
}

INSTANTIATE_TEST_SUITE_P(TestCompiledFormula, CompiledFormulaRunSuite, ValuesIn(g_expression_params));

TEST(TestCompiledFormulaRun, identifierComplex)
{
    const FormulaPtr formula{create_formula("z")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile());
    formula->set_value("z", {2.0, 4.0});

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(2.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
}

TEST(TestCompiledFormulaRun, addComplex)
{
    const FormulaPtr formula{create_formula("z+q")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {2.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(6.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(2.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, subtractComplex)
{
    const FormulaPtr formula{create_formula("z-q")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {2.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(-2.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(2.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

// (a + bi)(c + di) = (ac - bd) + (ad + bc)i
TEST(TestCompiledFormulaRun, multiplyComplex)
{
    const FormulaPtr formula{create_formula("z*q")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    const std::complex<double> expected{std::complex<double>{1.0, 2.0} * std::complex<double>{3.0, 4.0}};
    EXPECT_EQ(expected.real(), result.re);
    EXPECT_EQ(expected.imag(), result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, divideComplex)
{
    const FormulaPtr formula{create_formula("w/z")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("w", {1.0, 2.0});
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    const std::complex<double> expected{std::complex<double>{1.0, 2.0} / std::complex<double>{3.0, 4.0}};
    EXPECT_EQ(expected.real(), result.re);
    EXPECT_EQ(expected.imag(), result.im);
    const Complex w{formula->get_value("w")};
    EXPECT_EQ(1.0, w.re);
    EXPECT_EQ(2.0, w.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(4.0, z.im);
}

TEST(TestCompiledFormulaRun, unaryNegateComplex)
{
    const FormulaPtr formula{create_formula("-z")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(-2.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
}

TEST(TestCompiledFormulaRun, assignment)
{
    const FormulaPtr formula{create_formula("z=4+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(6.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, assignmentParens)
{
    const FormulaPtr formula{create_formula("(z=4)+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, chainedAssignment)
{
    const FormulaPtr formula{create_formula("z1=z2=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z1{formula->get_value("z1")};
    EXPECT_EQ(3.0, z1.re);
    EXPECT_EQ(0.0, z1.im);
    const Complex z2{formula->get_value("z2")};
    EXPECT_EQ(3.0, z2.re);
    EXPECT_EQ(0.0, z2.im);
}

TEST(TestCompiledFormulaRun, modulus)
{
    const FormulaPtr formula{create_formula("|z|")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {-3.0, -2.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(13.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, conjugate)
{
    const FormulaPtr formula{create_formula("conj(z)")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(-4.0, result.im);
}

TEST(TestCompiledFormulaRun, identity)
{
    const FormulaPtr formula{create_formula("ident(z)")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(4.0, result.im);
}

TEST(TestCompiledFormulaRun, compareLessPrecedence)
{
    const FormulaPtr formula{create_formula("3<z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, logicalAndShortCircuitTrue)
{
    const FormulaPtr formula{create_formula("0&&z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);                // 0 is false, so the second part is not evaluated
    EXPECT_EQ(0.0, result.im);                //
    const Complex z{formula->get_value("z")}; //
    EXPECT_EQ(0.0, z.re);                     // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, logicalOrShortCircuit)
{
    const FormulaPtr formula{create_formula("1||z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);                // 1 is true, so the second part is not evaluated
    EXPECT_EQ(0.0, result.im);                //
    const Complex z{formula->get_value("z")}; //
    EXPECT_EQ(0.0, z.re);                     // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, assignmentStatementsIterate)
{
    const FormulaPtr formula{create_formula("q=3\n"
                                            "z=4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, assignmentStatementsBailout)
{
    const FormulaPtr formula{create_formula("q=3\n"
                                            "z=4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaInitialize)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::INITIALIZE));
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {100.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::INITIALIZE)};

    EXPECT_EQ(4.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex pixel{formula->get_value("pixel")};
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaIterate)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {2.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::ITERATE)};

    EXPECT_EQ(8.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex pixel{formula->get_value("pixel")};
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(8.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaBailoutFalse)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {2.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(2.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaBailoutTrue)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {8.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
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
    const FormulaPtr formula{create_formula(param.expr)};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_NEAR(param.result.re, result.re, 1e-8);
    EXPECT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, RunFunctionCall, ValuesIn(g_calls));

TEST(TestCompiledFormulaRun, ifStatementBodyTrue)
{
    const FormulaPtr formula{create_formula("if(5)\n"
                                            "z=3\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(5<4)\n"
                                            "z=3\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {5.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(5.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifThenElseComplexBodyConditionFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "x=1\n"
                                            "y=2\n"
                                            "else\n"
                                            "z=3\n"
                                            "q=4\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("x", {0.0, 0.0});
    formula->set_value("y", {0.0, 0.0});
    formula->set_value("z", {0.0, 0.0});
    formula->set_value("q", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex x{formula->get_value("x")};
    EXPECT_EQ(0.0, x.re);
    EXPECT_EQ(0.0, x.im);
    const Complex y{formula->get_value("y")};
    EXPECT_EQ(0.0, y.re);
    EXPECT_EQ(0.0, y.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(4.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, ifThenElseComplexBodyConditionTrue)
{
    const FormulaPtr formula{create_formula("if(1)\n"
                                            "x=1\n"
                                            "y=2\n"
                                            "else\n"
                                            "z=3\n"
                                            "q=4\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("x", {0.0, 0.0});
    formula->set_value("y", {0.0, 0.0});
    formula->set_value("z", {0.0, 0.0});
    formula->set_value("q", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex x{formula->get_value("x")};
    EXPECT_EQ(1.0, x.re);
    EXPECT_EQ(0.0, x.im);
    const Complex y{formula->get_value("y")};
    EXPECT_EQ(2.0, y.re);
    EXPECT_EQ(0.0, y.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(0.0, z.re);
    EXPECT_EQ(0.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(0.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementBodyTrue)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(1)\n"
                                            "z=4\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(0)\n"
                                            "z=3\n"
                                            "else\n"
                                            "z=4\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifMultipleElseIfStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(0)\n"
                                            "z=3\n"
                                            "elseif(1)\n"
                                            "z=4\n"
                                            "else\n"
                                            "z=5\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

} // namespace formula::test
