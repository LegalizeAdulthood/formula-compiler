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

TEST(TestLexer, whitespaceOnly)
{
    Lexer lexer("   \t  ");
    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::END_OF_INPUT, token.type);
}

TEST(TestLexer, terminatorLF)
{
    Lexer lexer("1\n2");
    Token tokens[3];
    for (Token &t : tokens)
    {
        t = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_EQ(TokenType::TERMINATOR, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
}

TEST(TestLexer, terminatorCRLF)
{
    Lexer lexer("1\r\n2");
    Token tokens[3];
    for (Token &t : tokens)
    {
        t = lexer.next_token();
    }

    EXPECT_EQ(TokenType::NUMBER, tokens[0].type);
    EXPECT_EQ(TokenType::TERMINATOR, tokens[1].type);
    EXPECT_EQ(TokenType::NUMBER, tokens[2].type);
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

TEST(TestLexer, lineContinuationWithLF)
{
    Lexer lexer("1\\\n2");

    const Token token1 = lexer.next_token();
    const Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
}

TEST(TestLexer, lineContinuationWithCRLF)
{
    Lexer lexer("1\\\r\n2");

    Token token1 = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
}

TEST(TestLexer, lineContinuationMultiple)
{
    Lexer lexer("1\\\n\\\n2");
    Token token1 = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
}

TEST(TestLexer, lineContinuationWithSpaces)
{
    Lexer lexer("1 \\\n  2");
    Token token1 = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
}

TEST(TestLexer, backslashNotFollowedByNewlineIsInvalid)
{
    Lexer lexer("1\\2");
    Token token1 = lexer.next_token();
    Token invalid = lexer.next_token();
    Token token2 = lexer.next_token();

    EXPECT_EQ(TokenType::NUMBER, token1.type);
    EXPECT_EQ(TokenType::INVALID, invalid.type);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
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

TEST(TestLexer, builtinVariablsHaveNameValue)
{
    Lexer lexer("maxit");

    Token token = lexer.next_token();

    EXPECT_EQ(TokenType::MAX_ITER, token.type);
    EXPECT_EQ("maxit", std::get<std::string>(token.value));
    EXPECT_EQ(0, token.position);
    EXPECT_EQ(5, token.length);
}

TEST(TestLexer, parenthesesWithIdentifiers)
{
    Lexer lexer("f(x)");
    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.next_token();
    }

    EXPECT_EQ(TokenType::IDENTIFIER, tokens[0].type);
    EXPECT_EQ("f", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::LEFT_PAREN, tokens[1].type);
    EXPECT_EQ(TokenType::IDENTIFIER, tokens[2].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::RIGHT_PAREN, tokens[3].type);
}

// Parameterized test for all builtin variables
struct TextTokenParam
{
    std::string_view name;
    std::string_view input;
    TokenType token{};
    size_t position{};
    size_t length{};
};

void PrintTo(const TextTokenParam &param, std::ostream *os)
{
    *os << param.name;
}

class TokenRecognized : public TestWithParam<TextTokenParam>
{
};

TEST_P(TokenRecognized, recognized)
{
    const TextTokenParam &param = GetParam();

    Lexer lexer(param.input);
    const Token token = lexer.next_token();

    EXPECT_EQ(param.token, token.type);
    EXPECT_EQ(param.position, token.position);
    EXPECT_EQ(param.length != 0 ? param.length : param.input.length(), token.length);
}

static TextTokenParam s_params[]{
    {"simpleInteger", "1", TokenType::NUMBER},                                //
    {"simpleDecimal", "3.14", TokenType::NUMBER},                             //
    {"decimalStartingWithPoint", ".5", TokenType::NUMBER},                    //
    {"scientificNotation", "1.5e10", TokenType::NUMBER},                      //
    {"scientificNotationNegativeExponent", "2.5e-3", TokenType::NUMBER},      //
    {"scientificNotationPositiveExponent", "3.7e+5", TokenType::NUMBER},      //
    {"scientificNotationUppercaseE", "1.2E6", TokenType::NUMBER},             //
    {"zero", "0", TokenType::NUMBER},                                         //
    {"integerWithLeadingZeros", "007", TokenType::NUMBER},                    //
    {"decimalWithTrailingZeros", "1.500", TokenType::NUMBER},                 //
    {"veryLargeNumber", "1.7976931348623157e+308", TokenType::NUMBER},        //
    {"verySmallNumber", "2.2250738585072014e-308", TokenType::NUMBER},        //
    {"skipsLeadingWhitespace", "  42", TokenType::NUMBER, 2, 2},              //
    {"emptyInput", "", TokenType::END_OF_INPUT},                              //
    {"plus", "+", TokenType::PLUS},                                           //
    {"minus", "-", TokenType::MINUS},                                         //
    {"multiply", "*", TokenType::MULTIPLY},                                   //
    {"divide", "/", TokenType::DIVIDE},                                       //
    {"modulus", "|", TokenType::MODULUS},                                     //
    {"power", "^", TokenType::POWER},                                         //
    {"assignment", "=", TokenType::ASSIGN},                                   //
    {"lessThan", "<", TokenType::LESS_THAN},                                  //
    {"greaterThan", ">", TokenType::GREATER_THAN},                            //
    {"lessEqual", "<=", TokenType::LESS_EQUAL},                               //
    {"greaterEqual", ">=", TokenType::GREATER_EQUAL},                         //
    {"equal", "==", TokenType::EQUAL},                                        //
    {"notEqual", "!=", TokenType::NOT_EQUAL},                                 //
    {"logicalAnd", "&&", TokenType::LOGICAL_AND},                             //
    {"logicalOr", "||", TokenType::LOGICAL_OR},                               //
    {"colon", ":", TokenType::COLON},                                         //
    {"comma", ",", TokenType::COMMA},                                         //
    {"newline", "\n", TokenType::TERMINATOR},                                 //
    {"invalidIdentifier", "1a", TokenType::INVALID},                          //
    {"simpleIdentifier", "x", TokenType::IDENTIFIER},                         //
    {"longerIdentifier", "variable", TokenType::IDENTIFIER},                  //
    {"identifierWithDigits", "var123", TokenType::IDENTIFIER},                //
    {"identifierWithUnderscore", "my_var", TokenType::IDENTIFIER},            //
    {"identifierStartingWithUnderscore", "_private", TokenType::IDENTIFIER},  //
    {"longIdentifier", "very_long_variable_name_123", TokenType::IDENTIFIER}, //
    {"upperCaseIdentifier", "CONSTANT", TokenType::IDENTIFIER},               //
    {"mixedCaseIdentifier", "camelCase", TokenType::IDENTIFIER},              //
    {"leftParenthesis", "(", TokenType::LEFT_PAREN},                          //
    {"rightParenthesis", ")", TokenType::RIGHT_PAREN},                        //
    {"if", "if", TokenType::IF},                                              //
    {"elseif", "elseif", TokenType::ELSE_IF},                                 //
    {"else", "else", TokenType::ELSE},                                        //
    {"endif", "endif", TokenType::END_IF},                                    //
    {"ifPrefix", "ifx", TokenType::IDENTIFIER},                               //
    {"ifSuffix", "xif", TokenType::IDENTIFIER},                               //
    {"elseIfPrefix", "elseif2", TokenType::IDENTIFIER},                       //
    {"elseSuffix", "myelse", TokenType::IDENTIFIER},                          //
    {"endIfPrefix", "endif_func", TokenType::IDENTIFIER},                     //
    {"global", "global", TokenType::GLOBAL},                                  //
    {"builtin", "builtin", TokenType::BUILTIN},                               //
    {"init", "init", TokenType::INIT},                                        //
    {"loop", "loop", TokenType::LOOP},                                        //
    {"bailout", "bailout", TokenType::BAILOUT},                               //
    {"perturbinit", "perturbinit", TokenType::PERTURB_INIT},                  //
    {"perturbloop", "perturbloop", TokenType::PERTURB_LOOP},                  //
    {"default", "default", TokenType::DEFAULT},                               //
    {"switch", "switch", TokenType::SWITCH},                                  //
    {"p1", "p1", TokenType::P1},                                              //
    {"p2", "p2", TokenType::P2},                                              //
    {"p3", "p3", TokenType::P3},                                              //
    {"p4", "p4", TokenType::P4},                                              //
    {"p5", "p5", TokenType::P5},                                              //
    {"pixel", "pixel", TokenType::PIXEL},                                     //
    {"lastsqr", "lastsqr", TokenType::LAST_SQR},                              //
    {"rand", "rand", TokenType::RAND},                                        //
    {"pi", "pi", TokenType::PI},                                              //
    {"e", "e", TokenType::E},                                                 //
    {"maxit", "maxit", TokenType::MAX_ITER},                                  //
    {"scrnmax", "scrnmax", TokenType::SCREEN_MAX},                            //
    {"scrnpix", "scrnpix", TokenType::SCREEN_PIXEL},                          //
    {"whitesq", "whitesq", TokenType::WHITE_SQUARE},                          //
    {"ismand", "ismand", TokenType::IS_MAND},                                 //
    {"center", "center", TokenType::CENTER},                                  //
    {"magxmag", "magxmag", TokenType::MAG_X_MAG},                             //
    {"rotskew", "rotskew", TokenType::ROT_SKEW},                              //
    {"sinh", "sinh", TokenType::SINH},                                        //
    {"cosh", "cosh", TokenType::COSH},                                        //
    {"cosxx", "cosxx", TokenType::COSXX},                                     //
    {"sin", "sin", TokenType::SIN},                                           //
    {"cos", "cos", TokenType::COS},                                           //
    {"cotanh", "cotanh", TokenType::COTANH},                                  //
    {"cotan", "cotan", TokenType::COTAN},                                     //
    {"tanh", "tanh", TokenType::TANH},                                        //
    {"tan", "tan", TokenType::TAN},                                           //
    {"sqrt", "sqrt", TokenType::SQRT},                                        //
    {"log", "log", TokenType::LOG},                                           //
    {"exp", "exp", TokenType::EXP},                                           //
    {"abs", "abs", TokenType::ABS},                                           //
    {"conj", "conj", TokenType::CONJ},                                        //
    {"real", "real", TokenType::REAL},                                        //
    {"imag", "imag", TokenType::IMAG},                                        //
    {"flip", "flip", TokenType::FLIP},                                        //
    {"fn1", "fn1", TokenType::FN1},                                           //
    {"fn2", "fn2", TokenType::FN2},                                           //
    {"fn3", "fn3", TokenType::FN3},                                           //
    {"fn4", "fn4", TokenType::FN4},                                           //
    {"srand", "srand", TokenType::SRAND},                                     //
    {"asinh", "asinh", TokenType::ASINH},                                     //
    {"acosh", "acosh", TokenType::ACOSH},                                     //
    {"asin", "asin", TokenType::ASIN},                                        //
    {"acos", "acos", TokenType::ACOS},                                        //
    {"atanh", "atanh", TokenType::ATANH},                                     //
    {"atan", "atan", TokenType::ATAN},                                        //
    {"cabs", "cabs", TokenType::CABS},                                        //
    {"sqr", "sqr", TokenType::SQR},                                           //
    {"floor", "floor", TokenType::FLOOR},                                     //
    {"ceil", "ceil", TokenType::CEIL},                                        //
    {"trunc", "trunc", TokenType::TRUNC},                                     //
    {"round", "round", TokenType::ROUND},                                     //
    {"ident", "ident", TokenType::IDENT},                                     //
    {"one", "one", TokenType::ONE},                                           //
    {"zero", "zero", TokenType::ZERO},                                        //
    {"commentAfter", "1;this is a comment", TokenType::NUMBER, 0, 1},         //
    {"commentBefore", ";this is a comment\n1", TokenType::TERMINATOR, 18, 1}, //
    {"continuation", "\\\n1", TokenType::NUMBER, 2, 1},                       //
};

INSTANTIATE_TEST_SUITE_P(TestLexing, TokenRecognized, ValuesIn(s_params));

} // namespace formula::test
