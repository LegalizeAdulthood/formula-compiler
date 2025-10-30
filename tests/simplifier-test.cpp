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
#include <iostream>
#include <sstream>

using namespace formula::ast;
using namespace testing;

namespace formula::test
{

class TestFormulaSimplifier : public Test
{
protected:
    std::ostringstream str;
    NodeFormatter formatter{str};
};

TEST_F(TestFormulaSimplifier, simplifySingleStatementSequence)
{
    const Expr simplified{simplify(statements({number(42.0)}))};

    simplified->visit(formatter);
    EXPECT_EQ("number:42\n", str.str());
}

TEST_F(TestFormulaSimplifier, multipleStatementSequencePreserved)
{
    const Expr simplified{simplify(statements({number(42.0), identifier("z")}))};

    simplified->visit(formatter);
    EXPECT_EQ("statement_seq:2 {\n"
              "number:42\n"
              "identifier:z\n"
              "}\n",
        str.str());
}

TEST_F(TestFormulaSimplifier, collapseMultipleNumberStatements)
{
    const Expr simplified{simplify(statements({identifier("z"), number(42.0), number(7.0), identifier("q")}))};

    simplified->visit(formatter);
    EXPECT_EQ("statement_seq:3 {\n"
              "identifier:z\n"
              "number:7\n"
              "identifier:q\n"
              "}\n",

        str.str());
}

TEST_F(TestFormulaSimplifier, unaryPlusNumber)
{
    const Expr simplified{simplify(unary('+', number(-42.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:-42\n", str.str());
}

TEST_F(TestFormulaSimplifier, unaryMinusNumber)
{
    const Expr simplified{simplify(unary('-', number(-42.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:42\n", str.str());
}

TEST_F(TestFormulaSimplifier, unaryModulusNumber)
{
    const Expr simplified{simplify(unary('|', number(-6.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:36\n", str.str());
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

// Google Test formatter for BinaryOpTestParam
void PrintTo(const BinaryOpTestParam &param, std::ostream *os)
{
    std::ostringstream expr_str;
    NodeFormatter formatter{expr_str};
    param.expression->visit(formatter);
    *os << "BinaryOpTestParam{test_name: " << param.test_name << ", expression: " << trim_ws(expr_str.str())
        << ", expected: " << trim_ws(param.expected) << "}";
}

class TestSimplifyBinaryOp : public TestFormulaSimplifier, public WithParamInterface<BinaryOpTestParam>
{
};

TEST_P(TestSimplifyBinaryOp, simplifyBinaryOperation)
{
    const auto &param = GetParam();

    const Expr simplified{simplify(param.expression)};

    simplified->visit(formatter);
    EXPECT_EQ(param.expected, str.str());
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
    // Arrange
    const auto &param = GetParam();
    const Expr expression = function_call(param.function_name, number(param.input_value));

    // Act
    const Expr simplified = simplify(expression);

    // Assert
    simplified->visit(formatter);
    std::ostringstream expected;
    expected << "number:" << param.expected_result << "\n";
    EXPECT_EQ(expected.str(), str.str());
}

INSTANTIATE_TEST_SUITE_P(FunctionCalls, TestSimplifyFunctionCall, ValuesIn(s_function_simplify_test_params),
    [](const TestParamInfo<FunctionSimplifyTestParam> &info) { return info.param.test_name; });

TEST_F(TestFormulaSimplifier, functionCallOnIdentifierNotSimplified)
{
    // Arrange
    const Expr expression = function_call("sin", identifier("x"));

    // Act
    const Expr simplified = simplify(expression);

    // Assert
    simplified->visit(formatter);
    EXPECT_EQ("function_call:sin(\n"
              "identifier:x\n"
              ")\n",
        str.str());
}

TEST_F(TestFormulaSimplifier, unknownFunctionNotSimplified)
{
    // Arrange
    const Expr expression = function_call("unknown_func", number(5.0));

    // Act
    const Expr simplified = simplify(expression);

    // Assert
    simplified->visit(formatter);
    EXPECT_EQ("function_call:unknown_func(\n"
              "number:5\n"
              ")\n",
        str.str());
}

} // namespace formula::test
