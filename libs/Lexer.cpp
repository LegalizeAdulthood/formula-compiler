// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Lexer.h>

#include <algorithm>
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
        return {TokenType::END_OF_INPUT, m_position, 0};
    }

    char ch = current_char();

    // Check for end-of-line (newline) as separator
    if (ch == '\n')
    {
        size_t start = m_position;
        advance();
        return {TokenType::TERMINATOR, start, 1};
    }

    // Also handle standalone \r followed by \n (handled above) or as separator on its own
    // This is needed for the test case where \r\n is treated as a single separator
    // The \r is already consumed by skip_whitespace, so this won't be reached
    // unless there's a standalone \r without \n following

    // Check if this is the start of a number
    if (is_number_start())
    {
        return lex_number();
    }

    // Check if this is the start of an identifier
    if (is_identifier_start())
    {
        return lex_identifier();
    }

    // Check for operators
    size_t start = m_position;
    advance();

    switch (ch)
    {
    case '+':
        return {TokenType::PLUS, start, 1};
    case '-':
        return {TokenType::MINUS, start, 1};
    case '*':
        return {TokenType::MULTIPLY, start, 1};
    case '/':
        return {TokenType::DIVIDE, start, 1};
    case '^':
        return {TokenType::POWER, start, 1};
    case '=':
        // Check for == (EQUAL) vs = (ASSIGN)
        if (current_char() == '=')
        {
            advance();
            return {TokenType::EQUAL, start, 2};
        }
        return {TokenType::ASSIGN, start, 1};
    case '<':
        // Check for <= (LESS_EQUAL) vs < (LESS_THAN)
        if (current_char() == '=')
        {
            advance();
            return {TokenType::LESS_EQUAL, start, 2};
        }
        return {TokenType::LESS_THAN, start, 1};
    case '>':
        // Check for >= (GREATER_EQUAL) vs > (GREATER_THAN)
        if (current_char() == '=')
        {
            advance();
            return {TokenType::GREATER_EQUAL, start, 2};
        }
        return {TokenType::GREATER_THAN, start, 1};
    case '!':
        // Check for != (NOT_EQUAL)
        if (current_char() == '=')
        {
            advance();
            return {TokenType::NOT_EQUAL, start, 2};
        }
        // Single ! is not recognized
        return {TokenType::INVALID, start, 1};
    case '(':
        return {TokenType::LEFT_PAREN, start, 1};
    case ')':
        return {TokenType::RIGHT_PAREN, start, 1};
    case ':':
        return {TokenType::COLON, start, 1};
    case ',':
        return {TokenType::COMMA, start, 1};
    case '|':
        return {TokenType::MODULUS, start, 1};
    case '\\':
        // Backslash here means it wasn't part of a line continuation
        // (those are handled in skip_whitespace)
        return {TokenType::INVALID, start, 1};
    default:
        // Unknown character
        return {TokenType::INVALID, start, 1};
    }
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
    while (m_position < m_input.length())
    {
        char ch = m_input[m_position];
        // Skip only spaces and tabs, not newlines
        if (ch == ' ' || ch == '\t' || ch == '\r')
        {
            ++m_position;
        }
        else if (ch == '\\')
        {
            // Check for line continuation: backslash followed by newline
            if (m_position + 1 < m_input.length())
            {
                char next_ch = m_input[m_position + 1];
                if (next_ch == '\n')
                {
                    // Skip backslash and newline
                    m_position += 2;
                }
                else if (next_ch == '\r' && m_position + 2 < m_input.length() && m_input[m_position + 2] == '\n')
                {
                    // Skip backslash, CR, and LF
                    m_position += 3;
                }
                else
                {
                    // Backslash not followed by newline, stop skipping whitespace
                    break;
                }
            }
            else
            {
                // Backslash at end of input, stop skipping whitespace
                break;
            }
        }
        else if (ch == ';')
        {
            skip_comment();
        }
        else
        {
            break;
        }
    }
}

void Lexer::skip_comment()
{
    // Skip from semicolon to end of line, not including the newline
    while (m_position < m_input.length())
    {
        char ch = m_input[m_position];
        ++m_position;
        if (ch == '\n')
        {
            --m_position;
            break;
        }
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

    return false;
}

Token Lexer::lex_number()
{
    size_t start = m_position;
    std::string number_str;

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
    return {value, start, length};
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

bool Lexer::is_identifier_start() const
{
    char ch = current_char();
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexer::is_identifier_continue(char c) const
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

Token Lexer::lex_identifier()
{
    size_t start = m_position;
    std::string identifier;

    // Start with letter or underscore
    identifier += current_char();
    advance();

    // Continue with letters, digits, or underscores
    while (m_position < m_input.length() && is_identifier_continue(current_char()))
    {
        identifier += current_char();
        advance();
    }

    size_t length = m_position - start;

    // Check for keywords and builtin variables (case-sensitive)
    struct TextTokenType
    {
        std::string_view text;
        TokenType type;
    };

    static constexpr TextTokenType reserved[] = {
        {"if", TokenType::IF},                    // keywords
        {"elseif", TokenType::ELSE_IF},           //
        {"else", TokenType::ELSE},                //
        {"endif", TokenType::END_IF},             //
        {"global", TokenType::GLOBAL},            // Section names
        {"builtin", TokenType::BUILTIN},          //
        {"init", TokenType::INIT},                //
        {"loop", TokenType::LOOP},                //
        {"bailout", TokenType::BAILOUT},          //
        {"perturbinit", TokenType::PERTURB_INIT}, //
        {"perturbloop", TokenType::PERTURB_LOOP}, //
        {"default", TokenType::DEFAULT},          //
        {"switch", TokenType::SWITCH},            //
        {"p1", TokenType::P1},                    // Built-in variables
        {"p2", TokenType::P2},                    //
        {"p3", TokenType::P3},                    //
        {"p4", TokenType::P4},                    //
        {"p5", TokenType::P5},                    //
        {"pixel", TokenType::PIXEL},              //
        {"lastsqr", TokenType::LAST_SQR},         //
        {"rand", TokenType::RAND},                //
        {"pi", TokenType::PI},                    //
        {"e", TokenType::E},                      //
        {"maxit", TokenType::MAX_ITER},           //
        {"scrnmax", TokenType::SCREEN_MAX},       //
        {"scrnpix", TokenType::SCREEN_PIXEL},     //
        {"whitesq", TokenType::WHITE_SQUARE},     //
        {"ismand", TokenType::IS_MAND},           //
        {"center", TokenType::CENTER},            //
        {"magxmag", TokenType::MAG_X_MAG},        //
        {"rotskew", TokenType::ROT_SKEW},         //
        {"sinh", TokenType::SINH},                // Built-in functions
        {"cosh", TokenType::COSH},                //
        {"cosxx", TokenType::COSXX},              //
        {"sin", TokenType::SIN},                  //
        {"cos", TokenType::COS},                  //
        {"cotanh", TokenType::COTANH},            //
        {"cotan", TokenType::COTAN},              //
        {"tanh", TokenType::TANH},                //
        {"tan", TokenType::TAN},                  //
        {"sqrt", TokenType::SQRT},                //
        {"log", TokenType::LOG},                  //
        {"exp", TokenType::EXP},                  //
        {"abs", TokenType::ABS},                  //
        {"conj", TokenType::CONJ},                //
        {"real", TokenType::REAL},                //
        {"imag", TokenType::IMAG},                //
        {"flip", TokenType::FLIP},                //
        {"fn1", TokenType::FN1},                  //
        {"fn2", TokenType::FN2},                  //
        {"fn3", TokenType::FN3},                  //
        {"fn4", TokenType::FN4},                  //
        {"srand", TokenType::SRAND},              //
        {"asinh", TokenType::ASINH},              //
        {"acosh", TokenType::ACOSH},              //
        {"asin", TokenType::ASIN},                //
        {"acos", TokenType::ACOS},                //
        {"atanh", TokenType::ATANH},              //
        {"atan", TokenType::ATAN},                //
        {"cabs", TokenType::CABS},                //
        {"sqr", TokenType::SQR},                  //
        {"floor", TokenType::FLOOR},              //
        {"ceil", TokenType::CEIL},                //
        {"trunc", TokenType::TRUNC},              //
        {"round", TokenType::ROUND},              //
        {"ident", TokenType::IDENT},              //
        {"one", TokenType::ONE},                  //
        {"zero", TokenType::ZERO},                //
    };

    if (auto it = std::find_if(std::begin(reserved), std::end(reserved),
            [&identifier](const TextTokenType &kw) { return kw.text == identifier; });
        it != std::end(reserved))
    {
        return {it->type, start, length};
    }

    // Not a reserved word, return as identifier
    return {TokenType::IDENTIFIER, identifier, start, length};
}

} // namespace formula
