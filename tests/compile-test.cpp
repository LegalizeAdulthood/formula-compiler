#include <formula/formula.h>

#include <gtest/gtest.h>

#include <cmath>

TEST(TestCompiledFormulaRun, one)
{
    const auto formula{formula::parse("1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run());
}

TEST(TestCompiledFormulaRun, two)
{
    const auto formula{formula::parse("2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(2.0, formula->run());
}

TEST(TestCompiledFormulaRun, identifier)
{
    const auto formula{formula::parse("e")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(std::exp(1.0), formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, unknownIdentifierIsZero)
{
    const auto formula{formula::parse("a")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run());
}

TEST(TestCompiledFormulaRun, add)
{
    const auto formula{formula::parse("1.2+1.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(2.4, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, subtract)
{
    const auto formula{formula::parse("1.5-2.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(-0.7, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, multiply)
{
    const auto formula{formula::parse("2.2*3.1")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(6.82, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, divide)
{
    const auto formula{formula::parse("13.76/4.3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(3.2, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, avogadrosNumberDivide)
{
    const auto formula{formula::parse("6.02e23/2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(3.01e23, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, unaryNegate)
{
    const auto formula{formula::parse("--1.6")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(1.6, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, addAddadd)
{
    const auto formula{formula::parse("1.1+2.2+3.3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(6.6, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, mulMulMul)
{
    const auto formula{formula::parse("2.2*2.2*2.2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(10.648, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, addMulAdd)
{
    const auto formula{formula::parse("1.1+2.2*3.3+4.4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_NEAR(12.76, formula->run(), 1e-6);
}

TEST(TestCompiledFormulaRun, power)
{
    const auto formula{formula::parse("2^3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(8.0, formula->run());
}

TEST(TestCompiledFormulaRun, chainedPower)
{
    const auto formula{formula::parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(64.0, formula->run());
}

TEST(TestCompiledFormulaRun, powerPrecedence)
{
    const auto formula{formula::parse("2*3^2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(18.0, formula->run());
}

TEST(TestCompiledFormulaRun, assignment)
{
    const auto formula{formula::parse("z=4+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(6.0, formula->run());
    ASSERT_EQ(6.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, assignmentParens)
{
    const auto formula{formula::parse("(z=4)+2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(6.0, formula->run());
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, chainedAssignment)
{
    const auto formula{formula::parse("z1=z2=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(3.0, formula->run());
    ASSERT_EQ(3.0, formula->get_value("z1"));
    ASSERT_EQ(3.0, formula->get_value("z2"));
}

TEST(TestCompiledFormulaRun, modulus)
{
    const auto formula{formula::parse("|-3.0|")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(3.0, formula->run());
}

TEST(TestCompiledFormulaRun, compareLessFalse)
{
    const auto formula{formula::parse("4<3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run()); // false is 0.0
}

TEST(TestCompiledFormulaRun, compareLessTrue)
{
    const auto formula{formula::parse("3<4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run()); // true is 1.0
}

TEST(TestCompiledFormulaRun, compareLessPrecedence)
{
    const auto formula{formula::parse("3<z=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run());
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, compareLessEqualTrueEquality)
{
    const auto formula{formula::parse("3<=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run());
}

TEST(TestCompiledFormulaRun, compareLessEqualTrueLess)
{
    const auto formula{formula::parse("3<=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run());
}

TEST(TestCompiledFormulaRun, compareLessEqualFalse)
{
    const auto formula{formula::parse("3<=2")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run());
}

TEST(TestCompiledFormulaRun, compareGreaterFalse)
{
    const auto formula{formula::parse("3>4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run()); // false is 0.0
}

TEST(TestCompiledFormulaRun, compareGreaterTrue)
{
    const auto formula{formula::parse("4>3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run()); // true is 1.0
}

TEST(TestCompiledFormulaRun, compareGreaterEqualTrueEquality)
{
    const auto formula{formula::parse("3>=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run());
}

TEST(TestCompiledFormulaRun, compareGreaterEqualTrueGreater)
{
    const auto formula{formula::parse("4>=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run());
}

TEST(TestCompiledFormulaRun, compareEqualTrue)
{
    const auto formula{formula::parse("3==3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run()); // true is 1.0
}

TEST(TestCompiledFormulaRun, compareEqualFalse)
{
    const auto formula{formula::parse("3==4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run()); // false is 0.0
}

TEST(TestCompiledFormulaRun, compareNotEqualTrue)
{
    const auto formula{formula::parse("3!=4")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(1.0, formula->run()); // true is 1.0
}

TEST(TestCompiledFormulaRun, compareNotEqualFalse)
{
    const auto formula{formula::parse("3!=3")};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->compile());

    ASSERT_EQ(0.0, formula->run()); // false is 0.0
}
