// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Lexer.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>

using namespace testing;

namespace formula::test
{

TEST(TestLexer, skipsTrailingWhitespace)
{
    Lexer lexer{"42   "};

    const Token token{lexer.get_token()};
    const Token end_token{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token.type);
    EXPECT_EQ(42, std::get<int>(token.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end_token.type);
}

TEST(TestLexer, multipleNumbers)
{
    Lexer lexer{"1 2.5 3"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};
    const Token token3{lexer.get_token()};
    const Token end_token{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(1, std::get<int>(token1.value));
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(2.5, std::get<double>(token2.value));
    EXPECT_EQ(TokenType::INTEGER, token3.type);
    EXPECT_EQ(3, std::get<int>(token3.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end_token.type);
}

TEST(TestLexer, whitespaceOnly)
{
    Lexer lexer{"   \t  "};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::END_OF_INPUT, token.type);
}

TEST(TestLexer, terminatorLF)
{
    Lexer lexer{"1\n2"};

    Token tokens[3];
    for (Token &t : tokens)
    {
        t = lexer.get_token();
    }

    EXPECT_EQ(TokenType::INTEGER, tokens[0].type);
    EXPECT_EQ(TokenType::TERMINATOR, tokens[1].type);
    EXPECT_EQ(TokenType::INTEGER, tokens[2].type);
}

TEST(TestLexer, terminatorCRLF)
{
    Lexer lexer{"1\r\n2"};

    Token tokens[3];
    for (Token &t : tokens)
    {
        t = lexer.get_token();
    }

    EXPECT_EQ(TokenType::INTEGER, tokens[0].type);
    EXPECT_EQ(TokenType::TERMINATOR, tokens[1].type);
    EXPECT_EQ(TokenType::INTEGER, tokens[2].type);
}

TEST(TestLexer, peekDoesNotAdvance)
{
    Lexer lexer{"42 3.14"};

    const Token peek_token{lexer.peek_token()};
    const Token peek_again{lexer.peek_token()};
    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, peek_token.type);
    EXPECT_EQ(42, std::get<int>(peek_token.value));
    EXPECT_EQ(TokenType::INTEGER, peek_again.type);
    EXPECT_EQ(42, std::get<int>(peek_again.value));
    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(42, std::get<int>(token1.value));
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.14, std::get<double>(token2.value));
}

TEST(TestLexer, lineContinuationWithLF)
{
    Lexer lexer{"1\\\n2"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
}

TEST(TestLexer, lineContinuationWithCRLF)
{
    Lexer lexer{"1\\\r\n2"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
}

TEST(TestLexer, lineContinuationMultiple)
{
    Lexer lexer{"1\\\n\\\n2"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
}

TEST(TestLexer, lineContinuationWithSpaces)
{
    Lexer lexer{"1 \\\n  2"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
}

TEST(TestLexer, lineContinuationWithTrailingWhitespace)
{
    Lexer lexer{"1\\ \n2"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
    ASSERT_FALSE(lexer.get_warnings().empty()) << "lexer should have produced a warning";
    const LexicalDiagnostic &warning{lexer.get_warnings().front()};
    EXPECT_EQ(LexerErrorCode::CONTINUATION_WITH_WHITESPACE, warning.code);
    EXPECT_EQ(1U, warning.location.line);
    EXPECT_EQ(3U, warning.location.column);
}

TEST(TestLexer, lineContinuationWithTrailingWhitespaceAndCRLF)
{
    Lexer lexer{"1\\ \r\n2"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
    ASSERT_FALSE(lexer.get_warnings().empty()) << "lexer should have produced a warning";
    const LexicalDiagnostic &warning{lexer.get_warnings().front()};
    EXPECT_EQ(LexerErrorCode::CONTINUATION_WITH_WHITESPACE, warning.code);
    EXPECT_EQ(1U, warning.location.line);
    EXPECT_EQ(3U, warning.location.column);
}

TEST(TestLexer, multipleWarnings)
{
    Lexer lexer{"1\\ \n2\\ \n3\n"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};
    const Token token3{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
    EXPECT_EQ(TokenType::INTEGER, token3.type);
    const auto &warnings{lexer.get_warnings()};
    ASSERT_EQ(2, warnings.size()) << "lexer should have produced two warnings";
    EXPECT_TRUE(std::all_of(warnings.begin(), warnings.end(),
        [](const LexicalDiagnostic &w) { return w.code == LexerErrorCode::CONTINUATION_WITH_WHITESPACE; }))
        << "all warnings should be CONTINUATION_WITH_WHITESPACE";
    EXPECT_EQ(1U, warnings[0].location.line) << "line of first warning";
    EXPECT_EQ(3U, warnings[0].location.column) << "column of first warning";
    EXPECT_EQ(2U, warnings[1].location.line) << "line of second warning";
    EXPECT_EQ(3U, warnings[1].location.column) << "column of second warning";
}

TEST(TestLexer, backslashNotFollowedByNewlineIsInvalid)
{
    Lexer lexer{"1\\2"};

    const Token token1{lexer.get_token()};
    const Token invalid{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INVALID, invalid.type);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
}

TEST(TestLexer, positionTracking)
{
    Lexer lexer{"  42  "};

    size_t pos = lexer.position();
    const Token token{lexer.get_token()};

    EXPECT_EQ(0U, pos);
    EXPECT_EQ(3U, token.location.column);
    EXPECT_EQ(2U, token.length);
}

TEST(TestLexer, singleExclamationInvalid)
{
    Lexer lexer{"1!2"};

    const Token token1{lexer.get_token()};
    const Token op{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(1, std::get<int>(token1.value));
    EXPECT_EQ(TokenType::INVALID, op.type);
    EXPECT_EQ(1U, op.length);
    EXPECT_EQ(TokenType::INTEGER, token2.type);
    EXPECT_EQ(2, std::get<int>(token2.value));
}

TEST(TestLexer, varColon)
{
    Lexer lexer{"ball_size:"};

    const Token t1{lexer.get_token()};
    const Token t2{lexer.get_token()};

    EXPECT_EQ(TokenType::IDENTIFIER, t1.type);
    EXPECT_EQ("ball_size", std::get<std::string>(t1.value));
    EXPECT_EQ(1U, t1.location.column);
    EXPECT_EQ(9, t1.length);
    EXPECT_EQ(TokenType::COLON, t2.type);
    EXPECT_EQ(10, t2.location.column);
}

TEST(TestLexer, builtinVariablsHaveNameValue)
{
    Lexer lexer{"maxit"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::MAX_ITER, token.type);
    EXPECT_EQ("maxit", std::get<std::string>(token.value));
    EXPECT_EQ(1U, token.location.column);
    EXPECT_EQ(5U, token.length);
}

TEST(TestLexer, parenthesesWithIdentifiers)
{
    Lexer lexer{"f(x)"};

    Token tokens[4];
    for (int i = 0; i < 4; ++i)
    {
        tokens[i] = lexer.get_token();
    }

    EXPECT_EQ(TokenType::IDENTIFIER, tokens[0].type);
    EXPECT_EQ("f", std::get<std::string>(tokens[0].value));
    EXPECT_EQ(TokenType::OPEN_PAREN, tokens[1].type);
    EXPECT_EQ(TokenType::IDENTIFIER, tokens[2].type);
    EXPECT_EQ("x", std::get<std::string>(tokens[2].value));
    EXPECT_EQ(TokenType::CLOSE_PAREN, tokens[3].type);
}

TEST(TestLexer, identifiersAreMadeLowerCase)
{
    Lexer lexer{"FOO"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::IDENTIFIER, token.type);
    EXPECT_EQ("foo", std::get<std::string>(token.value));
}

TEST(TestLexer, stringWithEscapedQuotes)
{
    Lexer lexer{R"text("He said \"Hello\" to me")text"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::STRING, token.type);
    EXPECT_EQ(R"text(He said "Hello" to me)text", std::get<std::string>(token.value));
}

TEST(TestLexer, stringWithMultipleEscapedQuotes)
{
    Lexer lexer{R"text("She said \"Hello, Doctor Frankenstein\" and he laughed.")text"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::STRING, token.type);
    EXPECT_EQ(R"text(She said "Hello, Doctor Frankenstein" and he laughed.)text", std::get<std::string>(token.value));
}

TEST(TestLexer, emptyString)
{
    Lexer lexer{R"text("")text"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::STRING, token.type);
    EXPECT_EQ("", std::get<std::string>(token.value));
}

TEST(TestLexer, stringUnterminatedInvalid)
{
    Lexer lexer{R"text("unterminated)text"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::INVALID, token.type);
}

TEST(TestLexer, stringWithNewlineInvalid)
{
    Lexer lexer{"\"line1\nline2\""};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::INVALID, token.type);
}

TEST(TestLexer, stringWithEscapedChar)
{
    Lexer lexer{R"text("line1\nline2")text"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::STRING, token.type);
    EXPECT_EQ(R"text(line1nline2)text", std::get<std::string>(token.value));
}

TEST(TestLexer, stringWithEscapedBackslash)
{
    Lexer lexer{R"text("path\\to\\file")text"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::STRING, token.type);
    EXPECT_EQ(R"(path\to\file)", std::get<std::string>(token.value));
}

TEST(TestLexer, integerLiteral)
{
    Lexer lexer{"42"};
    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token.type);
    EXPECT_EQ(42, std::get<int>(token.value));
}

TEST(TestLexer, floatingPointLiteral)
{
    Lexer lexer{"42.5"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(42.5, std::get<double>(token.value));
}

TEST(TestLexer, scientificNotationIsFloat)
{
    Lexer lexer{"1e10"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(1e10, std::get<double>(token.value));
}

TEST(TestLexer, decimalPointMakesFloat)
{
    Lexer lexer{"42.0"};

    const Token token{lexer.get_token()};

    EXPECT_EQ(TokenType::NUMBER, token.type);
    EXPECT_DOUBLE_EQ(42.0, std::get<double>(token.value));
}

// Parameterized test for all builtin variables
struct TextTokenParam
{
    std::string_view name;
    std::string_view input;
    const TokenType token{};
    size_t column{1};
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
    Lexer lexer{param.input};

    const Token token{lexer.get_token()};

    EXPECT_EQ(param.token, token.type);
    EXPECT_EQ(param.column, token.location.column);
    EXPECT_EQ(param.length != 0 ? param.length : param.input.length(), token.length);
}

static TextTokenParam s_params[]{
    {"simpleInteger", "1", TokenType::INTEGER},                               //
    {"simpleDecimal", "3.14", TokenType::NUMBER},                             //
    {"decimalStartingWithPoint", ".5", TokenType::NUMBER},                    //
    {"scientificNotation", "1.5e10", TokenType::NUMBER},                      //
    {"scientificNotationNegativeExponent", "2.5e-3", TokenType::NUMBER},      //
    {"scientificNotationPositiveExponent", "3.7e+5", TokenType::NUMBER},      //
    {"scientificNotationUppercaseE", "1.2E6", TokenType::NUMBER},             //
    {"zero", "0", TokenType::INTEGER},                                        //
    {"integerWithLeadingZeros", "007", TokenType::INTEGER},                   //
    {"decimalWithTrailingZeros", "1.500", TokenType::NUMBER},                 //
    {"veryLargeNumber", "1.7976931348623157e+308", TokenType::NUMBER},        //
    {"verySmallNumber", "2.2250738585072014e-308", TokenType::NUMBER},        //
    {"skipsLeadingWhitespace", "  42", TokenType::INTEGER, 3, 2},             //
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
    {"leftParenthesis", "(", TokenType::OPEN_PAREN},                          //
    {"rightParenthesis", ")", TokenType::CLOSE_PAREN},                        //
    {"if", "if", TokenType::IF},                                              //
    {"elseif", "elseif", TokenType::ELSE_IF},                                 //
    {"else", "else", TokenType::ELSE},                                        //
    {"endif", "endif", TokenType::END_IF},                                    //
    {"ifPrefix", "ifx", TokenType::IDENTIFIER},                               //
    {"ifSuffix", "xif", TokenType::IDENTIFIER},                               //
    {"elseIfPrefix", "elseif2", TokenType::IDENTIFIER},                       //
    {"elseSuffix", "myelse", TokenType::IDENTIFIER},                          //
    {"endIfPrefix", "endif_func", TokenType::IDENTIFIER},                     //
    {"global", "global:", TokenType::GLOBAL, 1, 7},                           //
    {"builtin", "builtin:", TokenType::BUILTIN, 1, 8},                        //
    {"init", "init:", TokenType::INIT, 1, 5},                                 //
    {"loop", "loop:", TokenType::LOOP, 1, 5},                                 //
    {"bailout", "bailout:", TokenType::BAILOUT, 1, 8},                        //
    {"perturbinit", "perturbinit:", TokenType::PERTURB_INIT, 1, 12},          //
    {"perturbloop", "perturbloop:", TokenType::PERTURB_LOOP, 1, 12},          //
    {"default", "default:", TokenType::DEFAULT, 1, 8},                        //
    {"switch", "switch:", TokenType::SWITCH, 1, 7},                           //
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
    {"commentAfter", "1;this is a comment", TokenType::INTEGER, 1, 1},        //
    {"commentBefore", ";this is a comment\n1", TokenType::TERMINATOR, 1, 1},  //
    {"continuation", "\\\n1", TokenType::INTEGER, 1, 1},                      //
    {"true", "true", TokenType::TRUE},                                        //
    {"false", "false", TokenType::FALSE},                                     //
    {"string", R"text("Some text.")text", TokenType::STRING},                 //
    {"boolType", "bool", TokenType::TYPE_BOOL},                               //
    {"intType", "int", TokenType::TYPE_INT},                                  //
    {"floatType", "float", TokenType::TYPE_FLOAT},                            //
    {"complexType", "complex", TokenType::TYPE_COMPLEX},                      //
    {"colorType", "color", TokenType::TYPE_COLOR},                            //
    {"beginParam", "param", TokenType::PARAM},                                //
    {"endParam", "endparam", TokenType::END_PARAM},                           //
    {"caseInsensitiveKeyword", "IF", TokenType::IF},                          //
};

INSTANTIATE_TEST_SUITE_P(TestLexing, TokenRecognized, ValuesIn(s_params));

TEST(TestLexer, putTokenSingleToken)
{
    Lexer lexer{"42 3.14"};

    const Token token1{lexer.get_token()};
    lexer.put_token(token1);
    const Token token_again{lexer.get_token()};
    const Token token2{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(42, std::get<int>(token1.value));
    EXPECT_EQ(TokenType::INTEGER, token_again.type);
    EXPECT_EQ(42, std::get<int>(token_again.value));
    EXPECT_EQ(token1.location, token_again.location);
    EXPECT_EQ(token1.length, token_again.length);
    EXPECT_EQ(TokenType::NUMBER, token2.type);
    EXPECT_DOUBLE_EQ(3.14, std::get<double>(token2.value));
}

TEST(TestLexer, putTokenMultipleTokens)
{
    Lexer lexer{"1 2 3"};

    const Token token1{lexer.get_token()};
    const Token token2{lexer.get_token()};
    lexer.put_token(token1);
    lexer.put_token(token2);
    const Token retrieved1{lexer.get_token()};
    const Token retrieved2{lexer.get_token()};
    const Token token3{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(1, std::get<int>(token1.value));
    EXPECT_EQ(TokenType::INTEGER, token2.type);
    EXPECT_EQ(2, std::get<int>(token2.value));
    EXPECT_EQ(TokenType::INTEGER, retrieved1.type);
    EXPECT_EQ(1, std::get<int>(retrieved1.value));
    EXPECT_EQ(TokenType::INTEGER, retrieved2.type);
    EXPECT_EQ(2, std::get<int>(retrieved2.value));
    EXPECT_EQ(TokenType::INTEGER, token3.type);
    EXPECT_EQ(3, std::get<int>(token3.value));
}

TEST(TestLexer, putTokenWithPeek)
{
    Lexer lexer{"42 3.14"};

    const Token peeked{lexer.peek_token()};
    const Token token1{lexer.get_token()};
    lexer.put_token(token1);
    const Token peeked_again{lexer.peek_token()};
    const Token retrieved{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, peeked.type);
    EXPECT_EQ(42, std::get<int>(peeked.value));
    EXPECT_EQ(TokenType::INTEGER, token1.type);
    EXPECT_EQ(TokenType::INTEGER, peeked_again.type);
    EXPECT_EQ(42, std::get<int>(peeked_again.value));
    EXPECT_EQ(TokenType::INTEGER, retrieved.type);
    EXPECT_EQ(42, std::get<int>(retrieved.value));
}

TEST(TestLexer, putTokenDifferentTypes)
{
    Lexer lexer{"xyz"};

    const Token int_token(42, {}, 1);
    const Token string_token(TokenType::STRING, std::string("hello"), {}, 1);
    const Token op_token(TokenType::PLUS, {}, 1);
    lexer.put_token(int_token);
    lexer.put_token(string_token);
    lexer.put_token(op_token);
    const Token retrieved1{lexer.get_token()};
    const Token retrieved2{lexer.get_token()};
    const Token retrieved3{lexer.get_token()};
    const Token original{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, retrieved1.type);
    EXPECT_EQ(42, std::get<int>(retrieved1.value));
    EXPECT_EQ(TokenType::STRING, retrieved2.type);
    EXPECT_EQ("hello", std::get<std::string>(retrieved2.value));
    EXPECT_EQ(TokenType::PLUS, retrieved3.type);
    EXPECT_EQ(TokenType::IDENTIFIER, original.type);
    EXPECT_EQ("xyz", std::get<std::string>(original.value));
}

TEST(TestLexer, putTokenAfterEndOfInput)
{
    Lexer lexer{"42"};

    const Token token{lexer.get_token()};
    const Token end_token{lexer.get_token()};
    lexer.put_token(token);
    const Token retrieved{lexer.get_token()};
    const Token end_again{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, token.type);
    EXPECT_EQ(TokenType::END_OF_INPUT, end_token.type);
    EXPECT_EQ(TokenType::INTEGER, retrieved.type);
    EXPECT_EQ(42, std::get<int>(retrieved.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end_again.type);
}

TEST(TestLexer, putTokenPreservesTokenDetails)
{
    Lexer lexer{"  maxit  "};

    const Token token{lexer.get_token()};
    const SourceLocation orig_position{token.location};
    const size_t orig_length{token.length};
    lexer.put_token(token);
    const Token retrieved{lexer.get_token()};

    EXPECT_EQ(TokenType::MAX_ITER, token.type);
    EXPECT_EQ("maxit", std::get<std::string>(token.value));
    EXPECT_EQ(TokenType::MAX_ITER, retrieved.type);
    EXPECT_EQ("maxit", std::get<std::string>(retrieved.value));
    EXPECT_EQ(orig_position, retrieved.location);
    EXPECT_EQ(orig_length, retrieved.length);
}

TEST(TestLexer, putTokenMultiplePeeks)
{
    Lexer lexer{"1 2 3"};

    const Token token1{lexer.get_token()};
    lexer.put_token(token1);
    const Token peek1{lexer.peek_token()};
    const Token peek2{lexer.peek_token()};
    const Token peek3{lexer.peek_token()};
    const Token consumed{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, peek1.type);
    EXPECT_EQ(1, std::get<int>(peek1.value));
    EXPECT_EQ(TokenType::INTEGER, peek2.type);
    EXPECT_EQ(1, std::get<int>(peek2.value));
    EXPECT_EQ(TokenType::INTEGER, peek3.type);
    EXPECT_EQ(1, std::get<int>(peek3.value));
    EXPECT_EQ(TokenType::INTEGER, consumed.type);
    EXPECT_EQ(1, std::get<int>(consumed.value));
}

TEST(TestLexer, putTokenComplexSequence)
{
    Lexer lexer{"x + y"};

    const Token id1{lexer.get_token()};
    const Token op{lexer.get_token()};
    const Token id2{lexer.get_token()};
    lexer.put_token(id1);
    lexer.put_token(op);
    lexer.put_token(id2);
    const Token r1{lexer.get_token()};
    const Token r2{lexer.get_token()};
    const Token r3{lexer.get_token()};
    const Token end{lexer.get_token()};

    EXPECT_EQ(TokenType::IDENTIFIER, id1.type);
    EXPECT_EQ("x", std::get<std::string>(id1.value));
    EXPECT_EQ(TokenType::PLUS, op.type);
    EXPECT_EQ(TokenType::IDENTIFIER, id2.type);
    EXPECT_EQ("y", std::get<std::string>(id2.value));
    EXPECT_EQ(TokenType::IDENTIFIER, r1.type);
    EXPECT_EQ("x", std::get<std::string>(r1.value));
    EXPECT_EQ(TokenType::PLUS, r2.type);
    EXPECT_EQ(TokenType::IDENTIFIER, r3.type);
    EXPECT_EQ("y", std::get<std::string>(r3.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end.type);
}

TEST(TestLexer, putTokenWithInvalidToken)
{
    Lexer lexer{"42"};

    const Token invalid_token(TokenType::INVALID, {}, 1);
    lexer.put_token(invalid_token);
    const Token retrieved{lexer.get_token()};
    const Token original{lexer.get_token()};

    EXPECT_EQ(TokenType::INVALID, retrieved.type);
    EXPECT_EQ(TokenType::INTEGER, original.type);
    EXPECT_EQ(42, std::get<int>(original.value));
}

TEST(TestLexer, putTokenEmptyLexer)
{
    Lexer lexer{""};

    const Token token(99, {}, 1);
    lexer.put_token(token);
    const Token retrieved{lexer.get_token()};
    const Token end{lexer.get_token()};

    EXPECT_EQ(TokenType::INTEGER, retrieved.type);
    EXPECT_EQ(99, std::get<int>(retrieved.value));
    EXPECT_EQ(TokenType::END_OF_INPUT, end.type);
}

} // namespace formula::test
