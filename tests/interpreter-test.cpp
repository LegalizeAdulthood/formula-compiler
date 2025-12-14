// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "ExpressionParam.h"
#include "function-call.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <string_view>

using namespace testing;

namespace formula::test
{

using InterpreterParam = ExpressionParam;

class FormulaInterpretSuite : public TestWithParam<InterpreterParam>
{
};

TEST_P(FormulaInterpretSuite, interpret)
{
    const InterpreterParam &param{GetParam()};
    const FormulaPtr formula{create_formula(param.text)};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(param.section));

    const Complex result{formula->interpret(param.section)};

    EXPECT_NEAR(param.expected_re, result.re, 1e-8);
    EXPECT_NEAR(param.expected_im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestInterpreter, FormulaInterpretSuite, ValuesIn(g_expression_params));

TEST(TestFormulaInterpreter, setComplexValue)
{
    const FormulaPtr formula{create_formula("0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("a", {1.0, 2.0});

    const Complex a = formula->get_value("a");

    EXPECT_EQ(1.0, a.re);
    EXPECT_EQ(2.0, a.im);
}

TEST(TestFormulaInterpreter, setSymbolValue)
{
    const FormulaPtr formula{create_formula("a*a + b*b")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("a", {2.0, 0.0});
    formula->set_value("b", {3.0, 0.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_NEAR(13.0, result.re, 1e-5);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, getValue)
{
    const FormulaPtr formula{create_formula("1")};
    EXPECT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex e = formula->get_value("e");

    EXPECT_EQ(std::exp(1.0), e.re);
    EXPECT_EQ(0.0, e.im);
}

TEST(TestFormulaInterpreter, getValueUnknown)
{
    const FormulaPtr formula{create_formula("1")};
    EXPECT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex a = formula->get_value("a"); // unknown identifier

    EXPECT_EQ(0.0, a.re); // unknown identifier should return 0.0
    EXPECT_EQ(0.0, a.im);
}

TEST(TestFormulaInterpreter, assignment)
{
    const FormulaPtr formula{create_formula("z=4+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(6.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, assignmentParens)
{
    const FormulaPtr formula{create_formula("(z=4)+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ((formula::Complex{4.0, 0.0}), z);
}

TEST(TestFormulaInterpreter, chainedAssignment)
{
    const FormulaPtr formula{create_formula("z1=z2=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z1 = formula->get_value("z1");
    const Complex z2 = formula->get_value("z2");
    EXPECT_EQ((formula::Complex{3.0, 0.0}), z1);
    EXPECT_EQ((formula::Complex{3.0, 0.0}), z2);
}

TEST(TestFormulaInterpreter, conjugate)
{
    const FormulaPtr formula{create_formula("conj(z)")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {3.0, 4.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(-4.0, result.im);
}

TEST(TestFormulaInterpreter, identity)
{
    const FormulaPtr formula{create_formula("ident(z)")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {3.0, 4.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(4.0, result.im);
}

TEST(TestFormulaInterpreter, compareLessPrecedence)
{
    const FormulaPtr formula{create_formula("3<z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, logicalAndShortCircuitTrue)
{
    const FormulaPtr formula{create_formula("0&&z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re); // 0 is false, so the second part is not evaluated
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(0.0, z.re); // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, logicalOrShortCircuitTrue)
{
    const FormulaPtr formula{create_formula("1||z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(0.0, z.re); // z should not be set
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, assignmentStatementsIterate)
{
    const FormulaPtr formula{create_formula("q=3\n"
                                            "z=4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    formula->set_value("q", {-1.0, -1.0});
    formula->set_value("z", {-1.0, -1.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q = formula->get_value("q");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
    EXPECT_EQ(-1.0, z.re);
    EXPECT_EQ(-1.0, z.im);
}

TEST(TestFormulaInterpreter, assignmentStatementsBailout)
{
    const FormulaPtr formula{create_formula("q=3\n"
                                            "z=4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("q", {-1.0, -1.0});
    formula->set_value("z", {-1.0, -1.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q = formula->get_value("q");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(-1.0, q.re);
    EXPECT_EQ(-1.0, q.im);
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, commaSeparatedAssignmentStatementsIterate)
{
    const FormulaPtr formula{create_formula("q=3,z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    formula->set_value("q", {-1.0, -1.0});
    formula->set_value("z", {-1.0, -1.0});

    const Complex result{formula->interpret(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q = formula->get_value("q");
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(-1.0, z.re);
    EXPECT_EQ(-1.0, z.im);
}

TEST(TestFormulaInterpreter, commaSeparatedAssignmentStatementsBailout)
{
    const FormulaPtr formula{create_formula("q=3,z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("q", {-1.0, -1.0});
    formula->set_value("z", {-1.0, -1.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q = formula->get_value("q");
    const Complex z = formula->get_value("z");
    EXPECT_EQ(-1.0, q.re);
    EXPECT_EQ(-1.0, q.im);
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, formulaInitialize)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::INITIALIZE));
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
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
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
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
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
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
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4")};
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
    const FormulaPtr formula{create_formula(param.expr)};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_NEAR(param.result.re, result.re, 1e-8);
    EXPECT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, InterpreterFunctionCall, ValuesIn(g_calls));

TEST(TestFormulaInterpreter, ifStatementEmptyBodyTrue)
{
    const FormulaPtr formula{create_formula("if(5)\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifStatementEmptyBodyFalse)
{
    const FormulaPtr formula{create_formula("if(5<4)\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifStatementBodyTrue)
{
    const FormulaPtr formula{create_formula("if(5)\n"
                                            "z=3\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(5<4)\n"
                                            "z=3\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {5.0, 0.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(5.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifThenElseComplexBodyConditionFalse)
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

    const Complex result{formula->interpret(Section::BAILOUT)};

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

    const Complex result{formula->interpret(Section::BAILOUT)};

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
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "elseif(1)\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementEmptyBodyFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "elseif(0)\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifElseIfElseStatementEmptyBodyFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "elseif(0)\n"
                                            "else\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementBodyTrue)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(1)\n"
                                            "z=4\n"
                                            "endif")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);

    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifElseIfStatementBodyFalse)
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

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestFormulaInterpreter, ifMultipleElseIfStatementBodyFalse)
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

    const Complex result{formula->interpret(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z = formula->get_value("z");
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

} // namespace formula::test
