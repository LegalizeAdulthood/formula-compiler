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
    Expr sequence();
    Expr statement();
    Expr if_statement();
    Expr if_statement_no_endif();
    Expr block();
    Expr assignment();
    Expr conjunctive();
    Expr comparative();
    Expr additive();
    Expr term();
    Expr unary();
    Expr power();
    Expr primary();
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool is_user_identifier(const Expr &expr) const;
    void skip_newlines();
    bool require_newlines();

    FormulaSectionsPtr m_ast;
    std::string_view m_text;
    Lexer m_lexer;
    Token m_curr;
};

void split_iterate_bailout(FormulaSections &result, const Expr &expr)
{
    if (const auto *seq = dynamic_cast<StatementSeqNode *>(expr.get()); seq)
    {
        if (seq->statements().size() > 1)
        {
            std::vector<Expr> statements = seq->statements();
            result.bailout = statements.back();
            statements.pop_back();
            result.iterate = std::make_shared<StatementSeqNode>(statements);
        }
        else
        {
            result.bailout = expr;
        }
    }
    else
    {
        result.bailout = expr;
    }
}

// If parsing failed, return nullptr instead of partially constructed AST
FormulaSectionsPtr Descent::parse()
{
    advance();
    Expr result = sequence();
    if (!result)
    {
        return nullptr;
    }

    if (match(TokenType::COLON))
    {
        m_ast->initialize = result;
        result = sequence();
        if (!result)
        {
            return nullptr;
        }
    }

    // Check if we have multiple top-level newline-separated statements (StatementSeqNode)
    split_iterate_bailout(*m_ast, result);

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

// Only allow IdentifierNode for assignment target
bool Descent::is_user_identifier(const Expr &expr) const
{
    if (const IdentifierNode *node = dynamic_cast<const IdentifierNode *>(expr.get()))
    {
        // Built-in variables
        static constexpr std::string_view builtins[]{
            "p1",
            "p2",
            "p3",
            "p4",
            "p5",
            "pixel",
            "lastsqr",
            "rand",
            "pi",
            "e",
            "maxit",
            "scrnmax",
            "scrnpix",
            "whitesq",
            "ismand",
            "center",
            "magxmag",
            "rotskew",
        };
        return std::find(std::begin(builtins), std::end(builtins), node->name()) == std::end(builtins);
    }
    return false;
}

void Descent::skip_newlines()
{
    while (match(TokenType::TERMINATOR))
    {
        // Skip all newlines
    }
}

bool Descent::require_newlines()
{
    if (!match(TokenType::TERMINATOR))
    {
        return false;
    }
    skip_newlines();
    return true;
}

Expr Descent::sequence()
{
    // Parse the first statement
    Expr first = statement();
    if (!first)
    {
        return nullptr;
    }

    // Collect all comma-separated statements
    std::vector<Expr> seq;
    seq.push_back(first);

    // Parse sequence of statements separated by COMMA or TERMINATOR (eol)
    while (check(TokenType::COMMA) || check(TokenType::TERMINATOR))
    {
        while (check(TokenType::COMMA) || check(TokenType::TERMINATOR))
        {
            advance();
        }

        // Check if we've reached end of input
        if (check(TokenType::END_OF_INPUT))
        {
            break;
        }

        Expr stmt = statement();
        if (!stmt)
        {
            return nullptr;
        }
        seq.push_back(stmt);
    }

    // Return appropriate node based on statement count
    if (seq.size() == 1)
    {
        return seq[0];
    }

    // Multiple newline-separated statements - return as a special StatementSeqNode
    // that will be split into iterate + bailout
    return std::make_shared<StatementSeqNode>(std::move(seq));
}

Expr Descent::statement()
{
    if (check(TokenType::IF))
    {
        return if_statement();
    }
    return conjunctive();
}

Expr Descent::if_statement()
{
    Expr result = if_statement_no_endif();
    if (!result)
    {
        return nullptr;
    }

    // Consume the endif token
    if (!match(TokenType::END_IF))
    {
        return nullptr;
    }

    return result;
}

Expr Descent::if_statement_no_endif()
{
    // Handle both 'if' and 'elseif' tokens
    if (!match(TokenType::IF) && !match(TokenType::ELSE_IF))
    {
        return nullptr;
    }

    // Parse condition in parentheses
    if (!match(TokenType::LEFT_PAREN))
    {
        return nullptr;
    }

    Expr condition = conjunctive();
    if (!condition)
    {
        return nullptr;
    }

    if (!match(TokenType::RIGHT_PAREN))
    {
        return nullptr;
    }

    // Require newline after condition
    if (!require_newlines())
    {
        return nullptr;
    }

    // Parse then block
    Expr then_block = block();

    // Parse else block (elseif or else or empty)
    Expr else_block = nullptr;

    if (check(TokenType::ELSE_IF))
    {
        // Recursively parse elseif as a nested if statement without consuming endif
        else_block = if_statement_no_endif();
        if (!else_block)
        {
            return nullptr;
        }
    }
    else if (match(TokenType::ELSE))
    {
        if (!require_newlines())
        {
            return nullptr;
        }
        else_block = block();
    }

    // Don't consume endif here - let the caller handle it
    return std::make_shared<IfStatementNode>(condition, then_block, else_block);
}

Expr Descent::block()
{
    // A block can be empty or contain statements
    // Check if we're at endif, else, or elseif - that means empty block
    if (check(TokenType::END_IF) || check(TokenType::ELSE) || check(TokenType::ELSE_IF))
    {
        return nullptr; // Empty block
    }

    // Parse statements in the block
    std::vector<Expr> statements;

    while (!check(TokenType::END_IF) && !check(TokenType::ELSE) && !check(TokenType::ELSE_IF))
    {
        Expr stmt = statement();
        if (!stmt)
        {
            // If we failed to parse a statement but have no statements yet, return nullptr
            // If we have statements, we might be at the end of the block
            if (statements.empty())
            {
                return nullptr;
            }
            break;
        }

        statements.push_back(stmt);

        // Check for comma separator
        if (match(TokenType::COMMA))
        {
            // Skip any whitespace after comma
            while (check(TokenType::TERMINATOR))
            {
                advance();
            }
            // Continue parsing more statements
            continue;
        }

        // After the statement, skip any newlines
        if (check(TokenType::TERMINATOR))
        {
            skip_newlines();
        }
        else
        {
            // No newline means we should be at block end
            break;
        }
    }

    // Return appropriate node based on statement count
    if (statements.empty())
    {
        return nullptr;
    }
    else if (statements.size() == 1)
    {
        return statements[0];
    }
    else
    {
        return std::make_shared<StatementSeqNode>(statements);
    }
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

Expr Descent::conjunctive()
{
    Expr left = comparative();

    // Handle logical operators: && and ||
    // Left-associative
    while (left && (check(TokenType::LOGICAL_AND) || check(TokenType::LOGICAL_OR)))
    {
        std::string op;
        if (m_curr.type == TokenType::LOGICAL_AND)
        {
            op = "&&";
        }
        else if (m_curr.type == TokenType::LOGICAL_OR)
        {
            op = "||";
        }

        advance();
        Expr right = comparative();
        if (!right)
        {
            return nullptr;
        }

        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

// Handle relational operators: <, <=, >, >=, ==, !=
Expr Descent::comparative()
{
    Expr left = assignment();

    while (left &&
        (check(TokenType::LESS_THAN) || check(TokenType::LESS_EQUAL)             //
            || check(TokenType::GREATER_THAN) || check(TokenType::GREATER_EQUAL) //
            || check(TokenType::EQUAL) || check(TokenType::NOT_EQUAL)))
    {
        std::string op;
        if (m_curr.type == TokenType::LESS_THAN)
            op = "<";
        else if (m_curr.type == TokenType::LESS_EQUAL)
            op = "<=";
        else if (m_curr.type == TokenType::GREATER_THAN)
            op = ">";
        else if (m_curr.type == TokenType::GREATER_EQUAL)
            op = ">=";
        else if (m_curr.type == TokenType::EQUAL)
            op = "==";
        else if (m_curr.type == TokenType::NOT_EQUAL)
            op = "!=";

        advance();
        Expr right = assignment();
        if (!right)
        {
            return nullptr;
        }

        left = std::make_shared<BinaryOpNode>(left, op, right);
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

    // Handle builtin variables - they should be treated as identifiers
    // Builtin tokens don't store string values, so map TokenType to name
    if (m_curr.type == TokenType::P1 || m_curr.type == TokenType::P2 || m_curr.type == TokenType::P3 ||
        m_curr.type == TokenType::P4 || m_curr.type == TokenType::P5 || m_curr.type == TokenType::PIXEL ||
        m_curr.type == TokenType::LAST_SQR || m_curr.type == TokenType::RAND || m_curr.type == TokenType::PI ||
        m_curr.type == TokenType::E || m_curr.type == TokenType::MAX_ITER || m_curr.type == TokenType::SCREEN_MAX ||
        m_curr.type == TokenType::SCREEN_PIXEL || m_curr.type == TokenType::WHITE_SQUARE ||
        m_curr.type == TokenType::IS_MAND || m_curr.type == TokenType::CENTER || m_curr.type == TokenType::MAG_X_MAG ||
        m_curr.type == TokenType::ROT_SKEW)
    {
        // Map token type to variable name
        std::string name;
        switch (m_curr.type)
        {
        case TokenType::P1:
            name = "p1";
            break;
        case TokenType::P2:
            name = "p2";
            break;
        case TokenType::P3:
            name = "p3";
            break;
        case TokenType::P4:
            name = "p4";
            break;
        case TokenType::P5:
            name = "p5";
            break;
        case TokenType::PIXEL:
            name = "pixel";
            break;
        case TokenType::LAST_SQR:
            name = "lastsqr";
            break;
        case TokenType::RAND:
            name = "rand";
            break;
        case TokenType::PI:
            name = "pi";
            break;
        case TokenType::E:
            name = "e";
            break;
        case TokenType::MAX_ITER:
            name = "maxit";
            break;
        case TokenType::SCREEN_MAX:
            name = "scrnmax";
            break;
        case TokenType::SCREEN_PIXEL:
            name = "scrnpix";
            break;
        case TokenType::WHITE_SQUARE:
            name = "whitesq";
            break;
        case TokenType::IS_MAND:
            name = "ismand";
            break;
        case TokenType::CENTER:
            name = "center";
            break;
        case TokenType::MAG_X_MAG:
            name = "magxmag";
            break;
        case TokenType::ROT_SKEW:
            name = "rotskew";
            break;
        default:
            return nullptr; // Shouldn't reach here
        }

        Expr result = std::make_shared<IdentifierNode>(name);
        advance(); // consume the builtin variable
        return result;
    }

    if (m_curr.type == TokenType::LEFT_PAREN)
    {
        advance();
        Expr expr = assignment(); // Allow full expressions including assignment in parens
        if (expr && check(TokenType::RIGHT_PAREN))
        {
            advance(); // consume ')'
            return expr;
        }
        return nullptr; // missing closing parenthesis
    }

    // Handle modulus operator |expr|
    if (m_curr.type == TokenType::MODULUS)
    {
        advance();
        Expr expr = assignment(); // Parse the inner expression
        if (expr && check(TokenType::MODULUS))
        {
            advance(); // consume closing '|'
            return std::make_shared<UnaryOpNode>('|', expr);
        }
        return nullptr; // missing closing '|'
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
