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

std::string_view to_string(TokenType value)
{
    switch (value)
    {
    case TokenType::NONE:
        return "NONE";
    case TokenType::END_OF_INPUT:
        return "END_OF_INPUT";
    case TokenType::INTEGER:
        return "INTEGER";
    case TokenType::NUMBER:
        return "NUMBER";
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::MULTIPLY:
        return "MULTIPLY";
    case TokenType::DIVIDE:
        return "DIVIDE";
    case TokenType::POWER:
        return "POWER";
    case TokenType::ASSIGN:
        return "ASSIGN";
    case TokenType::LESS_THAN:
        return "LESS_THAN";
    case TokenType::GREATER_THAN:
        return "GREATER_THAN";
    case TokenType::LESS_EQUAL:
        return "LESS_EQUAL";
    case TokenType::GREATER_EQUAL:
        return "GREATER_EQUAL";
    case TokenType::EQUAL:
        return "EQUAL";
    case TokenType::NOT_EQUAL:
        return "NOT_EQUAL";
    case TokenType::LOGICAL_AND:
        return "LOGICAL_AND";
    case TokenType::LOGICAL_OR:
        return "LOGICAL_OR";
    case TokenType::MODULUS:
        return "MODULUS";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::LEFT_PAREN:
        return "LEFT_PAREN";
    case TokenType::RIGHT_PAREN:
        return "RIGHT_PAREN";
    case TokenType::LEFT_BRACKET:
        return "LEFT_BRACKET";
    case TokenType::RIGHT_BRACKET:
        return "RIGHT_BRACKET";
    case TokenType::LEFT_BRACE:
        return "LEFT_BRACE";
    case TokenType::RIGHT_BRACE:
        return "RIGHT_BRACE";
    case TokenType::PERIOD:
        return "PERIOD";
    case TokenType::COLON:
        return "COLON";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::TERMINATOR:
        return "TERMINATOR";
    case TokenType::IF:
        return "IF";
    case TokenType::ELSE_IF:
        return "ELSE_IF";
    case TokenType::ELSE:
        return "ELSE";
    case TokenType::END_IF:
        return "END_IF";
    case TokenType::WHILE:
        return "WHILE";
    case TokenType::END_WHILE:
        return "END_WHILE";
    case TokenType::REPEAT:
        return "REPEAT";
    case TokenType::UNTIL:
        return "UNTIL";
    case TokenType::FUNC:
        return "FUNC";
    case TokenType::END_FUNC:
        return "END_FUNC";
    case TokenType::PARAM:
        return "PARAM";
    case TokenType::END_PARAM:
        return "END_PARAM";
    case TokenType::HEADING:
        return "HEADING";
    case TokenType::END_HEADING:
        return "END_HEADING";
    case TokenType::CTX_CONST:
        return "CTX_CONST";
    case TokenType::CTX_IMPORT:
        return "CTX_IMPORT";
    case TokenType::CTX_NEW:
        return "CTX_NEW";
    case TokenType::CTX_RETURN:
        return "CTX_RETURN";
    case TokenType::CTX_STATIC:
        return "CTX_STATIC";
    case TokenType::CTX_THIS:
        return "CTX_THIS";
    case TokenType::GLOBAL:
        return "GLOBAL";
    case TokenType::BUILTIN:
        return "BUILTIN";
    case TokenType::INIT:
        return "INIT";
    case TokenType::LOOP:
        return "LOOP";
    case TokenType::BAILOUT:
        return "BAILOUT";
    case TokenType::PERTURB_INIT:
        return "PERTURB_INIT";
    case TokenType::PERTURB_LOOP:
        return "PERTURB_LOOP";
    case TokenType::DEFAULT:
        return "DEFAULT";
    case TokenType::SWITCH:
        return "SWITCH";
    case TokenType::P1:
        return "P1";
    case TokenType::P2:
        return "P2";
    case TokenType::P3:
        return "P3";
    case TokenType::P4:
        return "P4";
    case TokenType::P5:
        return "P5";
    case TokenType::PIXEL:
        return "PIXEL";
    case TokenType::LAST_SQR:
        return "LAST_SQR";
    case TokenType::RAND:
        return "RAND";
    case TokenType::PI:
        return "PI";
    case TokenType::E:
        return "E";
    case TokenType::MAX_ITER:
        return "MAX_ITER";
    case TokenType::SCREEN_MAX:
        return "SCREEN_MAX";
    case TokenType::SCREEN_PIXEL:
        return "SCREEN_PIXEL";
    case TokenType::WHITE_SQUARE:
        return "WHITE_SQUARE";
    case TokenType::IS_MAND:
        return "IS_MAND";
    case TokenType::CENTER:
        return "CENTER";
    case TokenType::MAG_X_MAG:
        return "MAG_X_MAG";
    case TokenType::ROT_SKEW:
        return "ROT_SKEW";
    case TokenType::SINH:
        return "SINH";
    case TokenType::COSH:
        return "COSH";
    case TokenType::COSXX:
        return "COSXX";
    case TokenType::SIN:
        return "SIN";
    case TokenType::COS:
        return "COS";
    case TokenType::COTANH:
        return "COTANH";
    case TokenType::COTAN:
        return "COTAN";
    case TokenType::TANH:
        return "TANH";
    case TokenType::TAN:
        return "TAN";
    case TokenType::SQRT:
        return "SQRT";
    case TokenType::LOG:
        return "LOG";
    case TokenType::EXP:
        return "EXP";
    case TokenType::ABS:
        return "ABS";
    case TokenType::CONJ:
        return "CONJ";
    case TokenType::REAL:
        return "REAL";
    case TokenType::IMAG:
        return "IMAG";
    case TokenType::FLIP:
        return "FLIP";
    case TokenType::FN1:
        return "FN1";
    case TokenType::FN2:
        return "FN2";
    case TokenType::FN3:
        return "FN3";
    case TokenType::FN4:
        return "FN4";
    case TokenType::SRAND:
        return "SRAND";
    case TokenType::ASINH:
        return "ASINH";
    case TokenType::ACOSH:
        return "ACOSH";
    case TokenType::ASIN:
        return "ASIN";
    case TokenType::ACOS:
        return "ACOS";
    case TokenType::ATANH:
        return "ATANH";
    case TokenType::ATAN:
        return "ATAN";
    case TokenType::CABS:
        return "CABS";
    case TokenType::SQR:
        return "SQR";
    case TokenType::FLOOR:
        return "FLOOR";
    case TokenType::CEIL:
        return "CEIL";
    case TokenType::TRUNC:
        return "TRUNC";
    case TokenType::ROUND:
        return "ROUND";
    case TokenType::IDENT:
        return "IDENT";
    case TokenType::ONE:
        return "ONE";
    case TokenType::ZERO:
        return "ZERO";
    case TokenType::INVALID:
        return "INVALID";
    case TokenType::TRUE:
        return "TRUE";
    case TokenType::FALSE:
        return "FALSE";
    case TokenType::STRING:
        return "STRING";
    case TokenType::TYPE_BOOL:
        return "TYPE_BOOL";
    case TokenType::TYPE_INT:
        return "TYPE_INT";
    case TokenType::TYPE_FLOAT:
        return "TYPE_FLOAT";
    case TokenType::TYPE_COMPLEX:
        return "TYPE_COMPLEX";
    case TokenType::TYPE_COLOR:
        return "TYPE_COLOR";
    }

    return "?Unknown";
}

Lexer::Lexer(std::string_view input) :
    m_input(input),
    m_position(0)
{
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

    // Check for quoted strings
    if (ch == '"')
    {
        return lex_string();
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
    case '&':
        // Check for && (LOGICAL_AND)
        if (current_char() == '&')
        {
            advance();
            return {TokenType::LOGICAL_AND, start, 2};
        }
        // Single & is not recognized
        return {TokenType::INVALID, start, 1};
    case '|':
        // Check for || (LOGICAL_OR) vs | (MODULUS)
        if (current_char() == '|')
        {
            advance();
            return {TokenType::LOGICAL_OR, start, 2};
        }
        // Single | is MODULUS
        return {TokenType::MODULUS, start, 1};
    case '(':
        return {TokenType::LEFT_PAREN, start, 1};
    case ')':
        return {TokenType::RIGHT_PAREN, start, 1};
    case ':':
        return {TokenType::COLON, start, 1};
    case ',':
        return {TokenType::COMMA, start, 1};
    case '\\':
        // Backslash here means it wasn't part of a line continuation
        // (those are handled in skip_whitespace)
    default:
        // Unknown character
        return {TokenType::INVALID, start, 1};
    }
}

void Lexer::put_token(Token token)
{
    m_peek_tokens.push_back(token);
}

Token Lexer::peek_token()
{
    if (m_peek_tokens.empty())
    {
        m_peek_tokens.push_back(get_token());
    }
    return m_peek_tokens.front();
}

void Lexer::skip_whitespace()
{
    while (m_position < m_input.length())
    {
        char ch = m_input[m_position];
        // Skip only spaces and tabs, not newlines
        if (ch == ' ' || ch == '\t' || ch == '\r')
        {
            advance();
        }
        else if (ch == '\\')
        {
            // Check for line continuation: backslash followed by optional whitespace and newline
            size_t look_ahead = m_position + 1;
            size_t trailing_ws_pos{};
            const auto warn_trailing_ws = [&trailing_ws_pos, this]
            {
                if (trailing_ws_pos != 0)
                {
                    SourceLocation loc = position_to_location(trailing_ws_pos);
                    warning(LexerErrorCode::CONTINUATION_WITH_WHITESPACE, loc);
                    trailing_ws_pos = 0;
                }
            };

            // Skip any trailing whitespace after the backslash
            while (look_ahead < m_input.length())
            {
                char next_ch = m_input[look_ahead];
                if (next_ch == ' ' || next_ch == '\t')
                {
                    trailing_ws_pos = look_ahead;
                    ++look_ahead;
                }
                else if (next_ch == '\r')
                {
                    // Check for \r\n
                    if (look_ahead + 1 < m_input.length() && m_input[look_ahead + 1] == '\n')
                    {
                        // Skip backslash, trailing whitespace, CR, and LF
                        size_t old_pos = m_position;
                        m_position = look_ahead + 2;
                        // Update source location
                        for (size_t i = old_pos; i < m_position; ++i)
                        {
                            if (m_input[i] == '\n')
                            {
                                ++m_source_location.line;
                                m_source_location.column = 1;
                            }
                        }
                        warn_trailing_ws();
                        break;
                    }

                    // Just \r, treat as continuation
                    size_t old_pos = m_position;
                    m_position = look_ahead + 1;
                    // Update source location
                    for (size_t i = old_pos; i < m_position; ++i)
                    {
                        if (m_input[i] == '\n')
                        {
                            ++m_source_location.line;
                            m_source_location.column = 1;
                        }
                    }
                    warn_trailing_ws();
                    break;
                }
                else if (next_ch == '\n')
                {
                    // Skip backslash, trailing whitespace, and newline
                    size_t old_pos = m_position;
                    m_position = look_ahead + 1;
                    // Update source location
                    ++m_source_location.line;
                    m_source_location.column = 1;
                    warn_trailing_ws();
                    break;
                }
                else
                {
                    // Backslash not followed by newline, stop skipping whitespace
                    break;
                }
            }

            // If we didn't find a newline after the backslash and optional whitespace, stop
            if (look_ahead >= m_input.length() || (m_input[look_ahead] != '\n' && m_input[look_ahead] != '\r'))
            {
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
    SourceLocation start_loc = m_source_location;
    std::string number_str;
    bool has_decimal_point = false;
    bool has_exponent = false;

    // Integer part
    while (m_position < m_input.length() && std::isdigit(static_cast<unsigned char>(current_char())))
    {
        number_str += current_char();
        advance();
    }

    // Decimal part
    if (m_position < m_input.length() && current_char() == '.')
    {
        has_decimal_point = true;
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
        has_exponent = true;
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

    // Check if number is immediately followed by a letter or underscore (invalid)
    if (m_position < m_input.length())
    {
        char next_ch = current_char();
        if (std::isalpha(static_cast<unsigned char>(next_ch)) || next_ch == '_')
        {
            // Invalid: number followed directly by identifier character
            // Consume all remaining identifier characters to make the entire sequence one INVALID token
            while (m_position < m_input.length() && is_identifier_continue(current_char()))
            {
                advance();
            }
            size_t length = m_position - start;
            error(LexerErrorCode::INVALID_NUMBER, start_loc);
            return {TokenType::INVALID, start, length};
        }
    }

    size_t length = m_position - start;

    // Determine if this is an integer or floating-point number
    if (has_decimal_point || has_exponent)
    {
        // Floating-point number
        char *end;
        double value = std::strtod(number_str.c_str(), &end);
        return {value, start, length};
    }
    else
    {
        // Integer number
        char *end;
        int value = static_cast<int>(std::strtol(number_str.c_str(), &end, 10));
        return {value, start, length};
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
    size_t peek_pos = m_position + offset;
    if (peek_pos < m_input.length())
    {
        return m_input[peek_pos];
    }
    return '\0';
}

void Lexer::advance(size_t count)
{
    for (size_t i = 0; i < count && m_position < m_input.length(); ++i)
    {
        if (m_input[m_position] == '\n')
        {
            ++m_source_location.line;
            m_source_location.column = 1;
        }
        else
        {
            ++m_source_location.column;
        }
        ++m_position;
    }
}

SourceLocation Lexer::position_to_location(size_t pos) const
{
    SourceLocation loc{1, 1};
    for (size_t i = 0; i < pos && i < m_input.length(); ++i)
    {
        if (m_input[i] == '\n')
        {
            ++loc.line;
            loc.column = 1;
        }
        else
        {
            ++loc.column;
        }
    }
    return loc;
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

    static constexpr TextTokenType reserved[]{
        {"if", TokenType::IF},                // keywords
        {"elseif", TokenType::ELSE_IF},       //
        {"else", TokenType::ELSE},            //
        {"endif", TokenType::END_IF},         //
        {"param", TokenType::PARAM},          //
        {"endparam", TokenType::END_PARAM},   //
        {"false", TokenType::FALSE},          // boolean values
        {"true", TokenType::TRUE},            //
        {"bool", TokenType::TYPE_BOOL},       // type names
        {"int", TokenType::TYPE_INT},         //
        {"float", TokenType::TYPE_FLOAT},     //
        {"complex", TokenType::TYPE_COMPLEX}, //
        {"color", TokenType::TYPE_COLOR},     //
        {"p1", TokenType::P1},                // Built-in variables
        {"p2", TokenType::P2},                //
        {"p3", TokenType::P3},                //
        {"p4", TokenType::P4},                //
        {"p5", TokenType::P5},                //
        {"pixel", TokenType::PIXEL},          //
        {"lastsqr", TokenType::LAST_SQR},     //
        {"rand", TokenType::RAND},            //
        {"pi", TokenType::PI},                //
        {"e", TokenType::E},                  //
        {"maxit", TokenType::MAX_ITER},       //
        {"scrnmax", TokenType::SCREEN_MAX},   //
        {"scrnpix", TokenType::SCREEN_PIXEL}, //
        {"whitesq", TokenType::WHITE_SQUARE}, //
        {"ismand", TokenType::IS_MAND},       //
        {"center", TokenType::CENTER},        //
        {"magxmag", TokenType::MAG_X_MAG},    //
        {"rotskew", TokenType::ROT_SKEW},     //
        {"sinh", TokenType::SINH},            // Built-in functions
        {"cosh", TokenType::COSH},            //
        {"cosxx", TokenType::COSXX},          //
        {"sin", TokenType::SIN},              //
        {"cos", TokenType::COS},              //
        {"cotanh", TokenType::COTANH},        //
        {"cotan", TokenType::COTAN},          //
        {"tanh", TokenType::TANH},            //
        {"tan", TokenType::TAN},              //
        {"sqrt", TokenType::SQRT},            //
        {"log", TokenType::LOG},              //
        {"exp", TokenType::EXP},              //
        {"abs", TokenType::ABS},              //
        {"conj", TokenType::CONJ},            //
        {"real", TokenType::REAL},            //
        {"imag", TokenType::IMAG},            //
        {"flip", TokenType::FLIP},            //
        {"fn1", TokenType::FN1},              //
        {"fn2", TokenType::FN2},              //
        {"fn3", TokenType::FN3},              //
        {"fn4", TokenType::FN4},              //
        {"srand", TokenType::SRAND},          //
        {"asinh", TokenType::ASINH},          //
        {"acosh", TokenType::ACOSH},          //
        {"asin", TokenType::ASIN},            //
        {"acos", TokenType::ACOS},            //
        {"atanh", TokenType::ATANH},          //
        {"atan", TokenType::ATAN},            //
        {"cabs", TokenType::CABS},            //
        {"sqr", TokenType::SQR},              //
        {"floor", TokenType::FLOOR},          //
        {"ceil", TokenType::CEIL},            //
        {"trunc", TokenType::TRUNC},          //
        {"round", TokenType::ROUND},          //
        {"ident", TokenType::IDENT},          //
        {"one", TokenType::ONE},              //
        {"zero", TokenType::ZERO},            //
    };

    const auto to_lower = [](std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
        return text;
    };
    if (auto it = std::find_if(std::begin(reserved), std::end(reserved),
            [&identifier, &to_lower](const TextTokenType &kw) { return kw.text == to_lower(identifier); });
        it != std::end(reserved))
    {
        return {it->type, std::string{it->text}, start, length};
    }

    static constexpr TextTokenType section_names[]{
        {"global", TokenType::GLOBAL},            // Section names
        {"builtin", TokenType::BUILTIN},          //
        {"init", TokenType::INIT},                //
        {"loop", TokenType::LOOP},                //
        {"bailout", TokenType::BAILOUT},          //
        {"perturbinit", TokenType::PERTURB_INIT}, //
        {"perturbloop", TokenType::PERTURB_LOOP}, //
        {"default", TokenType::DEFAULT},          //
        {"switch", TokenType::SWITCH},            //
    };
    if (auto it = std::find_if(std::begin(section_names), std::end(section_names),
            [&identifier, &to_lower](const TextTokenType &kw) { return kw.text == to_lower(identifier); });
        it != std::end(section_names))
    {
        if (current_char() == ':')
        {
            advance();                   // Consume the colon for section names
            length = m_position - start; // Recalculate length to include the colon
            return {it->type, std::string{it->text}, start, length};
        }
    }

    // Not a reserved word, return as identifier
    return {TokenType::IDENTIFIER, to_lower(identifier), start, length};
}

Token Lexer::lex_string()
{
    size_t start = m_position;
    std::string str_value;

    // Skip opening quote
    advance();

    while (!at_end())
    {
        char ch = current_char();

        if (ch == '\\')
        {
            // Escape sequence
            advance();
            if (at_end())
            {
                // Unterminated string (backslash at end)
                size_t length = m_position - start;
                return {TokenType::INVALID, start, length};
            }

            char escaped_ch = current_char();
            if (escaped_ch == '"')
            {
                // Escaped quote
                str_value.append(1, '"');
            }
            else if (escaped_ch == '\\')
            {
                // Escaped backslash
                str_value.append(1, '\\');
            }
            else
            {
                // Unknown escape sequence - treat as literal
                str_value.append(1, escaped_ch);
            }
            advance();
        }
        else if (ch == '"')
        {
            // End of string
            advance(); // Skip closing quote
            size_t length = m_position - start;
            return {TokenType::STRING, str_value, start, length};
        }
        else if (ch == '\n')
        {
            // Unterminated string (newline before closing quote)
            size_t length = m_position - start;
            return {TokenType::INVALID, start, length};
        }
        else
        {
            // Regular character
            str_value += ch;
            advance();
        }
    }

    // Reached end of input without closing quote
    size_t length = m_position - start;
    return {TokenType::INVALID, start, length};
}

} // namespace formula
