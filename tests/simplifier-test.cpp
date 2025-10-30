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

TEST_F(TestFormulaSimplifier, addTwoNumbers)
{
    const Expr simplified{simplify(binary(number(7.0), '+', number(12.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:19\n", str.str());
}

TEST_F(TestFormulaSimplifier, subtractTwoNumbers)
{
    const Expr simplified{simplify(binary(number(12.0), '-', number(7.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:5\n", str.str());
}

TEST_F(TestFormulaSimplifier, multiplyTwoNumbers)
{
    const Expr simplified{simplify(binary(number(12.0), '*', number(2.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:24\n", str.str());
}

TEST_F(TestFormulaSimplifier, divideTwoNumbers)
{
    const Expr simplified{simplify(binary(number(12.0), '/', number(2.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:6\n", str.str());
}

TEST_F(TestFormulaSimplifier, numberExpression)
{
    const Expr simplified{simplify(binary(number(12.0), '/', binary(number(1.0), '+', number(2.0))))};

    simplified->visit(formatter);
    EXPECT_EQ("number:4\n", str.str());
}

TEST_F(TestFormulaSimplifier, shortCircuitAnd)
{
    const Expr simplified{simplify(binary(number(0.0), "&&", identifier("x")))};

    simplified->visit(formatter);
    EXPECT_EQ("number:0\n", str.str());
}

TEST_F(TestFormulaSimplifier, shortCircuitOr)
{
    const Expr simplified{simplify(binary(number(12.0), "||", identifier("x")))};

    simplified->visit(formatter);
    EXPECT_EQ("number:1\n", str.str());
}

TEST_F(TestFormulaSimplifier, logicalAnd)
{
    const Expr simplified{simplify(binary(number(3.0), "&&", number(4.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:1\n", str.str());
}

TEST_F(TestFormulaSimplifier, logicalOr)
{
    const Expr simplified{simplify(binary(number(0.0), "||", number(3.0)))};

    simplified->visit(formatter);
    EXPECT_EQ("number:1\n", str.str());
}

} // namespace formula::test
