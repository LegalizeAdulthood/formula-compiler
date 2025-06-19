#include <formula/formula.h>

#include <gtest/gtest.h>

#include <cmath>

TEST(TestFormulaInterpret, one)
{
    ASSERT_EQ(1.0, formula::parse("1")->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, two)
{
    ASSERT_EQ(2.0, formula::parse("2")->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, add)
{
    ASSERT_EQ(2.0, formula::parse("1+1")->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, unaryMinusNegativeOne)
{
    ASSERT_EQ(1.0, formula::parse("--1")->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, multiply)
{
    ASSERT_EQ(6.0, formula::parse("2*3")->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, divide)
{
    ASSERT_EQ(3.0, formula::parse("6/2")->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, addMultiply)
{
    const auto formula{formula::parse("1+3*2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(7.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, multiplyAdd)
{
    const auto formula{formula::parse("3*2+1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(7.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, addAddAdd)
{
    const auto formula{formula::parse("1+1+1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, mulMulMul)
{
    const auto formula{formula::parse("2*2*2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(8.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, twoPi)
{
    const auto formula{formula::parse("2*pi")};
    ASSERT_TRUE(formula);

    ASSERT_NEAR(6.28318, formula->interpret(formula::ITERATE), 1e-5);
}

TEST(TestFormulaInterpret, unknownIdentifierIsZero)
{
    const auto formula{formula::parse("a")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, setSymbolValue)
{
    const auto formula{formula::parse("a*a + b*b")};
    ASSERT_TRUE(formula);
    formula->set_value("a", 2.0);
    formula->set_value("b", 3.0);

    ASSERT_NEAR(13.0, formula->interpret(formula::ITERATE), 1e-5);
}

TEST(TestFormulaInterpret, power)
{
    const auto formula{formula::parse("2^3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(8.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, chainedPower)
{
    // TODO: is power operator left associative or right associative?
    const auto formula{formula::parse("2^3^2")}; // same as (2^3)^2
    ASSERT_TRUE(formula);

    ASSERT_EQ(64.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, powerPrecedence)
{
    const auto formula{formula::parse("2*3^2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(18.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, getValue)
{
    const auto formula{formula::parse("1")};
    EXPECT_TRUE(formula);

    EXPECT_EQ(std::exp(1.0), formula->get_value("e"));
    EXPECT_EQ(0.0, formula->get_value("a")); // unknown identifier should return 0.0
}

TEST(TestFormulaInterpret, assignment)
{
    const auto formula{formula::parse("z=4+2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(6.0, formula->interpret(formula::ITERATE));
    ASSERT_EQ(6.0, formula->get_value("z"));
}

TEST(TestFormulaInterpret, assignmentParens)
{
    const auto formula{formula::parse("(z=4)+2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(6.0, formula->interpret(formula::ITERATE));
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestFormulaInterpret, chainedAssignment)
{
    const auto formula{formula::parse("z1=z2=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE));
    ASSERT_EQ(3.0, formula->get_value("z1"));
    ASSERT_EQ(3.0, formula->get_value("z2"));
}

TEST(TestFormulaInterpret, modulus)
{
    const auto formula{formula::parse("|-3.0|")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(3.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareLessFalse)
{
    const auto formula{formula::parse("4<3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE)); // false is 0.0
}

TEST(TestFormulaInterpret, compareLessTrue)
{
    const auto formula{formula::parse("3<4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // true is 1.0
}

TEST(TestFormulaInterpret, compareLessPrecedence)
{
    const auto formula{formula::parse("3<z=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestFormulaInterpret, compareLessEqualTrueEquality)
{
    const auto formula{formula::parse("3<=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareLessEqualTrueLess)
{
    const auto formula{formula::parse("3<=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareLessEqualFalse)
{
    const auto formula{formula::parse("3<=2")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareAssociatesLeft)
{
    const auto formula{formula::parse("4<3<4")}; // (4 < 3) < 4
    ASSERT_TRUE(formula);

    ASSERT_EQ(
        1.0, formula->interpret(formula::ITERATE)); // (4 < 3) is false (0.0), (0 < 4) is true, so the result is 1.0
}

TEST(TestFormulaInterpret, compareGreaterFalse)
{
    const auto formula{formula::parse("3>4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE)); // false is 0.0
}

TEST(TestFormulaInterpret, compareGreaterTrue)
{
    const auto formula{formula::parse("4>3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // true is 1.0
}

TEST(TestFormulaInterpret, compareGreaterEqualTrueEquality)
{
    const auto formula{formula::parse("3>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareGreaterEqualTrueGreater)
{
    const auto formula{formula::parse("4>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareGreaterEqualFalse)
{
    const auto formula{formula::parse("2>=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, compareEqualTrue)
{
    const auto formula{formula::parse("3==3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // true is 1.0
}

TEST(TestFormulaInterpret, compareEqualFalse)
{
    const auto formula{formula::parse("3==4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE)); // false is 0.0
}

TEST(TestFormulaInterpret, compareNotEqualTrue)
{
    const auto formula{formula::parse("3!=4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // true is 1.0
}

TEST(TestFormulaInterpret, compareNotEqualFalse)
{
    const auto formula{formula::parse("3!=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE)); // false is 0.0
}

TEST(TestFormulaInterpret, logicalAndTrue)
{
    const auto formula{formula::parse("1&&1")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // true is 1.0
}

TEST(TestFormulaInterpret, logicalAndFalse)
{
    const auto formula{formula::parse("1&&0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE)); // false is 0.0
}

TEST(TestFormulaInterpret, logicalAndPrecedence)
{
    const auto formula{formula::parse("1+2&&3+4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // (1+2) && (3+4) is true
}

TEST(TestFormulaInterpret, logicalAndShortCircuitTrue)
{
    const auto formula{formula::parse("0&&z=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE)); // 0 is false, so the second part is not evaluated
    ASSERT_EQ(0.0, formula->get_value("z"));              // z should not be set
}

TEST(TestFormulaInterpret, logicalOrTrue)
{
    const auto formula{formula::parse("1||0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, logicalOrFalse)
{
    const auto formula{formula::parse("0||0")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(0.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, logicalOrPrecedence)
{
    const auto formula{formula::parse("1+2||3+4")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE)); // (1+2) || (3+4) is true
}

TEST(TestFormulaInterpret, logicalOrShortCircuitTrue)
{
    const auto formula{formula::parse("1||z=3")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(1.0, formula->interpret(formula::ITERATE));
    ASSERT_EQ(0.0, formula->get_value("z")); // z should not be set
}

TEST(TestFormulaInterpret, statements)
{
    const auto formula{formula::parse("3\n"
                                      "4\n")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE));
}

TEST(TestFormulaInterpret, assignmentStatements)
{
    const auto formula{formula::parse("z=3\n"
                                      "z=4\n")};
    ASSERT_TRUE(formula);

    ASSERT_EQ(4.0, formula->interpret(formula::ITERATE));
    ASSERT_EQ(4.0, formula->get_value("z"));
}

TEST(TestFormulaInterpret, formulaInitialize)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", 4.4);
    formula->set_value("z", 100.0);

    ASSERT_EQ(4.4, formula->interpret(formula::INITIALIZE));
    ASSERT_EQ(4.4, formula->get_value("pixel"));
    ASSERT_EQ(4.4, formula->get_value("z"));
}

TEST(TestFormulaInterpret, formulaIterate)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", 4.4);
    formula->set_value("z", 2.0);

    ASSERT_EQ(8.4, formula->interpret(formula::ITERATE));
    ASSERT_EQ(4.4, formula->get_value("pixel"));
    ASSERT_EQ(8.4, formula->get_value("z"));
}

TEST(TestFormulaInterpret, formulaBailoutFalse)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("z", 2.0);

    ASSERT_EQ(0.0, formula->interpret(formula::BAILOUT));
    ASSERT_EQ(2.0, formula->get_value("z"));
}
