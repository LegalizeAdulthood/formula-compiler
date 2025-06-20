#include <formula/formula.h>

#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <iterator>

using namespace testing;

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

TEST(TestFormulaInterpret, formulaBailoutTrue)
{
    const auto formula{formula::parse("z=pixel:z=z*z+pixel,|z|>4")};
    ASSERT_TRUE(formula);
    formula->set_value("pixel", 4.4);
    formula->set_value("z", 8.0);

    ASSERT_EQ(1.0, formula->interpret(formula::BAILOUT));
    ASSERT_EQ(8.0, formula->get_value("z"));
}

namespace
{

struct FunctionCallParam
{
    std::string_view expr;
    double result;
};

std::ostream &operator<<(std::ostream &str, const FunctionCallParam &value)
{
    return str << value.expr << '=' << value.result;
}

class FunctionCall : public TestWithParam<FunctionCallParam>
{
};

} // namespace

static FunctionCallParam s_calls[]{
    FunctionCallParam{"abs(-1.0)", 1.0},
    FunctionCallParam{"acos(-1.0)", 3.1415926535897931},
    FunctionCallParam{"acosh(10.0)", 2.9932228461263808},
    FunctionCallParam{"asin(1.0)", 1.5707963267948966},
    FunctionCallParam{"asinh(1.0)", 0.88137358701954305},
    FunctionCallParam{"atan(1.0)", 0.78539816339744828},
    FunctionCallParam{"atanh(0.9)", 1.4722194895832204},
    FunctionCallParam{"cabs(-1.0)", 1.0},
    FunctionCallParam{"ceil(1.1)", 2.0},
    FunctionCallParam{"conj(1.0)", -1.0},
    FunctionCallParam{"cos(pi)", -1.0},
    FunctionCallParam{"cosh(1)", 1.5430806348152437},
    FunctionCallParam{"cosxx(1)", 0.83373002513114913},
    FunctionCallParam{"cotan(1.0)", 0.64209261593433076},
    FunctionCallParam{"cotanh(1.0)", 1.3130352854993312},
    FunctionCallParam{"exp(1.0)", 2.7182818284590451},
    FunctionCallParam{"flip(1.0)", -1.0},
    FunctionCallParam{"floor(2.1)", 2.0},
    FunctionCallParam{"fn1(1.0)", 1.0},
    FunctionCallParam{"fn2(1.0)", 1.0},
    FunctionCallParam{"fn3(1.0)", 1.0},
    FunctionCallParam{"fn4(1.0)", 1.0},
    FunctionCallParam{"ident(1.0)", 1.0},
    FunctionCallParam{"imag(1.0)", -1.0},
    FunctionCallParam{"log(e)", 1.0},
    FunctionCallParam{"one(0.0)", 1.0},
    FunctionCallParam{"real(1.0)", 1.0},
    FunctionCallParam{"round(1.6)", 2.0},
    FunctionCallParam{"sin(pi/2)", 1.0},
    FunctionCallParam{"sinh(1)", 1.1752011936438014},
    FunctionCallParam{"sqr(2.0)", 4.0},
    FunctionCallParam{"sqrt(4.0)", 2.0},
    FunctionCallParam{"srand(1.0)", 0.0},
    FunctionCallParam{"tan(pi/4)", 1.0},
    FunctionCallParam{"tanh(1.0)", 0.76159415595576485},
    FunctionCallParam{"trunc(1.6)", 1.0},
    FunctionCallParam{"zero(1.0)", 0.0},
};

TEST_P(FunctionCall, call)
{
    const FunctionCallParam &param{GetParam()};
    const auto formula{formula::parse(param.expr)};
    ASSERT_TRUE(formula);

    ASSERT_NEAR(param.result, formula->interpret(formula::ITERATE), 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaInterpret, FunctionCall, ValuesIn(s_calls));
