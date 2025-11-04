// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Lexer.h>

#include <gtest/gtest.h>

#include <cmath>
#include <string>

using namespace testing;

namespace formula::test
{

TEST(TestLexer, simpleInteger)
{
    Lexer lexer("1");
    Token token = lexer.next_token();
    Token end_token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(token.value));
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(1u, token.length);
    EXPECT_EQ(TokenType::END_OF_INPUT, end_token.type);
}

TEST(TestLexer, simpleDecimal)
{
    Lexer lexer("3.14");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(3.14, std::get<double>(token.value));
}

TEST(TestLexer, decimalStartingWithPoint)
{
    Lexer lexer(".5");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(0.5, std::get<double>(token.value));
}

TEST(TestLexer, scientificNotation)
{
    Lexer lexer("1.5e10");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1.5e10, std::get<double>(token.value));
}

TEST(TestLexer, scientificNotationNegativeExponent)
{
    Lexer lexer("2.5e-3");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(2.5e-3, std::get<double>(token.value));
}

TEST(TestLexer, scientificNotationPositiveExponent)
{
    Lexer lexer("3.7e+5");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(3.7e+5, std::get<double>(token.value));
}

TEST(TestLexer, scientificNotationUppercaseE)
{
    Lexer lexer("1.2E6");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1.2e6, std::get<double>(token.value));
}

TEST(TestLexer, negativeNumber)
{
    Lexer lexer("-42");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::MINUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(token.value));
}

TEST(TestLexer, positiveNumber)
{
    Lexer lexer("+42");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(token.value));
}

TEST(TestLexer, negativeDecimal)
{
    Lexer lexer("-3.14");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::MINUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(3.14, std::get<double>(token.value));
}

TEST(TestLexer, positiveDecimal)
{
    Lexer lexer("+2.718");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(2.718, std::get<double>(token.value));
}

TEST(TestLexer, negativeDecimalStartingWithPoint)
{
    Lexer lexer("-.5");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::MINUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(0.5, std::get<double>(token.value));
}

TEST(TestLexer, positiveDecimalStartingWithPoint)
{
    Lexer lexer("+.75");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(0.75, std::get<double>(token.value));
}

TEST(TestLexer, negativeScientificNotation)
{
    Lexer lexer("-1.5e10");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::MINUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1.5e10, std::get<double>(token.value));
}

TEST(TestLexer, positiveScientificNotation)
{
    Lexer lexer("+2.5e-3");
    Token op = lexer.next_token();
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(2.5e-3, std::get<double>(token.value));
}

TEST(TestLexer, zero)
{
    Lexer lexer("0");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(0.0, std::get<double>(token.value));
}

TEST(TestLexer, integerWithLeadingZeros)
{
    Lexer lexer("007");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(7.0, std::get<double>(token.value));
}

TEST(TestLexer, decimalWithTrailingZeros)
{
    Lexer lexer("1.500");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1.5, std::get<double>(token.value));
}

TEST(TestLexer, veryLargeNumber)
{
    Lexer lexer("1.7976931348623157e+308");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1.7976931348623157e+308, std::get<double>(token.value));
}

TEST(TestLexer, verySmallNumber)
{
    Lexer lexer("2.2250738585072014e-308");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(2.2250738585072014e-308, std::get<double>(token.value));
}

TEST(TestLexer, skipsLeadingWhitespace)
{
    Lexer lexer("   42");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(token.value));
}

TEST(TestLexer, skipsTrailingWhitespace)
{
    Lexer lexer("42   ");
    Token token = lexer.next_token();
    Token end_token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(token.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end_token.type);
}

TEST(TestLexer, multipleNumbers)
{
    Lexer lexer("1 2.5 3");
    Token token1 = lexer.next_token();
    Token token2 = lexer.next_token();
    Token token3 = lexer.next_token();
    Token end_token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.5, std::get<double>(token2.value));
    EXPECT_EQ(TokenType::NUMBER, token3.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token3.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end_token.type);
}

TEST(TestLexer, emptyInput)
{
    Lexer lexer("");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::END_OF_INPUT, token.type);
}

TEST(TestLexer, whitespaceOnly)
{
    Lexer lexer("   \t\n\r  ");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::END_OF_INPUT, token.type);
}

TEST(TestLexer, peekDoesNotAdvance)
{
    Lexer lexer("42 3.14");
    Token peek_token = lexer.peek_token();
    Token peek_again = lexer.peek_token(); // Peek again should return the same token
    Token token1 = lexer.next_token();     // Now actually consume it
    Token token2 = lexer.next_token();     // Next token should be the second number

    EXPECT_EQ(TokenType::NUMBER, peek_token.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(peek_token.value));
    EXPECT_EQ(TokenType::NUMBER, peek_again.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(peek_again.value));
    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.14, std::get<double>(token2.value));
}

TEST(TestLexer, positionTracking)
{
    Lexer lexer("  42  ");
    size_t pos = lexer.position();
    Token token = lexer.next_token();

    EXPECT_EQ(0u, pos);
    EXPECT_EQ(2u, token.position); // After skipping leading whitespace
    EXPECT_EQ(2u, token.length);
}

TEST(TestLexer, integerWithoutFraction)
{
    Lexer lexer("100");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(100.0, std::get<double>(token.value));
}

TEST(TestLexer, fractionWithoutInteger)
{
    Lexer lexer(".25");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(0.25, std::get<double>(token.value));
}

TEST(TestLexer, plusOperator)
{
    Lexer lexer("1+2");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(token2.value));
}

TEST(TestLexer, minusOperator)
{
    Lexer lexer("5-3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::MINUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, multiplyOperator)
{
    Lexer lexer("4*6");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::MULTIPLY, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(6.0, std::get<double>(token2.value));
}

TEST(TestLexer, divideOperator)
{
    Lexer lexer("8/2");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(8.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::DIVIDE, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(token2.value));
}

TEST(TestLexer, operatorsWithSpaces)
{
    Lexer lexer("10 + 20");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(10.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(20.0, std::get<double>(token2.value));
}

TEST(TestLexer, complexExpression)
{
    Lexer lexer("1+2*3-4/2");
    Token tokens[9];
    for (int i = 0; i < 9; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::MULTIPLY, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
    EXPECT_EQ(TokenType::MINUS, tokens[5].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[6].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[6].value));
    EXPECT_EQ(TokenType::DIVIDE, tokens[7].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[8].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[8].value));
}

TEST(TestLexer, negativeNumberAfterOperator)
{
    Lexer lexer("5*-3");
    Token token1 = lexer.next_token();
    Token op1 = lexer.next_token();
    Token op2 = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::MULTIPLY, op1.type);
    EXPECT_EQ(TokenType::MINUS, op2.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, positiveNumberAfterOperator)
{
    Lexer lexer("7/+2");
    Token token1 = lexer.next_token();
    Token op1 = lexer.next_token();
    Token op2 = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(7.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::DIVIDE, op1.type);
    EXPECT_EQ(TokenType::PLUS, op2.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(token2.value));
}

TEST(TestLexer, standaloneOperators)
{
    Lexer lexer("+ - * /");
    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::PLUS, tokens[0].type);
    EXPECT_EQ(TokenType::MINUS, tokens[1].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[2].type);
    EXPECT_EQ(TokenType::DIVIDE, tokens[3].type);
}

TEST(TestLexer, powerOperator)
{
    Lexer lexer("2^3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::POWER, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, powerWithSpaces)
{
    Lexer lexer("5 ^ 2");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::POWER, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(token2.value));
}

TEST(TestLexer, chainedPower)
{
    Lexer lexer("2^3^4");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::POWER, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::POWER, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[4].value));
}

TEST(TestLexer, powerInExpression)
{
    Lexer lexer("1+2^3*4");
    Token tokens[7];
    for (int i = 0; i < 7; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::POWER, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
    EXPECT_EQ(TokenType::MULTIPLY, tokens[5].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[6].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[6].value));
}

TEST(TestLexer, allOperators)
{
    Lexer lexer("+ - * / ^");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::PLUS, tokens[0].type);
    EXPECT_EQ(TokenType::MINUS, tokens[1].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[2].type);
    EXPECT_EQ(TokenType::DIVIDE, tokens[3].type);
    EXPECT_EQ(TokenType::POWER, tokens[4].type);
}

TEST(TestLexer, assignmentOperator)
{
    Lexer lexer("x=5");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token1.type);
    EXPECT_EQ("x", std::get<std::string>(token1.value));
    EXPECT_EQ(TokenType::ASSIGN, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token2.value));
}

TEST(TestLexer, assignmentWithSpaces)
{
    Lexer lexer("10 = 20");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(10.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::ASSIGN, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(20.0, std::get<double>(token2.value));
}

TEST(TestLexer, assignmentInExpression)
{
    Lexer lexer("2+3=5");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::ASSIGN, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(tokens[4].value));
}

TEST(TestLexer, multipleAssignments)
{
    Lexer lexer("1=2=3");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::ASSIGN, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::ASSIGN, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
}

TEST(TestLexer, allOperatorsWithAssignment)
{
    Lexer lexer("+ - * / ^ =");
    Token tokens[6];
    for (int i = 0; i < 6; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::PLUS, tokens[0].type);
    EXPECT_EQ(TokenType::MINUS, tokens[1].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[2].type);
    EXPECT_EQ(TokenType::DIVIDE, tokens[3].type);
    EXPECT_EQ(TokenType::POWER, tokens[4].type);
    EXPECT_EQ(TokenType::ASSIGN, tokens[5].type);
}

TEST(TestLexer, lessThanOperator)
{
    Lexer lexer("3<5");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::LESS_THAN, op.type);
    EXPECT_EQ(1u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token2.value));
}

TEST(TestLexer, greaterThanOperator)
{
    Lexer lexer("5>3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::GREATER_THAN, op.type);
    EXPECT_EQ(1u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, lessEqualOperator)
{
    Lexer lexer("3<=5");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::LESS_EQUAL, op.type);
    EXPECT_EQ(2u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token2.value));
}

TEST(TestLexer, greaterEqualOperator)
{
    Lexer lexer("5>=3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::GREATER_EQUAL, op.type);
    EXPECT_EQ(2u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, equalOperator)
{
    Lexer lexer("5==5");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::EQUAL, op.type);
    EXPECT_EQ(2u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token2.value));
}

TEST(TestLexer, relationalOperatorsWithSpaces)
{
    Lexer lexer("10 < 20");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(10.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::LESS_THAN, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(20.0, std::get<double>(token2.value));
}

TEST(TestLexer, chainedComparisons)
{
    Lexer lexer("1<2<=3>4>=5==6");
    Token tokens[11];
    for (int i = 0; i < 11; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::LESS_THAN, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::LESS_EQUAL, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
    EXPECT_EQ(TokenType::GREATER_THAN, tokens[5].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[6].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[6].value));
    EXPECT_EQ(TokenType::GREATER_EQUAL, tokens[7].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[8].type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(tokens[8].value));
    EXPECT_EQ(TokenType::EQUAL, tokens[9].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[10].type);
    EXPECT_DOUBLE_EQ(6.0, std::get<double>(tokens[10].value));
}

TEST(TestLexer, allRelationalOperators)
{
    Lexer lexer("< > <= >= == !=");
    Token tokens[6];
    for (int i = 0; i < 6; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::LESS_THAN, tokens[0].type);
    EXPECT_EQ(TokenType::GREATER_THAN, tokens[1].type);
    EXPECT_EQ(TokenType::LESS_EQUAL, tokens[2].type);
    EXPECT_EQ(TokenType::GREATER_EQUAL, tokens[3].type);
    EXPECT_EQ(TokenType::EQUAL, tokens[4].type);
    EXPECT_EQ(TokenType::NOT_EQUAL, tokens[5].type);
}

TEST(TestLexer, mixedArithmeticAndRelational)
{
    Lexer lexer("1+2<3*4");
    Token tokens[7];
    for (int i = 0; i < 7; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::LESS_THAN, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
    EXPECT_EQ(TokenType::MULTIPLY, tokens[5].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[6].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[6].value));
}

TEST(TestLexer, assignmentVsEquality)
{
    Lexer lexer("x=5 y==5");
    Token tokens[6];
    for (int i = 0; i < 6; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::ASSIGN, tokens[1].type);
    EXPECT_EQ(1u, tokens[1].length);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("y", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::EQUAL, tokens[4].type);
    EXPECT_EQ(2u, tokens[4].length);
    EXPECT_EQ(TokenType::NUMBER, tokens[5].type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(tokens[5].value));
}

TEST(TestLexer, modulusOperator)
{
    Lexer lexer("5|3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::MODULUS, op.type);
    EXPECT_EQ(1u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, modulusWithSpaces)
{
    Lexer lexer("10 | 3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(10.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::MODULUS, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, doubleModulus)
{
    Lexer lexer("|-5|");
    Token op1 = lexer.next_token();
    Token op2 = lexer.next_token();
    Token num = lexer.next_token();
    Token op3 = lexer.next_token();

    EXPECT_EQ(TokenType::MODULUS, op1.type);
    EXPECT_EQ(TokenType::MINUS, op2.type);
    EXPECT_EQ(TokenType::NUMBER, num.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(num.value));
    EXPECT_EQ(TokenType::MODULUS, op3.type);
}

TEST(TestLexer, modulusInExpression)
{
    Lexer lexer("1+|2-3|*4");
    Token tokens[9];
    for (int i = 0; i < 9; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::MODULUS, tokens[2].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[3].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[3].value));
    EXPECT_EQ(TokenType::MINUS, tokens[4].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[5].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[5].value));
    EXPECT_EQ(TokenType::MODULUS, tokens[6].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[7].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[8].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[8].value));
}

TEST(TestLexer, allOperatorsWithModulus)
{
    Lexer lexer("+ - * / ^ = |");
    Token tokens[7];
    for (int i = 0; i < 7; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::PLUS, tokens[0].type);
    EXPECT_EQ(TokenType::MINUS, tokens[1].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[2].type);
    EXPECT_EQ(TokenType::DIVIDE, tokens[3].type);
    EXPECT_EQ(TokenType::POWER, tokens[4].type);
    EXPECT_EQ(TokenType::ASSIGN, tokens[5].type);
    EXPECT_EQ(TokenType::MODULUS, tokens[6].type);
}

TEST(TestLexer, notEqualOperator)
{
    Lexer lexer("5!=3");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::NOT_EQUAL, op.type);
    EXPECT_EQ(2u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(token2.value));
}

TEST(TestLexer, notEqualWithSpaces)
{
    Lexer lexer("10 != 20");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(10.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::NOT_EQUAL, op.type);
    EXPECT_EQ(2u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(20.0, std::get<double>(token2.value));
}

TEST(TestLexer, equalityOperators)
{
    Lexer lexer("1==2 3!=4");
    Token tokens[6];
    for (int i = 0; i < 6; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::EQUAL, tokens[1].type);
    EXPECT_EQ(2u, tokens[1].length);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::NUMBER, tokens[3].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[3].value));
    EXPECT_EQ(TokenType::NOT_EQUAL, tokens[4].type);
    EXPECT_EQ(2u, tokens[4].length);
    EXPECT_EQ(TokenType::NUMBER, tokens[5].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[5].value));
}

TEST(TestLexer, notEqualInExpression)
{
    Lexer lexer("1+2!=3*4");
    Token tokens[7];
    for (int i = 0; i < 7; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::NOT_EQUAL, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
    EXPECT_EQ(TokenType::MULTIPLY, tokens[5].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[6].type);
    EXPECT_DOUBLE_EQ(4.0, std::get<double>(tokens[6].value));
}

TEST(TestLexer, allComparisonOperators)
{
    Lexer lexer("< > <= >= == !=");
    Token tokens[6];
    for (int i = 0; i < 6; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::LESS_THAN, tokens[0].type);
    EXPECT_EQ(TokenType::GREATER_THAN, tokens[1].type);
    EXPECT_EQ(TokenType::LESS_EQUAL, tokens[2].type);
    EXPECT_EQ(TokenType::GREATER_EQUAL, tokens[3].type);
    EXPECT_EQ(TokenType::EQUAL, tokens[4].type);
    EXPECT_EQ(TokenType::NOT_EQUAL, tokens[5].type);
}

TEST(TestLexer, singleExclamationInvalid)
{
    Lexer lexer("1!2");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(token1.value));
    EXPECT_EQ(TokenType::INVALID, op.type);
    EXPECT_EQ(1u, op.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(token2.value));
}

TEST(TestLexer, chainedNotEqual)
{
    Lexer lexer("1!=2!=3");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[0].value));
    EXPECT_EQ(TokenType::NOT_EQUAL, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::NOT_EQUAL, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[4].value));
}

TEST(TestLexer, simpleIdentifier)
{
    Lexer lexer("x");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("x", std::get<std::string>(token.value));
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(1u, token.length);
}

TEST(TestLexer, longerIdentifier)
{
    Lexer lexer("variable");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("variable", std::get<std::string>(token.value));
}

TEST(TestLexer, identifierWithDigits)
{
    Lexer lexer("var123");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("var123", std::get<std::string>(token.value));
}

TEST(TestLexer, identifierWithUnderscore)
{
    Lexer lexer("my_var");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("my_var", std::get<std::string>(token.value));
}

TEST(TestLexer, identifierStartingWithUnderscore)
{
    Lexer lexer("_private");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("_private", std::get<std::string>(token.value));
}

TEST(TestLexer, multipleIdentifiers)
{
    Lexer lexer("x y z");
    Token tokens[3];
    for (int i = 0; i < 3; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("y", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("z", std::get<std::string>(tokens[2].value));
}

TEST(TestLexer, identifierInAssignment)
{
    Lexer lexer("x=5");
    Token token1 = lexer.next_token();
    Token op = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token1.type);
    EXPECT_EQ("x", std::get<std::string>(token1.value));
    EXPECT_EQ(TokenType::ASSIGN, op.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(5.0, std::get<double>(token2.value));
}

TEST(TestLexer, identifiersInExpression)
{
    Lexer lexer("a+b*c");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("a", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("b", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::MULTIPLY, tokens[3].type);
    EXPECT_EQ(TokenType::IDENT, tokens[4].type);
    EXPECT_EQ("c", std::get<std::string>(tokens[4].value));
}

TEST(TestLexer, mixedIdentifiersAndNumbers)
{
    Lexer lexer("x1 + 2y");
    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("x1", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::PLUS, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("y", std::get<std::string>(tokens[3].value));
}

TEST(TestLexer, identifierWithComparison)
{
    Lexer lexer("x==y");
    Token tokens[3];
    for (int i = 0; i < 3; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::EQUAL, tokens[1].type);
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("y", std::get<std::string>(tokens[2].value));
}

TEST(TestLexer, longIdentifier)
{
    Lexer lexer("very_long_variable_name_123");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("very_long_variable_name_123", std::get<std::string>(token.value));
}

TEST(TestLexer, upperCaseIdentifier)
{
    Lexer lexer("CONSTANT");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("CONSTANT", std::get<std::string>(token.value));
}

TEST(TestLexer, mixedCaseIdentifier)
{
    Lexer lexer("camelCase");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IDENT, token.type);
    EXPECT_EQ("camelCase", std::get<std::string>(token.value));
}

TEST(TestLexer, leftParenthesis)
{
    Lexer lexer("(");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::LEFT_PAREN, token.type);
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(1u, token.length);
}

TEST(TestLexer, rightParenthesis)
{
    Lexer lexer(")");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::RIGHT_PAREN, token.type);
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(1u, token.length);
}

TEST(TestLexer, matchingParentheses)
{
    Lexer lexer("()");
    Token left = lexer.next_token();
    Token right = lexer.next_token();

    EXPECT_EQ(TokenType::LEFT_PAREN, left.type);
    EXPECT_EQ(TokenType::RIGHT_PAREN, right.type);
}

TEST(TestLexer, parenthesesWithSpaces)
{
    Lexer lexer("( )");
    Token left = lexer.next_token();
    Token right = lexer.next_token();

    EXPECT_EQ(TokenType::LEFT_PAREN, left.type);
    EXPECT_EQ(TokenType::RIGHT_PAREN, right.type);
}

TEST(TestLexer, parenthesesInExpression)
{
    Lexer lexer("(1+2)*3");
    Token tokens[7];
    for (int i = 0; i < 7; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[0].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[1].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[1].value));
    EXPECT_EQ(TokenType::PLUS, tokens[2].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[3].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[3].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[4].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[5].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[6].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[6].value));
}

TEST(TestLexer, nestedParentheses)
{
    Lexer lexer("((1+2)*3)");
    Token tokens[9];
    for (int i = 0; i < 9; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[0].type);
    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
    EXPECT_DOUBLE_EQ(1.0, std::get<double>(tokens[2].value));
    EXPECT_EQ(TokenType::PLUS, tokens[3].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[4].type);
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(tokens[4].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[5].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[6].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[7].type);
    EXPECT_DOUBLE_EQ(3.0, std::get<double>(tokens[7].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[8].type);
}

TEST(TestLexer, parenthesesWithIdentifiers)
{
    Lexer lexer("f(x)");
    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("f", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[1].type);
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[3].type);
}

TEST(TestLexer, functionCallWithMultipleArgs)
{
    Lexer lexer("func(a, b)");
    Token tokens[6];
    for (int i = 0; i < 6; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("func", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[1].type);
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("a", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::INVALID, tokens[3].type); // comma not yet recognized
    EXPECT_EQ(TokenType::IDENT, tokens[4].type);
    EXPECT_EQ("b", std::get<std::string>(tokens[4].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[5].type);
}

TEST(TestLexer, emptyParentheses)
{
    Lexer lexer("f()");
    Token tokens[3];
    for (int i = 0; i < 3; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("f", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[1].type);
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[2].type);
}

TEST(TestLexer, parenthesesWithComparison)
{
    Lexer lexer("(x>y)");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[0].type);
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::GREATER_THAN, tokens[2].type);
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("y", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[4].type);
}

TEST(TestLexer, multipleParenthesisGroups)
{
    Lexer lexer("(a+b)*(c+d)");
    Token tokens[11];
    for (int i = 0; i < 11; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[0].type);
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("a", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::PLUS, tokens[2].type);
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("b", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[4].type);
    EXPECT_EQ(TokenType::MULTIPLY, tokens[5].type);
    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[6].type);
    EXPECT_EQ(TokenType::IDENT, tokens[7].type);
    EXPECT_EQ("c", std::get<std::string>(tokens[7].value));
    EXPECT_EQ(TokenType::PLUS, tokens[8].type);
    EXPECT_EQ(TokenType::IDENT, tokens[9].type);
    EXPECT_EQ("d", std::get<std::string>(tokens[9].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[10].type);
}

TEST(TestLexer, ifKeyword)
{
    Lexer lexer("if");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::IF, token.type);
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(2u, token.length);
}

TEST(TestLexer, elseifKeyword)
{
    Lexer lexer("elseif");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::ELSE_IF, token.type);
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(6u, token.length);
}

TEST(TestLexer, elseKeyword)
{
    Lexer lexer("else");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::ELSE, token.type);
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(4u, token.length);
}

TEST(TestLexer, endifKeyword)
{
    Lexer lexer("endif");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::END_IF, token.type);
    EXPECT_EQ(0u, token.position);
    EXPECT_EQ(5u, token.length);
}

TEST(TestLexer, ifStatement)
{
    Lexer lexer("if x>0");
    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IF, tokens[0].type);
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::GREATER_THAN, tokens[2].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[3].type);
    EXPECT_DOUBLE_EQ(0.0, std::get<double>(tokens[3].value));
}

TEST(TestLexer, ifElseEndif)
{
    Lexer lexer("if x else y endif");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IF, tokens[0].type);
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::ELSE, tokens[2].type);
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("y", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::END_IF, tokens[4].type);
}

TEST(TestLexer, ifElseifElseEndif)
{
    Lexer lexer("if a elseif b else c endif");
    Token tokens[7];
    for (int i = 0; i < 7; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IF, tokens[0].type);
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("a", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::ELSE_IF, tokens[2].type);
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("b", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::ELSE, tokens[4].type);
    EXPECT_EQ(TokenType::IDENT, tokens[5].type);
    EXPECT_EQ("c", std::get<std::string>(tokens[5].value));
    EXPECT_EQ(TokenType::END_IF, tokens[6].type);
}

TEST(TestLexer, keywordsAreCaseSensitive)
{
    Lexer lexer("IF If iF Elseif ELSEIF Else ELSE Endif ENDIF");
    Token tokens[9];
    for (int i = 0; i < 9; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    // All uppercase/mixed case should be identifiers
    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("IF", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("If", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("iF", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("Elseif", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::IDENT, tokens[4].type);
    EXPECT_EQ("ELSEIF", std::get<std::string>(tokens[4].value));
    EXPECT_EQ(TokenType::IDENT, tokens[5].type);
    EXPECT_EQ("Else", std::get<std::string>(tokens[5].value));
    EXPECT_EQ(TokenType::IDENT, tokens[6].type);
    EXPECT_EQ("ELSE", std::get<std::string>(tokens[6].value));
    EXPECT_EQ(TokenType::IDENT, tokens[7].type);
    EXPECT_EQ("Endif", std::get<std::string>(tokens[7].value));
    EXPECT_EQ(TokenType::IDENT, tokens[8].type);
    EXPECT_EQ("ENDIF", std::get<std::string>(tokens[8].value));
}

TEST(TestLexer, keywordsAsPartOfIdentifier)
{
    Lexer lexer("ifx xif elseif2 myelse endif_func");
    Token tokens[5];
    for (int i = 0; i < 5; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    // Keywords as part of larger identifiers should remain identifiers
    EXPECT_EQ(TokenType::IDENT, tokens[0].type);
    EXPECT_EQ("ifx", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::IDENT, tokens[1].type);
    EXPECT_EQ("xif", std::get<std::string>(tokens[1].value));
    EXPECT_EQ(TokenType::IDENT, tokens[2].type);
    EXPECT_EQ("elseif2", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::IDENT, tokens[3].type);
    EXPECT_EQ("myelse", std::get<std::string>(tokens[3].value));
    EXPECT_EQ(TokenType::IDENT, tokens[4].type);
    EXPECT_EQ("endif_func", std::get<std::string>(tokens[4].value));
}

// Parameterized test for all builtin variables
struct BuiltinVariableParam
{
    std::string input;
    TokenType expected_type;
};

void PrintTo(const BuiltinVariableParam &param, std::ostream *os)
{
    *os << param.input;
}

class BuiltinVariable : public TestWithParam<BuiltinVariableParam>
{
};

TEST_P(BuiltinVariable, recognized)
{
    const auto &test_case = GetParam();

    Lexer lexer(test_case.input);
    const Token token = lexer.next_token();

    EXPECT_EQ(test_case.expected_type, token.type);
    EXPECT_EQ(0, token.position);
    EXPECT_EQ(test_case.input.length(), token.length);
}

INSTANTIATE_TEST_SUITE_P(TestLexing, BuiltinVariable,
    Values(BuiltinVariableParam{"p1", TokenType::P1},             //
        BuiltinVariableParam{"p2", TokenType::P2},                //
        BuiltinVariableParam{"p3", TokenType::P3},                //
        BuiltinVariableParam{"p4", TokenType::P4},                //
        BuiltinVariableParam{"p5", TokenType::P5},                //
        BuiltinVariableParam{"pixel", TokenType::PIXEL},          //
        BuiltinVariableParam{"lastsqr", TokenType::LAST_SQR},     //
        BuiltinVariableParam{"rand", TokenType::RAND},            //
        BuiltinVariableParam{"pi", TokenType::PI},                //
        BuiltinVariableParam{"e", TokenType::E},                  //
        BuiltinVariableParam{"maxit", TokenType::MAX_ITER},       //
        BuiltinVariableParam{"scrnmax", TokenType::SCREEN_MAX},   //
        BuiltinVariableParam{"scrnpix", TokenType::SCREEN_PIXEL}, //
        BuiltinVariableParam{"whitesq", TokenType::WHITE_SQUARE}, //
        BuiltinVariableParam{"ismand", TokenType::IS_MAND},       //
        BuiltinVariableParam{"center", TokenType::CENTER},        //
        BuiltinVariableParam{"magxmag", TokenType::MAG_X_MAG},    //
        BuiltinVariableParam{"rotskew", TokenType::ROT_SKEW}));

TEST(TestLexer, allKeywords)
{
    Lexer lexer("if elseif else endif");
    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IF, tokens[0].type);
    EXPECT_EQ(TokenType::ELSE_IF, tokens[1].type);
    EXPECT_EQ(TokenType::ELSE, tokens[2].type);
    EXPECT_EQ(TokenType::END_IF, tokens[3].type);
}

} // namespace formula::test
