// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "descent.h"

#include <formula/Lexer.h>

using namespace formula::ast;

namespace formula::descent
{

namespace
{

class Descent
{
public:
    Descent(std::string_view text) :
        m_ast(std::make_shared<FormulaSections>()),
        m_text(text),
        m_lexer(text)
    {
    }

    FormulaSectionsPtr parse();

private:
    Expr expression();
    Expr additive();
    Expr term();
    Expr primary();
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;

    FormulaSectionsPtr m_ast;
    std::string_view m_text;
    Lexer m_lexer;
    Token m_curr;
};

FormulaSectionsPtr Descent::parse()
{
    m_ast->bailout = expression();
    return m_ast;
}

void Descent::advance()
{
    m_curr = m_lexer.next_token();
}

bool Descent::match(TokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool Descent::check(TokenType type) const
{
    return m_curr.type == type;
}

Expr Descent::expression()
{
    advance();
    return additive();
}

Expr Descent::additive()
{
    Expr left = term();

    while (left && (check(TokenType::PLUS) || check(TokenType::MINUS)))
    {
        char op = (m_curr.type == TokenType::PLUS) ? '+' : '-';
        advance(); // consume operator
        Expr right = term();
        if (!right)
        {
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

Expr Descent::term()
{
    Expr left = primary();

    while (left && (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE)))
    {
        char op = (m_curr.type == TokenType::MULTIPLY) ? '*' : '/';
        advance(); // consume operator
        Expr right = primary();
        if (!right)
        {
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

Expr Descent::primary()
{
    if (m_curr.type == TokenType::NUMBER)
    {
        Expr result = std::make_shared<NumberNode>(std::get<double>(m_curr.value));
        advance(); // consume the number
        return result;
    }

    if (m_curr.type == TokenType::IDENTIFIER)
    {
        Expr result = std::make_shared<IdentifierNode>(std::get<std::string>(m_curr.value));
        advance(); // consume the identifier
        return result;
    }

    if (m_curr.type == TokenType::LEFT_PAREN)
    {
        advance(); // consume '('
        Expr expr = additive();
        if (expr && check(TokenType::RIGHT_PAREN))
        {
            advance(); // consume ')'
            return expr;
        }
        return nullptr; // missing closing parenthesis
    }

    return nullptr;
}

} // namespace

FormulaSectionsPtr parse(std::string_view text)
{
    Descent parser(text);
    return parser.parse();
}

} // namespace formula::descent
