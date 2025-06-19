#include <formula/formula.h>

#include <gtest/gtest.h>

#include <cmath>

TEST(TestCompiledFormulaRun, one)
{
    const auto formula{formula::parse("1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, two)
{
    const auto formula{formula::parse("2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(2.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, identifier)
{
    const auto formula{formula::parse("e")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(std::exp(1.0), formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, unknownIdentifierIsZero)
{
    const auto formula{formula::parse("a")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, add)
{
    const auto formula{formula::parse("1.2+1.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(2.4, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, subtract)
{
    const auto formula{formula::parse("1.5-2.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(-0.7, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, multiply)
{
    const auto formula{formula::parse("2.2*3.1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(6.82, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, divide)
{
    const auto formula{formula::parse("13.76/4.3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(3.2, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, avogadrosNumberDivide)
{
    const auto formula{formula::parse("6.02e23/2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(3.01e23, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, unaryNegate)
{
    const auto formula{formula::parse("--1.6")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(1.6, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, addAddadd)
{
    const auto formula{formula::parse("1.1+2.2+3.3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(6.6, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, mulMulMul)
{
    const auto formula{formula::parse("2.2*2.2*2.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(10.648, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, addMulAdd)
{
    const auto formula{formula::parse("1.1+2.2*3.3+4.4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(12.76, formula->run(formula::ITERATE), 1e-6);
}

TEST(TestCompiledFormulaRun, power)
{
    const auto formula{formula::parse("2^3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(8.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, chainedPower)
{
    const auto formula{formula::parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(64.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, powerPrecedence)
{
    const auto formula{formula::parse("2*3^2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(18.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, assignment)
{
    const auto formula{formula::parse("z=4+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(6.0, formula->run(formula::ITERATE));
    ASSERT_EQ(6.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, assignmentParens)
{
    const auto formula{formula::parse("(z=4)+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(6.0, formula->run(formula::ITERATE));
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, chainedAssignment)
{
    const auto formula{formula::parse("z1=z2=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(3.0, formula->run(formula::ITERATE));
    ASSERT_EQ(3.0, formula->get_value("z1"));
    ASSERT_EQ(3.0, formula->get_value("z2"));
}

TEST(TestCompiledFormulaRun, modulus)
{
    const auto formula{formula::parse("|-3.0|")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(3.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareLessFalse)
{
    const auto formula{formula::parse("4<3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE)); // false is 0.0
}

TEST(TestCompiledFormulaRun, compareLessTrue)
{
    const auto formula{formula::parse("3<4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE)); // true is 1.0
}

TEST(TestCompiledFormulaRun, compareLessPrecedence)
{
    const auto formula{formula::parse("3<z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, compareLessEqualTrueEquality)
{
    const auto formula{formula::parse("3<=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareLessEqualTrueLess)
{
    const auto formula{formula::parse("3<=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareLessEqualFalse)
{
    const auto formula{formula::parse("3<=2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareGreaterFalse)
{
    const auto formula{formula::parse("3>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareGreaterTrue)
{
    const auto formula{formula::parse("4>3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareGreaterEqualTrueEquality)
{
    const auto formula{formula::parse("3>=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareGreaterEqualTrueGreater)
{
    const auto formula{formula::parse("4>=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareEqualTrue)
{
    const auto formula{formula::parse("3==3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareEqualFalse)
{
    const auto formula{formula::parse("3==4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareNotEqualTrue)
{
    const auto formula{formula::parse("3!=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, compareNotEqualFalse)
{
    const auto formula{formula::parse("3!=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, logicalAndTrue)
{
    const auto formula{formula::parse("1&&1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, logicalAndFalse)
{
    const auto formula{formula::parse("1&&0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, logicalAndShortCircuitTrue)
{
    const auto formula{formula::parse("0&&z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE)); // 0 is false, so the second part is not evaluated
    ASSERT_EQ(0.0, formula->get_value("z"));        // z should not be set
}

TEST(TestCompiledFormulaRun, logicalOrTrue)
{
    const auto formula{formula::parse("1||0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, logicalOrFalse)
{
    const auto formula{formula::parse("0||0")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, logicalOrShortCircuit)
{
    const auto formula{formula::parse("1||z=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::ITERATE)); // 1 is true, so the second part is not evaluated
    ASSERT_EQ(0.0, formula->get_value("z"));        // z should not be set
}

TEST(TestCompiledFormulaRun, statements)
{
    const auto formula{formula::parse("3\n"
                                      "4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(4.0, formula->run(formula::ITERATE));
}

TEST(TestCompiledFormulaRun, assignmentStatements)
{
    const auto formula{formula::parse("z=3\n"
                                      "z=4\n")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(4.0, formula->run(formula::ITERATE));
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, formulaInitialize)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", 4.4);
    formula->set_value("z", 100.0);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(4.4, formula->run(formula::INITIALIZE));
    ASSERT_EQ(4.4, formula->get_value("pixel"));
    ASSERT_EQ(4.4, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, formulaIterate)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", 4.4);
    formula->set_value("z", 2.0);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(8.4, formula->run(formula::ITERATE));
    ASSERT_EQ(4.4, formula->get_value("pixel"));
    ASSERT_EQ(8.4, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, formulaBailoutFalse)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("z", 2.0);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run(formula::BAILOUT));
    ASSERT_EQ(2.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, formulaBailoutTrue)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", 4.4);
    formula->set_value("z", 8.0);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run(formula::BAILOUT));
    ASSERT_EQ(8.0, formula->get_value("z"));
}
