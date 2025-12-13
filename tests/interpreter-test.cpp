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
#include <string_view>

using namespace testing;

namespace formula::test
{

struct InterpreterParam
{
    std::string_view name;
    std::string_view text;
    Section section;
    double expected_re;
    double expected_im;
};

inline void PrintTo(const InterpreterParam &param, std::ostream *os)
{
    *os << param.name;
}

// TODO: is power operator left associative or right associative?
// TODO: is an empty section allowed?
static InterpreterParam s_formulas[]{
    {"one", "1", Section::BAILOUT, 1.0, 0.0},                                           //
    {"two", "2", Section::BAILOUT, 2.0, 0.0},                                           //
    {"add", "1+1", Section::BAILOUT, 2.0, 0.0},                                         //
    {"unaryMinusNegativeOne", "--1", Section::BAILOUT, 1.0, 0.0},                       //
    {"multiply", "2*3", Section::BAILOUT, 6.0, 0.0},                                    //
    {"divide", "6/2", Section::BAILOUT, 3.0, 0.0},                                      //
    {"addMultiply", "1+3*2", Section::BAILOUT, 7.0, 0.0},                               //
    {"multiplyAdd", "3*2+1", Section::BAILOUT, 7.0, 0.0},                               //
    {"addAddAdd", "1+1+1", Section::BAILOUT, 3.0, 0.0},                                 //
    {"mulMulMul", "2*2*2", Section::BAILOUT, 8.0, 0.0},                                 //
    {"twoPi", "2*pi", Section::BAILOUT, 6.2831853071795862, 0.0},                       //
    {"unknownIdentifierIsZero", "a", Section::BAILOUT, 0.0, 0.0},                       //
    {"power", "2^3", Section::BAILOUT, 8.0, 0.0},                                       //
    {"powerLeftAssociative", "2^3^2", Section::BAILOUT, 64.0, 0.0},                     //
    {"powerPrecedence", "2*3^2", Section::BAILOUT, 18.0, 0.0},                          //
    {"modulus", "|-3.0 + flip(-2)|", Section::BAILOUT, 13.0, 0.0},                      //
    {"compareLessFalse", "4<3", Section::BAILOUT, 0.0, 0.0},                            //
    {"compareLessTrue", "3<4", Section::BAILOUT, 1.0, 0.0},                             //
    {"compareLessEqualTrueEquality", "3<=3", Section::BAILOUT, 1.0, 0.0},               //
    {"compareLessEqualTrueLess", "3<=4", Section::BAILOUT, 1.0, 0.0},                   //
    {"compareLessEqualFalse", "3<=2", Section::BAILOUT, 0.0, 0.0},                      //
    {"compareAssociatesLeft", "4<3<4", Section::BAILOUT, 1.0, 0.0},                     //
    {"compareGreaterFalse", "3>4", Section::BAILOUT, 0.0, 0.0},                         //
    {"compareGreaterTrue", "4>3", Section::BAILOUT, 1.0, 0.0},                          //
    {"compareGreaterEqualTrueEquality", "3>=3", Section::BAILOUT, 1.0, 0.0},            //
    {"compareGreaterEqualTrueGreater", "4>=3", Section::BAILOUT, 1.0, 0.0},             //
    {"compareGreaterEqualFalse", "2>=3", Section::BAILOUT, 0.0, 0.0},                   //
    {"compareEqualTrue", "3==3", Section::BAILOUT, 1.0, 0.0},                           //
    {"compareEqualFalse", "3==4", Section::BAILOUT, 0.0, 0.0},                          //
    {"compareNotEqualTrue", "3!=4", Section::BAILOUT, 1.0, 0.0},                        //
    {"compareNotEqualFalse", "3!=3", Section::BAILOUT, 0.0, 0.0},                       //
    {"logicalAndTrue", "1&&1", Section::BAILOUT, 1.0, 0.0},                             //
    {"logicalAndFalse", "1&&0", Section::BAILOUT, 0.0, 0.0},                            //
    {"logicalAndPrecedence", "1+2&&3+4", Section::BAILOUT, 1.0, 0.0},                   //
    {"logicalOrTrue", "1||0", Section::BAILOUT, 1.0, 0.0},                              //
    {"logicalOrFalse", "0||0", Section::BAILOUT, 0.0, 0.0},                             //
    {"logicalOrPrecedence", "1+2||3+4", Section::BAILOUT, 1.0, 0.0},                    //
    {"statementsIterate", "3\n4\n", Section::ITERATE, 3.0, 0.0},                        //
    {"statementsBailout", "3\n4\n", Section::BAILOUT, 4.0, 0.0},                        //
    {"commaSeparatedStatementsIterate", "3,4", Section::ITERATE, 3.0, 0.0},             //
    {"commaSeparatedStatementsBailout", "3,4", Section::BAILOUT, 4.0, 0.0},             //
    {"flip", "flip(1)", Section::BAILOUT, 0.0, 1.0},                                    //
    {"complexAdd", "1+flip(1)", Section::BAILOUT, 1.0, 1.0},                            //
    {"complexSubtract", "1-flip(1)", Section::BAILOUT, 1.0, -1.0},                      //
    {"complexMultiply", "flip(1)*flip(1)", Section::BAILOUT, -1.0, 0.0},                //
    {"complexDivideScalar", "(1+flip(1))/2", Section::BAILOUT, 0.5, 0.5},               //
    {"complexDivide", "(1+flip(1))/(2+flip(2))", Section::BAILOUT, 0.5, 0.0},           //
    {"globalSection", "global:\n1\n", Section::PER_IMAGE, 1.0, 0.0},                    //
    {"initSection", "init:\n2\n", Section::INITIALIZE, 2.0, 0.0},                       //
    {"loopSection", "loop:\n3\n", Section::ITERATE, 3.0, 0.0},                          //
    {"bailoutSection", "bailout:\n4\n", Section::BAILOUT, 4.0, 0.0},                    //
    {"perturbInitSection", "perturbinit:\n5\n", Section::PERTURB_INITIALIZE, 5.0, 0.0}, //
    {"perturbLoopSection", "perturbloop:\n6\n", Section::PERTURB_ITERATE, 6.0, 0.0},    //
    {"powerISquared", "flip(1)^2", Section::BAILOUT, -1.0, 0.0},                        //
    {"powerZeroZero", "0^0", Section::BAILOUT, 1.0, 0.0},                               //
    {"powerZeroOne", "0^1", Section::BAILOUT, 0.0, 0.0},                                //
    {"powerZeroTwo", "0^2", Section::BAILOUT, 0.0, 0.0},                                //
    {"powerOneOneSquared", "(1+flip(1))^2", Section::BAILOUT, 0.0, 2.0},                //
};

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

INSTANTIATE_TEST_SUITE_P(TestInterpreter, FormulaInterpretSuite, ValuesIn(s_formulas));

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
