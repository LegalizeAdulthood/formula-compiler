// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Parameter.h>

#include <gtest/gtest.h>

#include <ostream>
#include <sstream>
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
    const std::vector<Token> result{tokens("fractal:\n"
                                           "title=\"Name; kept\"\n"
                                           "magn=1.5\n"
                                           "center=-0.5/0.25\n")};

    ASSERT_EQ(11U, result.size());
    expect_token(result[0], TokenType::SECTION_LABEL, "fractal");
    expect_token(result[1], TokenType::KEY, "title");
    EXPECT_EQ(TokenType::ASSIGN, result[2].type);
    expect_token(result[3], TokenType::QUOTED_STRING, "Name; kept");
    expect_token(result[4], TokenType::KEY, "magn");
    EXPECT_EQ(TokenType::ASSIGN, result[5].type);
    expect_token(result[6], TokenType::RAW_ATOM, "1.5");
    expect_token(result[7], TokenType::KEY, "center");
    EXPECT_EQ(TokenType::ASSIGN, result[8].type);
    expect_token(result[9], TokenType::RAW_ATOM, "-0.5/0.25");
    EXPECT_EQ(TokenType::END_OF_INPUT, result[10].type);
}

TEST(TestParameterLexer, parameterEntriesUseSharedFileScanner)
{
    std::istringstream input{"One { fractal: }Next Entry { layer: }"};

    const std::vector<FileEntry> entries{load_parameter_entries(input)};

    ASSERT_EQ(2U, entries.size());
    EXPECT_EQ("One", entries[0].name);
    EXPECT_EQ(" fractal: ", entries[0].body);
    EXPECT_EQ("Next Entry", entries[1].name);
    EXPECT_EQ(" layer: ", entries[1].body);
}

TEST(TestParameterLexer, emitsCommentsOutsideStrings)
{
    Lexer lexer{"; comment\nfractal: credits=\"Name;date\""};

    const Token comment{lexer.get_token()};
    const Token section{lexer.get_token()};
    const Token key{lexer.get_token()};
    const Token assign{lexer.get_token()};
    const Token value{lexer.get_token()};

    expect_token(comment, TokenType::COMMENT, " comment");
    expect_token(section, TokenType::SECTION_LABEL, "fractal");
    expect_token(key, TokenType::KEY, "credits");
    EXPECT_EQ(TokenType::ASSIGN, assign.type);
    expect_token(value, TokenType::QUOTED_STRING, "Name;date");
}

TEST(TestParameterLexer, continuesQuotedStrings)
{
    Lexer lexer{"fractal: title=\"Alpha \\\r\n"
                "  Beta\""};

    lexer.get_token();
    lexer.get_token();
    lexer.get_token();
    const Token value{lexer.get_token()};

    expect_token(value, TokenType::QUOTED_STRING, "Alpha Beta");
}

TEST(TestParameterLexer, emitsCompressedPayloadAsOpaqueText)
{
    Lexer lexer{"; leading comment\n"
                "  ::AAAA\n"
                "BBBB\n"};

    expect_token(lexer.get_token(), TokenType::COMMENT, " leading comment");
    expect_token(lexer.get_token(), TokenType::COMPRESSED_PAYLOAD, "::AAAA\nBBBB\n");
    EXPECT_EQ(TokenType::END_OF_INPUT, lexer.get_token().type);
}

TEST(TestParameterLexer, permitsColonInAssignedAtom)
{
    Lexer lexer{"fractal: ref=file.ufm:Entry"};

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
    Lexer lexer{"fractal:", options};

    const Token token{lexer.get_token()};

    EXPECT_EQ("example.upr", token.location.filename);
}

TEST(TestParameterLexer, reportsUnterminatedString)
{
    Lexer lexer{"fractal: title=\"no close\n"};

    while (lexer.get_token().type != TokenType::END_OF_INPUT)
    {
    }

    ASSERT_FALSE(lexer.get_errors().empty());
    EXPECT_EQ(LexerErrorCode::UNTERMINATED_QUOTED_STRING, lexer.get_errors().front().code);
}

} // namespace formula::test
