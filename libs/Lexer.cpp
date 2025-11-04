// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Lexer.h>

#include <cctype>
#include <cstdlib>

namespace formula
{

Lexer::Lexer(std::string_view input) :
    m_input(input),
    m_position(0),
    m_has_peek(false)
{
}

Token Lexer::next_token()
{
    if (m_has_peek)
    {
        m_has_peek = false;
        return m_peek_token;
    }

    skip_whitespace();

    if (at_end())
    {
        return Token(TokenType::END_OF_INPUT, m_position, 0);
    }

    if (is_number_start())
    {
        return lex_number();
    }

    // Unknown character
    Token token(TokenType::INVALID, m_position, 1);
    advance();
    return token;
}

Token Lexer::peek_token()
{
    if (!m_has_peek)
    {
        m_peek_token = next_token();
        m_has_peek = true;
    }
    return m_peek_token;
}

void Lexer::skip_whitespace()
{
    while (m_position < m_input.length() && std::isspace(static_cast<unsigned char>(m_input[m_position])))
    {
        ++m_position;
    }
}

bool Lexer::is_number_start() const
{
    const auto is_digit = [](char c)
    {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    };
    const char ch = current_char();

    // Check for digit
    if (is_digit(ch))
    {
        return true;
    }

    // Check for decimal point followed by digit
    if (ch == '.' && m_position + 1 < m_input.length() && is_digit(m_input[m_position + 1]))
    {
        return true;
    }

    // Check for sign followed by digit or decimal
    if ((ch == '+' || ch == '-') && m_position + 1 < m_input.length())
    {
        char next_ch = m_input[m_position + 1];

        // Sign followed by digit
        if (is_digit(next_ch))
        {
            return true;
        }

        // Sign followed by decimal point and digit
        if (next_ch == '.' && m_position + 2 < m_input.length() && is_digit(m_input[m_position + 2]))
        {
            return true;
        }
    }

    return false;
}

Token Lexer::lex_number()
{
    size_t start = m_position;
    std::string number_str;

    // Handle optional leading sign
    if (current_char() == '+' || current_char() == '-')
    {
        number_str += current_char();
        advance();
    }

    // Integer part
    while (m_position < m_input.length() && std::isdigit(static_cast<unsigned char>(current_char())))
    {
        number_str += current_char();
        advance();
    }

    // Decimal part
    if (m_position < m_input.length() && current_char() == '.')
    {
        number_str += current_char();
        advance();

        while (m_position < m_input.length() && std::isdigit(static_cast<unsigned char>(current_char())))
        {
            number_str += current_char();
            advance();
        }
    }

    // Exponent part
    if (m_position < m_input.length() && (current_char() == 'e' || current_char() == 'E'))
    {
        number_str += current_char();
        advance();

        if (m_position < m_input.length() && (current_char() == '+' || current_char() == '-'))
        {
            number_str += current_char();
            advance();
        }

        while (m_position < m_input.length() && std::isdigit(static_cast<unsigned char>(current_char())))
        {
            number_str += current_char();
            advance();
        }
    }

    // Convert to double
    char *end;
    double value = std::strtod(number_str.c_str(), &end);

    size_t length = m_position - start;
    return Token(value, start, length);
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
    size_t peek_pos = m_position + offset;
    if (peek_pos < m_input.length())
    {
        return m_input[peek_pos];
    }
    return '\0';
}

void Lexer::advance(size_t count)
{
    m_position += count;
}

} // namespace formula
