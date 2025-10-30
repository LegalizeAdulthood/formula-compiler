// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include "node-builders.h"
#include "NodeFormatter.h"

#include <formula/Node.h>
#include <formula/Visitor.h>

#include <gtest/gtest.h>

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

TEST_F(TestFormulaSimplifier, unaryNumber)
{
    const Expr simplified{simplify(unary('-', number(-42.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:42\n", str.str());
}

struct BinaryOpTestParam
{
    Expr expression;
    std::string expected;
    std::string test_name;
};

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
    {binary(number(12.0), '/', binary(number(1.0), '+', number(2.0))), "number:4\n", "numberExpression"},
    {binary(number(0.0), "&&", identifier("x")), "number:0\n", "shortCircuitAnd"},
    {binary(number(12.0), "||", identifier("x")), "number:1\n", "shortCircuitOr"},
    {binary(number(3.0), "&&", number(4.0)), "number:1\n", "logicalAnd"},
    {binary(number(0.0), "||", number(3.0)), "number:1\n", "logicalOr"}};

INSTANTIATE_TEST_SUITE_P(BinaryOperations, TestSimplifyBinaryOp, ValuesIn(s_binary_op_test_params),
    [](const TestParamInfo<BinaryOpTestParam> &info) { return info.param.test_name; });

} // namespace formula::test
