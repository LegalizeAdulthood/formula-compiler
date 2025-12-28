// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/SourceLocation.h>

#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace formula::lexer
{

enum class LexerErrorCode
{
    NONE = 0,
    CONTINUATION_WITH_WHITESPACE,
    INVALID_NUMBER,
};

enum class TokenType
{
    NONE = 0,
    END_OF_INPUT = 1,
    INTEGER,
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
    LOGICAL_AND,
    LOGICAL_OR,
    MODULUS,
    IDENTIFIER,           // variable name or enum value
    CONSTANT_IDENTIFIER,  // #name
    PARAMETER_IDENTIFIER, // @name
    OPEN_PAREN,
    CLOSE_PAREN,
    COLON,
    COMMA,
    TERMINATOR,
    IF,           // keywords: if statement
    ELSE_IF,      //
    ELSE,         //
    END_IF,       //
    WHILE,        // while statement
    END_WHILE,    //
    REPEAT,       // repeat statement
    UNTIL,        //
    FUNC,         // function definition
    END_FUNC,     //
    PARAM,        // parameter block
    END_PARAM,    //
    HEADING,      // heading block
    END_HEADING,  //
    CTX_CONST,    // context-sensitive keywords
    CTX_IMPORT,   //
    CTX_NEW,      //
    CTX_RETURN,   //
    CTX_STATIC,   //
    CTX_THIS,     //
    GLOBAL,       // Section names global:
    BUILTIN,      // builtin:
    INIT,         // init:
    LOOP,         // loop:
    BAILOUT,      // bailout:
    PERTURB_INIT, // perturbinit:
    PERTURB_LOOP, // perturbloop:
    DEFAULT,      // default:
    SWITCH,       // switch:
    P1,           // Builtin variables
    P2,           //
    P3,           //
    P4,           //
    P5,           //
    PIXEL,        //
    LAST_SQR,     // lastsqr
    RAND,         //
    PI,           //
    E,            //
    MAX_ITER,     // maxit
    SCREEN_MAX,   // scrnmax
    SCREEN_PIXEL, // scrnpix
    WHITE_SQUARE, // whitesq
    IS_MAND,      // ismand
    CENTER,       //
    MAG_X_MAG,    // magxmag
    ROT_SKEW,     // rotskew
    SINH,         // Builtin functions
    COSH,
    COSXX,
    SIN,
    COS,
    COTANH,
    COTAN,
    TANH,
    TAN,
    SQRT,
    LOG,
    EXP,
    ABS,
    CONJ,
    REAL,
    IMAG,
    FLIP,
    FN1,
    FN2,
    FN3,
    FN4,
    SRAND,
    ASINH,
    ACOSH,
    ASIN,
    ACOS,
    ATANH,
    ATAN,
    CABS,
    SQR,
    FLOOR,
    CEIL,
    TRUNC,
    ROUND,
    IDENT,
    ONE,
    ZERO,
    INVALID,
    TRUE,             // boolean value
    FALSE,            // boolean value
    STRING,           // quoted string
    TYPE_IDENTIFIER,  // type name (bool, int, float, complex, color)
};

std::string to_string(TokenType value);

struct LexicalDiagnostic
{
    LexerErrorCode code{};
    SourceLocation location{};
};

struct Token
{
    Token() :
        type(TokenType::END_OF_INPUT)
    {
    }
    Token(TokenType t, SourceLocation pos, size_t len) :
        type(t),
        location(pos),
        length(len)
    {
    }
    Token(int val, SourceLocation pos, size_t len) :
        type(TokenType::INTEGER),
        value(val),
        location(pos),
        length(len)
    {
    }
    Token(double val, SourceLocation pos, size_t len) :
        type(TokenType::NUMBER),
        value(val),
        location(pos),
        length(len)
    {
    }
    Token(TokenType t, std::string str_val, SourceLocation pos, size_t len) :
        type(t),
        value(std::move(str_val)),
        location(pos),
        length(len)
    {
    }

    TokenType type;
    std::variant<int, double, std::string> value;
    SourceLocation location;
    size_t length{};
};

struct Options
{
    bool allow_identifier_prefix{};
};

class Lexer
{
public:
    explicit Lexer(std::string_view input);
    Lexer(std::string_view input, Options options);

    Token get_token();
    void put_token(Token token);
    Token peek_token();
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
    const std::vector<LexicalDiagnostic> &get_warnings() const
    {
        return m_warnings;
    }
    const std::vector<LexicalDiagnostic> &get_errors() const
    {
        return m_errors;
    }

private:
    void warning(LexerErrorCode code)
    {
        m_warnings.push_back(LexicalDiagnostic{code, m_source_location});
    }
    void warning(LexerErrorCode code, SourceLocation loc)
    {
        m_warnings.push_back(LexicalDiagnostic{code, loc});
    }
    void error(LexerErrorCode code)
    {
        m_errors.push_back(LexicalDiagnostic{code, m_source_location});
    }
    void error(LexerErrorCode code, SourceLocation loc)
    {
        m_errors.push_back(LexicalDiagnostic{code, loc});
    }
    void skip_whitespace();
    void skip_comment();
    Token lex_number();
    bool is_number_start() const;
    Token identifier();
    Token constant_identifier();
    Token parameter_identifier();
    Token string_literal();
    bool is_identifier_start() const;
    bool is_identifier_continue(char c) const;
    char current_char() const;
    char peek_char(size_t offset = 1) const;
    void advance(size_t count = 1);
    SourceLocation position_to_location(size_t pos) const;

    Options m_options;
    std::vector<LexicalDiagnostic> m_warnings;
    std::vector<LexicalDiagnostic> m_errors;
    std::string_view m_input;
    size_t m_position{};
    SourceLocation m_source_location;
    std::deque<Token> m_peek_tokens;
};

using LexerPtr = std::shared_ptr<Lexer>;

LexerPtr lex(std::string_view input);

} // namespace formula::lexer
