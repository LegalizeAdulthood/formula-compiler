// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Gradient.h>

#include <cctype>
#include <stdexcept>

namespace gradient
{

namespace
{

enum class TokenType
{
    END_OF_FILE,
    IDENTIFIER,
    INTEGER,
    QUOTED_STRING,
    EQUALS,
    LEFT_BRACE,
    RIGHT_BRACE,
    COLON,
};

struct Token
{
    TokenType type{TokenType::END_OF_FILE};
    std::string value;
};

class Tokenizer
{
public:
    explicit Tokenizer(std::string_view text) : m_text(text), m_pos(0)
    {
    }

    Token next()
    {
        skip_whitespace_and_comments();

        if (m_pos >= m_text.size())
            return {TokenType::END_OF_FILE, {}};

        const char ch = m_text[m_pos];

        if (ch == '=')
        {
            ++m_pos;
            return {TokenType::EQUALS, "="};
        }
        if (ch == '{')
        {
            ++m_pos;
            return {TokenType::LEFT_BRACE, "{"};
        }
        if (ch == '}')
        {
            ++m_pos;
            return {TokenType::RIGHT_BRACE, "}"};
        }
        if (ch == '"')
        {
            return scan_quoted_string();
        }
        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)))
        {
            return scan_integer();
        }
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_')
        {
            return scan_identifier();
        }

        throw std::runtime_error(std::string("unexpected character: ") + ch);
    }

    Token peek()
    {
        const std::size_t saved = m_pos;
        Token t = next();
        m_pos = saved;
        return t;
    }

private:
    void skip_whitespace_and_comments()
    {
        while (m_pos < m_text.size())
        {
            const char ch = m_text[m_pos];
            if (std::isspace(static_cast<unsigned char>(ch)))
            {
                ++m_pos;
            }
            else if (ch == ';')
            {
                while (m_pos < m_text.size() && m_text[m_pos] != '\n')
                    ++m_pos;
            }
            else
            {
                break;
            }
        }
    }

    Token scan_quoted_string()
    {
        ++m_pos;
        std::string value;
        while (m_pos < m_text.size() && m_text[m_pos] != '"')
        {
            value += m_text[m_pos];
            ++m_pos;
        }
        if (m_pos >= m_text.size())
            throw std::runtime_error("unterminated quoted string");
        ++m_pos;
        return {TokenType::QUOTED_STRING, std::move(value)};
    }

    Token scan_integer()
    {
        std::string value;
        if (m_text[m_pos] == '-')
        {
            value += '-';
            ++m_pos;
        }
        while (m_pos < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_pos])))
        {
            value += m_text[m_pos];
            ++m_pos;
        }
        return {TokenType::INTEGER, std::move(value)};
    }

    Token scan_identifier()
    {
        std::string value;
        while (m_pos < m_text.size())
        {
            const char ch = m_text[m_pos];
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-')
            {
                value += ch;
                ++m_pos;
            }
            else
            {
                break;
            }
        }
        if (m_pos < m_text.size() && m_text[m_pos] == ':')
        {
            ++m_pos;
            return {TokenType::COLON, std::move(value)};
        }
        return {TokenType::IDENTIFIER, std::move(value)};
    }

    std::string_view m_text;
    std::size_t m_pos;
};

} // namespace

GradientFile parse_gradient(std::string_view /*text*/)
{
    return {};
}

} // namespace gradient
