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
    Expr formula();

    FormulaSectionsPtr m_ast;
    std::string_view m_text;
    Lexer m_lexer;
    Token m_curr;
};

FormulaSectionsPtr Descent::parse()
{
    m_ast->bailout = formula();
    return m_ast;
}

Expr Descent::formula()
{
    m_curr = m_lexer.next_token();

    if (m_curr.type == TokenType::NUMBER)
    {
        return std::make_shared<NumberNode>(std::get<double>(m_curr.value));
    }
    if (m_curr.type == TokenType::IDENTIFIER)
    {
        return std::make_shared<IdentifierNode>(std::get<std::string>(m_curr.value));
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
