// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Parameter.h>

#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <vector>

using namespace formula::parameter;
using namespace testing;

namespace formula::parameter
{

static std::ostream &operator<<(std::ostream &os, TokenType type)
{
    return os << to_string(type);
}

} // namespace formula::parameter

namespace formula::test
{

namespace
{

std::vector<Token> tokens(std::string_view text)
{
    Lexer lexer{text};
    std::vector<Token> result;
    for (;;)
    {
        result.push_back(lexer.get_token());
        if (result.back().type == TokenType::END_OF_INPUT)
        {
            break;
        }
    }
    return result;
}

void expect_token(const Token &token, TokenType type, std::string_view value)
{
    EXPECT_EQ(type, token.type);
    EXPECT_EQ(value, token.value);
}

} // namespace

TEST(TestParameterLexer, lexesPlainParameterEntry)
{
    const std::vector<Token> result{tokens("Mandelbrot {\n"
                                           "  fractal:\n"
                                           "  title=\"Name; kept\"\n"
                                           "  magn=1.5\n"
                                           "  center=-0.5/0.25\n"
                                           "}")};

    ASSERT_EQ(14U, result.size());
    expect_token(result[0], TokenType::ENTRY_NAME, "Mandelbrot");
    EXPECT_EQ(TokenType::OPEN_BRACE, result[1].type);
    expect_token(result[2], TokenType::SECTION_LABEL, "fractal");
    expect_token(result[3], TokenType::KEY, "title");
    EXPECT_EQ(TokenType::ASSIGN, result[4].type);
    expect_token(result[5], TokenType::QUOTED_STRING, "Name; kept");
    expect_token(result[6], TokenType::KEY, "magn");
    EXPECT_EQ(TokenType::ASSIGN, result[7].type);
    expect_token(result[8], TokenType::RAW_ATOM, "1.5");
    expect_token(result[9], TokenType::KEY, "center");
    EXPECT_EQ(TokenType::ASSIGN, result[10].type);
    expect_token(result[11], TokenType::RAW_ATOM, "-0.5/0.25");
    EXPECT_EQ(TokenType::CLOSE_BRACE, result[12].type);
    EXPECT_EQ(TokenType::END_OF_INPUT, result[13].type);
}

TEST(TestParameterLexer, braceEndsEntryWithoutNewline)
{
    const std::vector<Token> result{tokens("One { fractal: }Next Entry { fractal: }")};

    ASSERT_EQ(9U, result.size());
    expect_token(result[0], TokenType::ENTRY_NAME, "One");
    EXPECT_EQ(TokenType::OPEN_BRACE, result[1].type);
    expect_token(result[2], TokenType::SECTION_LABEL, "fractal");
    EXPECT_EQ(TokenType::CLOSE_BRACE, result[3].type);
    expect_token(result[4], TokenType::ENTRY_NAME, "Next Entry");
    EXPECT_EQ(TokenType::OPEN_BRACE, result[5].type);
    expect_token(result[6], TokenType::SECTION_LABEL, "fractal");
    EXPECT_EQ(TokenType::CLOSE_BRACE, result[7].type);
    EXPECT_EQ(TokenType::END_OF_INPUT, result[8].type);
}

TEST(TestParameterLexer, emitsCommentsOutsideStrings)
{
    Lexer lexer{"A { ; comment\nfractal: credits=\"Name;date\" }"};

    const Token entry{lexer.get_token()};
    const Token open{lexer.get_token()};
    const Token comment{lexer.get_token()};
    const Token section{lexer.get_token()};
    const Token key{lexer.get_token()};
    const Token assign{lexer.get_token()};
    const Token value{lexer.get_token()};

    expect_token(entry, TokenType::ENTRY_NAME, "A");
    EXPECT_EQ(TokenType::OPEN_BRACE, open.type);
    expect_token(comment, TokenType::COMMENT, " comment");
    expect_token(section, TokenType::SECTION_LABEL, "fractal");
    expect_token(key, TokenType::KEY, "credits");
    EXPECT_EQ(TokenType::ASSIGN, assign.type);
    expect_token(value, TokenType::QUOTED_STRING, "Name;date");
}

TEST(TestParameterLexer, continuesQuotedStrings)
{
    Lexer lexer{"A { fractal: title=\"Alpha \\\r\n"
                "  Beta\" }"};

    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    const Token value{lexer.get_token()};

    expect_token(value, TokenType::QUOTED_STRING, "Alpha Beta");
}

TEST(TestParameterLexer, emitsCompressedPayloadAsOpaqueText)
{
    Lexer lexer{"Big {\n"
                "; leading comment\n"
                "  ::AAAA\n"
                "BBBB\n"
                "}Next { }"};

    expect_token(lexer.get_token(), TokenType::ENTRY_NAME, "Big");
    EXPECT_EQ(TokenType::OPEN_BRACE, lexer.get_token().type);
    expect_token(lexer.get_token(), TokenType::COMMENT, " leading comment");
    expect_token(lexer.get_token(), TokenType::COMPRESSED_PAYLOAD, "::AAAA\nBBBB\n");
    EXPECT_EQ(TokenType::CLOSE_BRACE, lexer.get_token().type);
    expect_token(lexer.get_token(), TokenType::ENTRY_NAME, "Next");
}

TEST(TestParameterLexer, permitsColonInAssignedAtom)
{
    Lexer lexer{"A { fractal: ref=file.ufm:Entry }"};

    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    const Token value{lexer.get_token()};

    expect_token(value, TokenType::RAW_ATOM, "file.ufm:Entry");
}

TEST(TestParameterLexer, sourceFilenameIsRetained)
{
    Options options;
    options.source_filename = "example.upr";
    Lexer lexer{"A { fractal: }", options};

    const Token token{lexer.get_token()};

    EXPECT_EQ("example.upr", token.location.filename);
}

TEST(TestParameterLexer, reportsUnterminatedString)
{
    Lexer lexer{"A { fractal: title=\"no close\n}"};

    while (lexer.get_token().type != TokenType::END_OF_INPUT)
    {
    }

    ASSERT_FALSE(lexer.get_errors().empty());
    EXPECT_EQ(LexerErrorCode::UNTERMINATED_QUOTED_STRING, lexer.get_errors().front().code);
}

} // namespace formula::test
