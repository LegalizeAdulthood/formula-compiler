// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Parameter.h>

#include <algorithm>
#include <utility>

namespace formula::parameter
{

#define TOKEN_TYPE_CASE(name_) \
    case TokenType::name_:     \
        return #name_

std::string to_string(TokenType type)
{
    switch (type)
    {
        TOKEN_TYPE_CASE(NONE);
        TOKEN_TYPE_CASE(END_OF_INPUT);
        TOKEN_TYPE_CASE(SECTION_LABEL);
        TOKEN_TYPE_CASE(KEY);
        TOKEN_TYPE_CASE(ASSIGN);
        TOKEN_TYPE_CASE(RAW_ATOM);
        TOKEN_TYPE_CASE(QUOTED_STRING);
        TOKEN_TYPE_CASE(COMMENT);
        TOKEN_TYPE_CASE(COMPRESSED_PAYLOAD);
    }
    return "TokenType(" + std::to_string(static_cast<int>(type)) + ")";
}

#define LEXER_ERROR_CASE(name_) \
    case LexerErrorCode::name_: \
        return #name_

std::string to_string(LexerErrorCode code)
{
    switch (code)
    {
        LEXER_ERROR_CASE(NONE);
        LEXER_ERROR_CASE(UNTERMINATED_QUOTED_STRING);
    }
    return "LexerErrorCode(" + std::to_string(static_cast<int>(code)) + ")";
}

Lexer::Lexer(std::string_view input) :
    Lexer(input, Options{})
{
}

Lexer::Lexer(std::string_view input, Options options) :
    m_options(std::move(options)),
    m_input(input)
{
    m_source_location.filename = m_options.source_filename;
}

Token Lexer::get_token()
{
    if (!m_peek_tokens.empty())
    {
        Token result{m_peek_tokens.front()};
        m_peek_tokens.pop_front();
        return result;
    }

    skip_whitespace();

    if (at_end())
    {
        return {TokenType::END_OF_INPUT, m_source_location, 0};
    }

    const char ch{current_char()};
    if (ch == ';')
    {
        return comment();
    }
    if (ch == ':' && peek_char() == ':')
    {
        return compressed_payload();
    }
    if (ch == '=')
    {
        const SourceLocation start{m_source_location};
        advance();
        m_after_assign = true;
        return {TokenType::ASSIGN, start, 1};
    }
    if (ch == '"')
    {
        return quoted_string();
    }

    return atom();
}

Token Lexer::peek_token()
{
    if (m_peek_tokens.empty())
    {
        m_peek_tokens.push_back(get_token());
    }
    return m_peek_tokens.front();
}

void Lexer::put_token(Token token)
{
    m_peek_tokens.push_back(std::move(token));
}

void Lexer::skip_whitespace()
{
    while (!at_end() && is_space(current_char()))
    {
        advance();
    }
}

Token Lexer::comment()
{
    const size_t start{m_position};
    const SourceLocation start_loc{m_source_location};
    advance();
    const size_t value_start{m_position};
    while (!at_end() && current_char() != '\n' && current_char() != '\r')
    {
        advance();
    }
    return {TokenType::COMMENT, std::string{m_input.substr(value_start, m_position - value_start)}, start_loc,
        m_position - start};
}

Token Lexer::atom()
{
    const size_t start{m_position};
    const SourceLocation start_loc{m_source_location};
    const bool allow_colon{m_after_assign};
    const size_t end{scan_atom_end(allow_colon)};

    if (!allow_colon && end > start && end < m_input.length() && m_input[end] == ':')
    {
        while (m_position <= end)
        {
            advance();
        }
        m_after_assign = false;
        return {
            TokenType::SECTION_LABEL, std::string{m_input.substr(start, end - start)}, start_loc, m_position - start};
    }

    if (end == start)
    {
        advance();
        m_after_assign = false;
        return {TokenType::RAW_ATOM, std::string{m_input.substr(start, 1)}, start_loc, 1};
    }

    while (m_position < end)
    {
        advance();
    }

    const TokenType type{!m_after_assign && atom_is_key(end) ? TokenType::KEY : TokenType::RAW_ATOM};
    m_after_assign = false;
    return {type, std::string{m_input.substr(start, end - start)}, start_loc, end - start};
}

Token Lexer::quoted_string()
{
    const size_t start{m_position};
    const SourceLocation start_loc{m_source_location};
    std::string value;

    advance();
    while (!at_end())
    {
        const char ch{current_char()};
        if (ch == '"')
        {
            advance();
            m_after_assign = false;
            return {TokenType::QUOTED_STRING, value, start_loc, m_position - start};
        }
        if (ch == '\\' && (peek_char() == '\n' || peek_char() == '\r'))
        {
            advance();
            if (current_char() == '\r')
            {
                advance();
            }
            if (current_char() == '\n')
            {
                advance();
            }
            while (!at_end() && (current_char() == ' ' || current_char() == '\t'))
            {
                advance();
            }
            continue;
        }
        if (ch == '\\' && (peek_char() == '"' || peek_char() == '\\'))
        {
            advance();
            value.append(1, current_char());
            advance();
            continue;
        }
        if (ch == '\n' || ch == '\r')
        {
            error(LexerErrorCode::UNTERMINATED_QUOTED_STRING, start_loc);
            m_after_assign = false;
            return {TokenType::QUOTED_STRING, value, start_loc, m_position - start};
        }
        value.append(1, ch);
        advance();
    }

    error(LexerErrorCode::UNTERMINATED_QUOTED_STRING, start_loc);
    m_after_assign = false;
    return {TokenType::QUOTED_STRING, value, start_loc, m_position - start};
}

Token Lexer::compressed_payload()
{
    const size_t start{m_position};
    const SourceLocation start_loc{m_source_location};
    while (!at_end() && current_char() != '}')
    {
        advance();
    }
    m_after_assign = false;
    return {TokenType::COMPRESSED_PAYLOAD, std::string{m_input.substr(start, m_position - start)}, start_loc,
        m_position - start};
}

bool Lexer::atom_is_key(size_t end) const
{
    const size_t next{skip_space_from(end)};
    return next < m_input.length() && m_input[next] == '=';
}

size_t Lexer::scan_atom_end(bool allow_colon) const
{
    size_t pos{m_position};
    while (pos < m_input.length() && !is_atom_delimiter(m_input[pos], allow_colon))
    {
        ++pos;
    }
    return pos;
}

size_t Lexer::skip_space_from(size_t pos) const
{
    while (pos < m_input.length() && is_space(m_input[pos]))
    {
        ++pos;
    }
    return pos;
}

bool Lexer::is_space(char ch) const
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

bool Lexer::is_atom_delimiter(char ch, bool allow_colon) const
{
    switch (ch)
    {
    case '{':
    case '}':
    case '=':
    case '"':
    case ';':
        return true;
    case ':':
        return !allow_colon;
    default:
        return is_space(ch);
    }
}

char Lexer::current_char() const
{
    if (m_position < m_input.length())
    {
        return m_input[m_position];
    }
    return '\0';
}

char Lexer::peek_char(size_t offset) const
{
    const size_t pos{m_position + offset};
    if (pos < m_input.length())
    {
        return m_input[pos];
    }
    return '\0';
}

void Lexer::advance()
{
    if (at_end())
    {
        return;
    }

    const char ch{m_input[m_position]};
    if (ch == '\r')
    {
        ++m_position;
        if (!at_end() && current_char() == '\n')
        {
            ++m_position;
        }
        ++m_source_location.line;
        m_source_location.column = 1;
        return;
    }
    if (ch == '\n')
    {
        ++m_position;
        ++m_source_location.line;
        m_source_location.column = 1;
        return;
    }

    ++m_position;
    ++m_source_location.column;
}

LexerPtr lex(std::string_view input)
{
    return std::make_shared<Lexer>(input);
}

std::vector<FileEntry> load_parameter_entries(std::istream &in, std::string filename)
{
    return load_file_entries(in, std::move(filename));
}

} // namespace formula::parameter
