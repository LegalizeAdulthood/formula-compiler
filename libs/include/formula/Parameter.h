// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/Dialect.h>
#include <formula/FileEntry.h>
#include <formula/SourceLocation.h>

#include <deque>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace formula::parameter
{

enum class LexerErrorCode
{
    NONE = 0,
    UNTERMINATED_QUOTED_STRING,
};

enum class TokenType
{
    NONE = 0,
    END_OF_INPUT,
    SECTION_LABEL,
    KEY,
    ASSIGN,
    RAW_ATOM,
    QUOTED_STRING,
    COMMENT,
    COMPRESSED_PAYLOAD,
};

std::string to_string(TokenType type);
std::string to_string(LexerErrorCode code);

struct LexicalDiagnostic
{
    LexerErrorCode code{};
    SourceLocation location;
};

struct Token
{
    Token() = default;
    Token(TokenType t, SourceLocation loc, size_t len) :
        type(t),
        location(std::move(loc)),
        length(len)
    {
    }
    Token(TokenType t, std::string text, SourceLocation loc, size_t len) :
        type(t),
        value(std::move(text)),
        location(std::move(loc)),
        length(len)
    {
    }

    TokenType type{TokenType::END_OF_INPUT};
    std::string value;
    SourceLocation location;
    size_t length{};
};

struct Options
{
    Dialect dialect{Dialect::EXTENDED};
    std::string source_filename;
    SourceLocation source_location;
};

enum class ParseErrorCode
{
    NONE = 0,
    EXPECTED_SECTION_LABEL,
    EXPECTED_KEY,
    EXPECTED_ASSIGN,
    EXPECTED_VALUE,
};

std::string to_string(ParseErrorCode code);

struct ParseDiagnostic
{
    ParseErrorCode code{};
    SourceLocation location;
};

struct ParameterValue
{
    TokenType token_type{};
    std::string text;
    SourceRange source_range;
};

struct ParameterAssignment
{
    std::string key;
    ParameterValue value;
    SourceRange source_range;
};

struct ParameterSection
{
    std::string name;
    std::vector<ParameterAssignment> assignments;
    SourceRange source_range;
};

struct ParameterBody
{
    std::vector<ParameterAssignment> assignments;
    std::vector<ParameterSection> sections;
};

struct ParameterParseResult
{
    ParameterBody body;
    std::vector<LexicalDiagnostic> lexical_diagnostics;
    std::vector<ParseDiagnostic> diagnostics;
};

struct ParameterEntry
{
    FileEntry file_entry;
    ParameterBody body;
    std::vector<LexicalDiagnostic> lexical_diagnostics;
    std::vector<ParseDiagnostic> diagnostics;
};

struct ParameterFile
{
    std::vector<ParameterEntry> entries;
    std::vector<LexicalDiagnostic> lexical_diagnostics;
    std::vector<ParseDiagnostic> diagnostics;
};

class Lexer
{
public:
    explicit Lexer(std::string_view input);
    Lexer(std::string_view input, Options options);

    Token get_token();
    Token peek_token();
    void put_token(Token token);

    size_t position() const
    {
        return m_position;
    }
    SourceLocation source_location() const
    {
        return m_source_location;
    }
    bool at_end() const
    {
        return m_position >= m_input.length();
    }
    const std::vector<LexicalDiagnostic> &get_errors() const
    {
        return m_errors;
    }

private:
    void error(LexerErrorCode code, SourceLocation loc)
    {
        m_errors.push_back(LexicalDiagnostic{code, std::move(loc)});
    }

    void skip_whitespace();
    Token comment();
    Token atom();
    Token quoted_string();
    Token compressed_payload();
    bool atom_is_key(size_t end) const;
    size_t scan_atom_end(bool allow_colon) const;
    size_t skip_space_from(size_t pos) const;
    bool is_space(char ch) const;
    bool is_atom_delimiter(char ch, bool allow_colon) const;
    char current_char() const;
    char peek_char(size_t offset = 1) const;
    void advance();

    Options m_options;
    std::string m_input;
    size_t m_position{};
    SourceLocation m_source_location;
    bool m_after_assign{};
    std::deque<Token> m_peek_tokens;
    std::vector<LexicalDiagnostic> m_errors;
};

using LexerPtr = std::shared_ptr<Lexer>;

LexerPtr lex(std::string_view input);
std::vector<FileEntry> load_parameter_entries(std::istream &in, std::string filename = {});
ParameterParseResult parse_parameter_body(std::string_view input, Options options = {});
ParameterEntry parse_parameter_entry(FileEntry file_entry, Options options = {});
ParameterFile parse_parameter_file(std::istream &in, Options options = {});
ParameterFile parse_parameter_file(std::istream &in, std::string filename);

} // namespace formula::parameter
