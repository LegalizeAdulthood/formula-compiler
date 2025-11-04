// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace formula
{

enum class TokenType
{
    END_OF_INPUT,
    NUMBER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    POWER,
    ASSIGN,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
    EQUAL,
    NOT_EQUAL,
    MODULUS,
    IDENT,
    LEFT_PAREN,
    RIGHT_PAREN,
    IF,
    ELSE_IF,
    ELSE,
    END_IF,
    // Builtin variables
    P1,
    P2,
    P3,
    P4,
    P5,
    PIXEL,
    LAST_SQR,
    RAND,
    PI,
    E,
    MAX_ITER,
    SCREEN_MAX,
    SCREEN_PIXEL,
    WHITE_SQUARE,
    IS_MAND,
    CENTER,
    MAG_X_MAG,
    ROT_SKEW,
    INVALID
};

struct Token
{
    Token() :
        type(TokenType::END_OF_INPUT),
        position(0),
        length(0)
    {
    }
    Token(TokenType t, size_t pos, size_t len) :
        type(t),
        position(pos),
        length(len)
    {
    }
    Token(double val, size_t pos, size_t len) :
        type(TokenType::NUMBER),
        value(val),
        position(pos),
        length(len)
    {
    }
    Token(TokenType t, std::string str_val, size_t pos, size_t len) :
        type(t),
        value(std::move(str_val)),
        position(pos),
        length(len)
    {
    }

    TokenType type;
    std::variant<double, std::string> value;
    size_t position;
    size_t length;
};

class Lexer
{
public:
    explicit Lexer(std::string_view input);

    Token next_token();
    Token peek_token();
    size_t position() const
    {
        return m_position;
    }
    bool at_end() const
    {
        return m_position >= m_input.length();
    }

private:
    void skip_whitespace();
    Token lex_number();
    bool is_number_start() const;
    Token lex_identifier();
    bool is_identifier_start() const;
    bool is_identifier_continue(char c) const;
    char current_char() const;
    char peek_char(size_t offset = 1) const;
    void advance(size_t count = 1);

    std::string_view m_input;
    size_t m_position;
    Token m_peek_token;
    bool m_has_peek;
};

using LexerPtr = std::shared_ptr<Lexer>;

LexerPtr lex(std::string_view input);

} // namespace formula
