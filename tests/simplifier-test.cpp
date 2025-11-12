// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include "function-call.h"
#include "node-builders.h"
#include "NodeFormatter.h"
#include "trim_ws.h"

#include <formula/Node.h>

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

struct SimplifierParam
{
    std::string_view name;
    Expr expression;
    std::string_view expected;
};

struct BinaryOpTestParam
{
    Expr expression;
    std::string expected;
    std::string test_name;
};

struct FunctionSimplifyTestParam
{
    std::string function_name;
    double input_value;
    double expected_result;
    std::string test_name;
};

inline void PrintTo(const SimplifierParam &param, std::ostream *os)
{
    *os << param.name;
}

void PrintTo(const BinaryOpTestParam &param, std::ostream *os)
{
    *os << "BinaryOpTestParam{" << param.test_name << ": expression: " << trim_ws(to_string(param.expression))
        << ", expected: " << trim_ws(param.expected) << "}";
}

void PrintTo(const FunctionSimplifyTestParam &param, std::ostream *os)
{
    *os << "FunctionSimplifyTestParam{" << param.test_name << ": function: " << param.function_name
        << ", input: " << param.input_value << ", expected: " << param.expected_result << "}";
}

static SimplifierParam s_simplifier_tests[]{
    {"simplifySingleStatementSequence", statements({number(42.0)}), "literal:42\n"},
    {"multipleStatementSequencePreserved", statements({number(42.0), identifier("z")}),
        "statement_seq:2 {\n"
        "literal:42\n"
        "identifier:z\n"
        "}\n"},
    {"collapseMultipleNumberStatements", statements({identifier("z"), number(42.0), number(7.0), identifier("q")}),
        "statement_seq:3 {\n"
        "identifier:z\n"
        "literal:7\n"
        "identifier:q\n"
        "}\n"},
    {"unaryPlusNumber", unary('+', number(-42.0)), "literal:-42\n"},
    {"unaryMinusNumber", unary('-', number(-42.0)), "literal:42\n"},
    {"unaryModulusNumber", unary('|', number(-6.0)), "literal:36\n"},
    {"functionCallOnIdentifierNotSimplified", function_call("sin", identifier("x")),
        "function_call:sin(\n"
        "identifier:x\n"
        ")\n"},
    {"otherIdentifierNotSimplified", identifier("someVariable"), "identifier:someVariable\n"},
    {"ifStatementWithNonZeroConditionSimplified", if_statement(number(1.0), number(42.0), number(7.0)), "literal:42\n"},
    {"ifStatementWithZeroConditionSimplified", if_statement(number(0.0), number(42.0), number(7.0)), "literal:7\n"},
    {"ifStatementWithNonZeroConditionNoElse", if_statement(number(5.0), number(42.0)), "literal:42\n"},
    {"ifStatementWithZeroConditionNoElse", if_statement(number(0.0), number(42.0)), "literal:0\n"},
    {"ifStatementWithNonZeroConditionNoThen", if_statement(number(3.0), nullptr, number(7.0)), "literal:1\n"},
    {"ifStatementWithZeroConditionNoThen", if_statement(number(0.0), nullptr, number(7.0)), "literal:7\n"},
    {"ifStatementWithExpressionConditionSimplified",
        if_statement(binary(number(2.0), '+', number(3.0)), number(42.0), number(7.0)), "literal:42\n"},
    {"ifStatementWithVariableConditionNotSimplified", if_statement(identifier("x"), number(42.0), number(7.0)),
        "if_statement:(\n"
        "identifier:x\n"
        ") {\n"
        "literal:42\n"
        "} else {\n"
        "literal:7\n"
        "} endif\n"},
};

static BinaryOpTestParam s_binary_op_test_params[] = {
    {binary(number(7.0), '+', number(12.0)), "literal:19\n", "addTwoNumbers"},
    {binary(number(12.0), '-', number(7.0)), "literal:5\n", "subtractTwoNumbers"},
    {binary(number(12.0), '*', number(2.0)), "literal:24\n", "multiplyTwoNumbers"},
    {binary(number(12.0), '/', number(2.0)), "literal:6\n", "divideTwoNumbers"},
    {binary(number(2.0), "^", number(2.0)), "literal:4\n", "power"},
    {binary(number(12.0), '/', binary(number(1.0), '+', number(2.0))), "literal:4\n", "numberExpression"},
    {binary(number(0.0), "&&", identifier("x")), "literal:0\n", "shortCircuitAnd"},
    {binary(number(12.0), "||", identifier("x")), "literal:1\n", "shortCircuitOr"},
    {binary(number(3.0), "&&", number(4.0)), "literal:1\n", "logicalAnd"},
    {binary(number(0.0), "||", number(3.0)), "literal:1\n", "logicalOr"},
    {binary(number(0.0), "<", number(4.0)), "literal:1\n", "lessThan"},
    {binary(number(4.0), ">", number(0.0)), "literal:1\n", "greaterThan"},
    {binary(number(3.0), "==", number(3.0)), "literal:1\n", "equalTo"},
    {binary(number(0.0), "<=", number(4.0)), "literal:1\n", "lessThanOrEqual"},
    {binary(number(4.0), ">=", number(0.0)), "literal:1\n", "greaterThanOrEqual"},
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

class TestSimplifyBinaryOp : public TestFormulaSimplifier, public WithParamInterface<BinaryOpTestParam>
{
};

class SimplifierSuite : public TestFormulaSimplifier, public WithParamInterface<SimplifierParam>
{
};

TEST_P(SimplifierSuite, simplify)
{
    const SimplifierParam &param{GetParam()};
    const Expr simplified{simplify(param.expression)};

    EXPECT_EQ(param.expected, to_string(simplified));
}

INSTANTIATE_TEST_SUITE_P(TestSimplifier, SimplifierSuite, ValuesIn(s_simplifier_tests));

TEST_P(TestSimplifyBinaryOp, simplifyBinaryOperation)
{
    const BinaryOpTestParam &param = GetParam();

    const Expr simplified{simplify(param.expression)};

    EXPECT_EQ(param.expected, to_string(simplified));
}

INSTANTIATE_TEST_SUITE_P(BinaryOperations, TestSimplifyBinaryOp, ValuesIn(s_binary_op_test_params),
    [](const TestParamInfo<BinaryOpTestParam> &info) { return info.param.test_name; });

TEST_P(TestSimplifyFunctionCall, simplifyFunctionCallOnNumber)
{
    const FunctionSimplifyTestParam &param = GetParam();

    const Expr simplified = simplify(function_call(param.function_name, number(param.input_value)));

    std::ostringstream expected;
    expected << "literal:" << param.expected_result << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

INSTANTIATE_TEST_SUITE_P(FunctionCalls, TestSimplifyFunctionCall, ValuesIn(s_function_simplify_test_params),
    [](const TestParamInfo<FunctionSimplifyTestParam> &info) { return info.param.test_name; });

TEST_F(TestFormulaSimplifier, unknownFunctionNotSimplified)
{
    EXPECT_THROW(simplify(function_call("unknown_func", number(5.0))), std::exception);
}

TEST_F(TestFormulaSimplifier, piConstantSimplified)
{
    const Expr simplified = simplify(identifier("pi"));

    std::ostringstream expected;
    expected << "literal:" << std::atan2(0.0, -1.0) << "\n"; // ~3.14159
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, eConstantSimplified)
{
    const Expr simplified = simplify(identifier("e"));

    std::ostringstream expected;
    expected << "literal:" << std::exp(1.0) << "\n"; // ~2.71828
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, piInExpressionSimplified)
{
    const Expr simplified = simplify(binary(identifier("pi"), '*', number(2.0)));

    std::ostringstream expected;
    expected << "literal:" << (std::atan2(0.0, -1.0) * 2.0) << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, eInExpressionSimplified)
{
    const Expr simplified = simplify(binary(number(1.0), '+', identifier("e")));

    std::ostringstream expected;
    expected << "literal:" << (1.0 + std::exp(1.0)) << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

TEST_F(TestFormulaSimplifier, constantsInFunctionCallsSimplified)
{
    // Test sin(pi/2) which should equal 1.0
    const Expr simplified = simplify(function_call("sin", binary(identifier("pi"), '/', number(2.0))));

    const double expectedValue = std::sin(std::atan2(0.0, -1.0) / 2.0);
    std::ostringstream expected;
    expected << "literal:" << expectedValue << "\n";
    EXPECT_EQ(expected.str(), to_string(simplified));
}

} // namespace formula::test
