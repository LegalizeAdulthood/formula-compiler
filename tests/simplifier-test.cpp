// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include "function-call.h"
#include "node-builders.h"
#include "NodeFormatter.h"

#include <formula/Node.h>
#include <formula/Visitor.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace formula::ast;
using namespace testing;

namespace formula::test
{

class TestFormulaSimplifier : public Test
{
};

TEST_F(TestFormulaSimplifier, simplifySingleStatementSequence)
{
    const Expr simplified{simplify(statements({number(42.0)}))};

    EXPECT_EQ("number:42\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, multipleStatementSequencePreserved)
{
    const Expr simplified{simplify(statements({number(42.0), identifier("z")}))};

    EXPECT_EQ("statement_seq:2 {\n"
              "number:42\n"
              "identifier:z\n"
              "}\n",
        to_string(simplified));
}

TEST_F(TestFormulaSimplifier, collapseMultipleNumberStatements)
{
    const Expr simplified{simplify(statements({identifier("z"), number(42.0), number(7.0), identifier("q")}))};

    EXPECT_EQ("statement_seq:3 {\n"
              "identifier:z\n"
              "number:7\n"
              "identifier:q\n"
              "}\n",

        to_string(simplified));
}

TEST_F(TestFormulaSimplifier, unaryPlusNumber)
{
    const Expr simplified{simplify(unary('+', number(-42.0)))};

    EXPECT_EQ("number:-42\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, unaryMinusNumber)
{
    const Expr simplified{simplify(unary('-', number(-42.0)))};

    EXPECT_EQ("number:42\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, unaryModulusNumber)
{
    const Expr simplified{simplify(unary('|', number(-6.0)))};

    EXPECT_EQ("number:36\n", to_string(simplified));
}

struct BinaryOpTestParam
{
    Expr expression;
    std::string expected;
    std::string test_name;
};

static std::string trim_ws(std::string s)
{
    for (auto nl = s.find('\n'); nl != std::string::npos; nl = s.find('\n', nl))
    {
        s[nl] = ' ';
    }
    while (s.back() == '\n' || s.back() == ' ')
    {
        s.pop_back();
    }
    return s;
}

void PrintTo(const BinaryOpTestParam &param, std::ostream *os)
{
    *os << "BinaryOpTestParam{test_name: " << param.test_name
        << ", expression: " << trim_ws(to_string(param.expression)) << ", expected: " << trim_ws(param.expected) << "}";
}

class TestSimplifyBinaryOp : public TestFormulaSimplifier, public WithParamInterface<BinaryOpTestParam>
{
};

TEST_P(TestSimplifyBinaryOp, simplifyBinaryOperation)
{
    const auto &param = GetParam();

    const Expr simplified{simplify(param.expression)};

    EXPECT_EQ(param.expected, to_string(simplified));
}

static BinaryOpTestParam s_binary_op_test_params[] = {
    {binary(number(7.0), '+', number(12.0)), "number:19\n", "addTwoNumbers"},
    {binary(number(12.0), '-', number(7.0)), "number:5\n", "subtractTwoNumbers"},
    {binary(number(12.0), '*', number(2.0)), "number:24\n", "multiplyTwoNumbers"},
    {binary(number(12.0), '/', number(2.0)), "number:6\n", "divideTwoNumbers"},
    {binary(number(2.0), "^", number(2.0)), "number:4\n", "power"},
    {binary(number(12.0), '/', binary(number(1.0), '+', number(2.0))), "number:4\n", "numberExpression"},
    {binary(number(0.0), "&&", identifier("x")), "number:0\n", "shortCircuitAnd"},
    {binary(number(12.0), "||", identifier("x")), "number:1\n", "shortCircuitOr"},
    {binary(number(3.0), "&&", number(4.0)), "number:1\n", "logicalAnd"},
    {binary(number(0.0), "||", number(3.0)), "number:1\n", "logicalOr"},
    {binary(number(0.0), "<", number(4.0)), "number:1\n", "lessThan"},
    {binary(number(4.0), ">", number(0.0)), "number:1\n", "greaterThan"},
    {binary(number(3.0), "==", number(3.0)), "number:1\n", "equalTo"},
    {binary(number(0.0), "<=", number(4.0)), "number:1\n", "lessThanOrEqual"},
    {binary(number(4.0), ">=", number(0.0)), "number:1\n", "greaterThanOrEqual"},
};

INSTANTIATE_TEST_SUITE_P(BinaryOperations, TestSimplifyBinaryOp, ValuesIn(s_binary_op_test_params),
    [](const TestParamInfo<BinaryOpTestParam> &info) { return info.param.test_name; });

struct FunctionSimplifyTestParam
{
    std::string function_name;
    double input_value;
    double expected_result;
    std::string test_name;
};

// Simple function calls on numbers that should be simplified
static FunctionSimplifyTestParam s_function_simplify_test_params[] = {
    {"sin", 0.0, 0.0, "sinZero"},
    {"cos", 0.0, 1.0, "cosZero"},
    {"abs", -5.0, 5.0, "absNegative"},
    {"sqrt", 4.0, 2.0, "sqrtFour"},
    {"exp", 0.0, 1.0, "expZero"},
    {"log", 1.0, 0.0, "logOne"},
    {"sqr", 3.0, 9.0, "sqrThree"},
    {"floor", 2.7, 2.0, "floorPositive"},
    {"ceil", 2.3, 3.0, "ceilPositive"},
    {"round", 2.7, 3.0, "roundUp"},
    {"trunc", 2.9, 2.0, "truncPositive"},
    {"ident", 42.0, 42.0, "identValue"},
    {"one", 123.0, 1.0, "oneAnyInput"},
    {"zero", 456.0, 0.0, "zeroAnyInput"},
    {"real", 7.5, 7.5, "realValue"},
    {"cabs", -3.0, 3.0, "cabsNegative"},
    {"tan", 0.0, 0.0, "tanZero"},
    {"sinh", 0.0, 0.0, "sinhZero"},
    {"cosh", 0.0, 1.0, "coshZero"},
    {"tanh", 0.0, 0.0, "tanhZero"},
};

class TestSimplifyFunctionCall : public TestFormulaSimplifier, public WithParamInterface<FunctionSimplifyTestParam>
{
};

TEST_P(TestSimplifyFunctionCall, simplifyFunctionCallOnNumber)
{
    const auto &param = GetParam();

    const Expr simplified = simplify(function_call(param.function_name, number(param.input_value)));

    std::ostringstream expected;
    expected << "number:" << param.expected_result << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

INSTANTIATE_TEST_SUITE_P(FunctionCalls, TestSimplifyFunctionCall, ValuesIn(s_function_simplify_test_params),
    [](const TestParamInfo<FunctionSimplifyTestParam> &info) { return info.param.test_name; });

TEST_F(TestFormulaSimplifier, functionCallOnIdentifierNotSimplified)
{
    const Expr simplified = simplify(function_call("sin", identifier("x")));

    EXPECT_EQ("function_call:sin(\n"
              "identifier:x\n"
              ")\n",
        to_string(simplified));
}

TEST_F(TestFormulaSimplifier, unknownFunctionNotSimplified)
{
    EXPECT_THROW(simplify(function_call("unknown_func", number(5.0))), std::exception);
}

TEST_F(TestFormulaSimplifier, piConstantSimplified)
{
    const Expr simplified = simplify(identifier("pi"));

    std::ostringstream expected;
    expected << "number:" << std::atan2(0.0, -1.0) << "\n"; // ~3.14159
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, eConstantSimplified)
{
    const Expr simplified = simplify(identifier("e"));

    std::ostringstream expected;
    expected << "number:" << std::exp(1.0) << "\n"; // ~2.71828
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, otherIdentifierNotSimplified)
{
    const Expr simplified = simplify(identifier("someVariable"));

    EXPECT_EQ("identifier:someVariable\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, piInExpressionSimplified)
{
    const Expr simplified = simplify(binary(identifier("pi"), '*', number(2.0)));

    std::ostringstream expected;
    expected << "number:" << (std::atan2(0.0, -1.0) * 2.0) << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, eInExpressionSimplified)
{
    const Expr simplified = simplify(binary(number(1.0), '+', identifier("e")));

    std::ostringstream expected;
    expected << "number:" << (1.0 + std::exp(1.0)) << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, constantsInFunctionCallsSimplified)
{
    // Test sin(pi/2) which should equal 1.0
    const Expr simplified = simplify(function_call("sin", binary(identifier("pi"), '/', number(2.0))));

    const double expectedValue = std::sin(std::atan2(0.0, -1.0) / 2.0);
    std::ostringstream expected;
    expected << "number:" << expectedValue << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithNonZeroConditionSimplified)
{
    const Expr simplified = simplify(if_statement(number(1.0), number(42.0), number(7.0)));

    EXPECT_EQ("number:42\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithZeroConditionSimplified)
{
    const Expr simplified = simplify(if_statement(number(0.0), number(42.0), number(7.0)));

    EXPECT_EQ("number:7\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithNonZeroConditionNoElse)
{
    const Expr simplified = simplify(if_statement(number(5.0), number(42.0)));

    EXPECT_EQ("number:42\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithZeroConditionNoElse)
{
    const Expr simplified = simplify(if_statement(number(0.0), number(42.0)));

    EXPECT_EQ("number:0\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithNonZeroConditionNoThen)
{
    const Expr simplified = simplify(if_statement(number(3.0), nullptr, number(7.0)));

    EXPECT_EQ("number:1\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithZeroConditionNoThen)
{
    const Expr simplified = simplify(if_statement(number(0.0), nullptr, number(7.0)));

    EXPECT_EQ("number:7\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithExpressionConditionSimplified)
{
    // If condition evaluates to a number, it should be simplified
    const Expr simplified = simplify(if_statement(binary(number(2.0), '+', number(3.0)), number(42.0), number(7.0)));

    EXPECT_EQ("number:42\n", to_string(simplified));
}

TEST_F(TestFormulaSimplifier, ifStatementWithVariableConditionNotSimplified)
{
    // If condition contains variables, it should not be simplified
    const Expr simplified = simplify(if_statement(identifier("x"), number(42.0), number(7.0)));

    EXPECT_EQ("if_statement:(\n"
              "identifier:x\n"
              ") {\n"
              "number:42\n"
              "} else {\n"
              "number:7\n"
              "} endif\n",
        to_string(simplified));
}

} // namespace formula::test
