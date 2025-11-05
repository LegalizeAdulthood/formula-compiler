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
    Expr assignment();
    Expr additive();
    Expr term();
    Expr unary();
    Expr power();
    Expr primary();
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool is_user_identifier(const Expr &expr) const;

    FormulaSectionsPtr m_ast;
    std::string_view m_text;
    Lexer m_lexer;
    Token m_curr;
};

FormulaSectionsPtr Descent::parse()
{
    m_ast->bailout = expression();
    // If parsing failed, return nullptr instead of partially constructed AST
    if (!m_ast->bailout)
    {
        return nullptr;
    }
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

bool Descent::is_user_identifier(const Expr &expr) const
{
    // Only allow IdentifierNode for assignment target
    return dynamic_cast<const IdentifierNode *>(expr.get()) != nullptr;
}

Expr Descent::expression()
{
    advance();
    return assignment();
}

Expr Descent::assignment()
{
    Expr left = additive();

    // Assignment is right-associative and has lowest precedence
    if (left && check(TokenType::ASSIGN))
    {
        // Validate that left side is a user identifier
        if (!is_user_identifier(left))
        {
            // Left side must be a user identifier, not a reserved word or expression
            return nullptr;
        }

        // Get the variable name from the IdentifierNode
        const IdentifierNode *id_node = static_cast<const IdentifierNode *>(left.get());
        std::string var_name = id_node->name();

        advance();                 // consume '='
        Expr right = assignment(); // Right-associative: recursive call
        if (!right)
        {
            return nullptr;
        }

        return std::make_shared<AssignmentNode>(var_name, right);
    }

    return left;
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
    Expr left = unary();

    while (left && (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE)))
    {
        char op = (m_curr.type == TokenType::MULTIPLY) ? '*' : '/';
        advance(); // consume operator
        Expr right = unary();
        if (!right)
        {
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

Expr Descent::unary()
{
    if (check(TokenType::PLUS) || check(TokenType::MINUS))
    {
        char op = (m_curr.type == TokenType::PLUS) ? '+' : '-';
        advance();              // consume operator
        Expr operand = unary(); // Allow chaining: --1
        if (!operand)
        {
            return nullptr;
        }
        return std::make_shared<UnaryOpNode>(op, operand);
    }

    return power();
}

Expr Descent::power()
{
    Expr left = primary();

    // Right-associative: parse from right to left
    if (left && check(TokenType::POWER))
    {
        advance();            // consume '^'
        Expr right = power(); // Recursive call for right associativity
        if (!right)
        {
            return nullptr;
        }
        return std::make_shared<BinaryOpNode>(left, '^', right);
    }

    return left;
}

Expr Descent::primary()
{
    // Check for invalid tokens first
    if (m_curr.type == TokenType::INVALID)
    {
        return nullptr;
    }

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
        advance();                // consume '('
        Expr expr = assignment(); // Allow full expressions including assignment in parens
        if (expr && check(TokenType::RIGHT_PAREN))
        {
            advance(); // consume ')'
            return expr;
        }
        // missing closing parenthesis
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
