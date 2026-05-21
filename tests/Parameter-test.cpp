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

static std::ostream &operator<<(std::ostream &os, ParseErrorCode code)
{
    return os << to_string(code);
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

TEST(TestParameterLexer, lineContinuationsAreInvisible)
{
    Lexer lexer{"fractal:\\\r\n"
                "  title=Alpha\\\r\n"
                "  Beta"};

    expect_token(lexer.get_token(), TokenType::SECTION_LABEL, "fractal");
    expect_token(lexer.get_token(), TokenType::KEY, "title");
    EXPECT_EQ(TokenType::ASSIGN, lexer.get_token().type);
    expect_token(lexer.get_token(), TokenType::RAW_ATOM, "AlphaBeta");
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

TEST(TestParameterParser, parsesBasicNameValuePairs)
{
    Options options;
    options.dialect = Dialect::BASIC;

    const ParameterParseResult result{parse_parameter_body("title=\"Name\" magn=1.5 center=-0.5/0.25", options)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(3U, result.body.assignments.size());
    EXPECT_TRUE(result.body.sections.empty());
    EXPECT_EQ("title", result.body.assignments[0].key);
    EXPECT_EQ("Name", result.body.assignments[0].value.text);
    EXPECT_EQ(TokenType::QUOTED_STRING, result.body.assignments[0].value.token_type);
    EXPECT_EQ("magn", result.body.assignments[1].key);
    EXPECT_EQ("1.5", result.body.assignments[1].value.text);
    EXPECT_EQ("center", result.body.assignments[2].key);
    EXPECT_EQ("-0.5/0.25", result.body.assignments[2].value.text);
}

TEST(TestParameterParser, parsesExtendedSections)
{
    const ParameterParseResult result{parse_parameter_body("fractal:\n"
                                                           "title=\"Name\"\n"
                                                           "layer:\n"
                                                           "caption=\"Layer 1\"\n"
                                                           "formula:\n"
                                                           "filename=\"mmf.ufm\"\n"
                                                           "entry=\"Mandelbrot\"\n"
                                                           "p_power=2\n")};

    ASSERT_TRUE(result.diagnostics.empty());
    EXPECT_TRUE(result.body.assignments.empty());
    ASSERT_EQ(3U, result.body.sections.size());
    EXPECT_EQ("fractal", result.body.sections[0].name);
    ASSERT_EQ(1U, result.body.sections[0].assignments.size());
    EXPECT_EQ("title", result.body.sections[0].assignments[0].key);
    EXPECT_EQ("Name", result.body.sections[0].assignments[0].value.text);
    EXPECT_EQ("layer", result.body.sections[1].name);
    EXPECT_EQ("formula", result.body.sections[2].name);
    ASSERT_EQ(3U, result.body.sections[2].assignments.size());
    EXPECT_EQ("p_power", result.body.sections[2].assignments[2].key);
    EXPECT_EQ("2", result.body.sections[2].assignments[2].value.text);
}

TEST(TestParameterParser, preservesRepeatedAssignmentsInOrder)
{
    const ParameterParseResult result{parse_parameter_body("gradient:\n"
                                                           "index=0 color=4278190080\n"
                                                           "index=1 color=4294967295\n")};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(1U, result.body.sections.size());
    ASSERT_EQ(4U, result.body.sections[0].assignments.size());
    EXPECT_EQ("index", result.body.sections[0].assignments[0].key);
    EXPECT_EQ("0", result.body.sections[0].assignments[0].value.text);
    EXPECT_EQ("color", result.body.sections[0].assignments[1].key);
    EXPECT_EQ("4278190080", result.body.sections[0].assignments[1].value.text);
    EXPECT_EQ("index", result.body.sections[0].assignments[2].key);
    EXPECT_EQ("1", result.body.sections[0].assignments[2].value.text);
    EXPECT_EQ("color", result.body.sections[0].assignments[3].key);
    EXPECT_EQ("4294967295", result.body.sections[0].assignments[3].value.text);
}

TEST(TestParameterParser, recoversAtNextTopLevelEntry)
{
    std::istringstream input{"Bad { junk title=\"skipped\" }Good { fractal: title=\"ok\" }"};

    const ParameterFile file{parse_parameter_file(input)};

    ASSERT_EQ(2U, file.entries.size());
    ASSERT_FALSE(file.entries[0].diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_SECTION_LABEL, file.entries[0].diagnostics[0].code);
    ASSERT_EQ(1U, file.entries[1].body.sections.size());
    ASSERT_EQ(1U, file.entries[1].body.sections[0].assignments.size());
    EXPECT_EQ("title", file.entries[1].body.sections[0].assignments[0].key);
    EXPECT_EQ("ok", file.entries[1].body.sections[0].assignments[0].value.text);
}

TEST(TestParameterParser, entrySourceLocationSeedsBodyTokens)
{
    std::istringstream input{"One {\n"
                             "fractal: title=\"ok\"\n"
                             "}"};

    const ParameterFile file{parse_parameter_file(input, "example.upr")};

    ASSERT_EQ(1U, file.entries.size());
    ASSERT_EQ(1U, file.entries[0].body.sections.size());
    EXPECT_EQ("example.upr", file.entries[0].body.sections[0].source_range.begin.filename);
    EXPECT_EQ(2U, file.entries[0].body.sections[0].source_range.begin.line);
    EXPECT_EQ(1U, file.entries[0].body.sections[0].source_range.begin.column);
}

} // namespace formula::test
