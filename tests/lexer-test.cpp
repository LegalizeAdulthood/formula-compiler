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

} // namespace formula::test
