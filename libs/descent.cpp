// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "descent.h"

#include <formula/Lexer.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace formula::ast;

namespace formula::descent
{

namespace
{

constexpr TokenType s_builtin_vars[]{
    TokenType::P1, TokenType::P2, TokenType::P3, TokenType::P4,              //
    TokenType::P5, TokenType::PIXEL, TokenType::LAST_SQR, TokenType::RAND,   //
    TokenType::PI, TokenType::E, TokenType::MAX_ITER, TokenType::SCREEN_MAX, //
    TokenType::SCREEN_PIXEL, TokenType::WHITE_SQUARE, TokenType::IS_MAND,    //
    TokenType::CENTER, TokenType::MAG_X_MAG, TokenType::ROT_SKEW,            //
};

constexpr std::array<TokenType, 9> s_sections{
    TokenType::GLOBAL, TokenType::BUILTIN,                //
    TokenType::INIT, TokenType::LOOP, TokenType::BAILOUT, //
    TokenType::PERTURB_INIT, TokenType::PERTURB_LOOP,     //
    TokenType::DEFAULT, TokenType::SWITCH,                //
};

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
    bool builtin_section();
    std::optional<double> signed_number();
    bool default_number_setting(std::string name);
    std::optional<Complex> complex_number();
    bool default_complex_setting(std::string name);
    bool default_string_setting(std::string name);
    bool default_method_setting();
    bool default_perturb_setting();
    bool default_precision_setting();
    bool default_rating_setting();
    bool default_render_setting();
    std::optional<Expr> param_string(const std::string &name);
    std::optional<Expr> param_default(const std::string &type);
    std::optional<Expr> param_bool_expr(const std::string &name);
    std::optional<Expr> param_enum();
    std::optional<Expr> param_bool(const std::string &name);
    std::optional<Expr> param_number(const std::string &type, const std::string &name);
    bool default_param_block();
    bool default_section();
    bool switch_section();
    std::optional<bool> section_formula();
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
    Expr builtin_var();
    Expr function_call();
    Expr number();
    Expr identifier();
    Expr builtin_function();
    Expr primary();
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool check(std::initializer_list<TokenType> types) const
    {
        return std::any_of(types.begin(), types.end(), [this](TokenType t) { return check(t); });
    }
    bool is_user_identifier(const Expr &expr) const;
    void skip_newlines();
    bool require_newlines();

    const std::string &str() const
    {
        return std::get<std::string>(m_curr.value);
    }
    double num() const
    {
        return std::get<double>(m_curr.value);
    }

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

bool Descent::builtin_section()
{
    if (!check(TokenType::IDENTIFIER) || str() != "type")
    {
        return false;
    }
    advance(); // advance past type parameter name

    if (!check(TokenType::ASSIGN))
    {
        return false;
    }
    advance(); // advance past assignment token

    if (!check(TokenType::NUMBER))
    {
        return false;
    }
    const double value{num()};
    advance();

    if (value != 1.0 && value != 2.0)
    {
        return false;
    }

    m_ast->builtin = std::make_shared<SettingNode>("type", static_cast<int>(value));
    return true;
}

std::string_view s_default_number_settings[]{
    "angle", "magn", "maxiter", "periodicity", "skew", "stretch", //
};

std::optional<double> Descent::signed_number()
{
    const bool prefix_op = check({TokenType::PLUS, TokenType::MINUS});
    if (!(check(TokenType::NUMBER) || prefix_op))
    {
        return {};
    }
    const double sign = !check(TokenType::MINUS) ? 1.0 : -1.0;
    if (prefix_op)
    {
        advance(); // consume prefix sign
        if (!check(TokenType::NUMBER))
        {
            return {};
        }
    }
    const double value{num()};
    advance();

    return {sign * value};
}

bool Descent::default_number_setting(const std::string name)
{
    const std::optional num{signed_number()};
    if (!num)
    {
        return false;
    }

    m_ast->defaults = std::make_shared<SettingNode>(name, num.value());
    return true;
}

std::optional<Complex> Descent::complex_number()
{
    if (std::optional value{signed_number()})
    {
        return Complex{value.value(), 0.0};
    }

    if (!check(TokenType::LEFT_PAREN))
    {
        return {};
    }
    advance();

    const std::optional real{signed_number()};
    if (!real)
    {
        return {};
    }

    if (!check(TokenType::COMMA))
    {
        return {};
    }
    advance();

    const std::optional imag{signed_number()};
    if (!imag)
    {
        return {};
    }

    if (!check(TokenType::RIGHT_PAREN))
    {
        return {};
    }
    advance();

    return Complex{real.value(), imag.value()};
}

bool Descent::default_complex_setting(const std::string name)
{
    std::optional value{complex_number()};
    if (!value)
    {
        return false;
    }

    m_ast->defaults = std::make_shared<SettingNode>(name, value.value());
    return true;
}

bool Descent::default_string_setting(const std::string name)
{
    if (!check(TokenType::STRING))
    {
        return false;
    }
    const std::string value{str()};
    advance();

    m_ast->defaults = std::make_shared<SettingNode>(name, value);
    return true;
}

bool Descent::default_method_setting()
{
    if (!check(TokenType::IDENTIFIER))
    {
        return false;
    }
    const std::string method{str()};
    if (method != "guessing" && method != "multipass" && method != "onepass")
    {
        return false;
    }
    advance(); // consume method value
    m_ast->defaults = std::make_shared<SettingNode>("method", EnumName{method});
    return true;
}

bool Descent::default_perturb_setting()
{
    if (check({TokenType::TRUE, TokenType::FALSE}))
    {
        const bool value{check(TokenType::TRUE)};
        advance();

        m_ast->defaults = std::make_shared<SettingNode>("perturb", value);
        return true;
    }

    Expr expr = conjunctive();
    if (!expr)
    {
        return false;
    }

    m_ast->defaults = std::make_shared<SettingNode>("perturb", expr);
    return true;
}

bool Descent::default_precision_setting()
{
    Expr expr = conjunctive();
    if (!expr)
    {
        return false;
    }

    m_ast->defaults = std::make_shared<SettingNode>("precision", expr);
    return true;
}

bool Descent::default_rating_setting()
{
    if (!check(TokenType::IDENTIFIER))
    {
        return false;
    }

    if (str() == "recommended" || str() == "average" || str() == "notRecommended")
    {
        const std::string rating{str()};
        advance(); // consume rating value
        m_ast->defaults = std::make_shared<SettingNode>("rating", EnumName{rating});
        return true;
    }
    return false;
}

bool Descent::default_render_setting()
{
    if (!check({TokenType::TRUE, TokenType::FALSE}))
    {
        return false;
    }

    const bool value{check(TokenType::TRUE)};
    advance();
    m_ast->defaults = std::make_shared<SettingNode>("render", value);
    return true;
}

std::optional<Expr> Descent::param_string(const std::string &name)
{
    if (!check(TokenType::STRING))
    {
        return {};
    }
    Expr body = std::make_shared<SettingNode>(name, str());
    advance();
    return body;
}

std::optional<Expr> Descent::param_default(const std::string &type)
{
    if (type == "bool")
    {
        if (!check({TokenType::TRUE, TokenType::FALSE}))
        {
            return {};
        }
        Expr body = std::make_shared<SettingNode>("default", check(TokenType::TRUE));
        advance();
        return body;
    }
    if (type == "int")
    {
        if (!check(TokenType::NUMBER))
        {
            return {};
        }
        Expr body = std::make_shared<SettingNode>("default", static_cast<int>(num()));
        advance();
        return body;
    }
    if (type == "float")
    {
        if (!check(TokenType::NUMBER))
        {
            return {};
        }
        Expr body = std::make_shared<SettingNode>("default", num());
        advance();
        return body;
    }
    if (type == "complex")
    {
        if (const std::optional value{complex_number()})
        {
            return std::make_shared<SettingNode>("default", value.value());
        }
    }
    return {};
}

std::optional<Expr> Descent::param_bool_expr(const std::string &name)
{
    Expr expr = conjunctive();
    if (!expr)
    {
        return {};
    }
    return std::make_shared<SettingNode>(name, expr);
}

std::optional<Expr> Descent::param_enum()
{
    std::vector<std::string> values;
    while (check(TokenType::STRING))
    {
        values.push_back(str());
        advance();
    }
    if (values.empty())
    {
        return {};
    }
    return std::make_shared<SettingNode>("enum", values);
}

std::optional<Expr> Descent::param_bool(const std::string &name)
{
    if (!check({TokenType::TRUE, TokenType::FALSE}))
    {
        return {};
    }
    Expr body = std::make_shared<SettingNode>(name, check(TokenType::TRUE));
    advance();
    return body;
}

std::optional<Expr> Descent::param_number(const std::string &type, const std::string &name)
{
    if (type == "int")
    {
        std::optional<double> value = signed_number();
        if (!value)
        {
            return {};
        }
        return std::make_shared<SettingNode>(name, static_cast<int>(value.value()));
    }
    if (type == "float")
    {
        std::optional<double> value = signed_number();
        if (!value)
        {
            return {};
        }
        return std::make_shared<SettingNode>(name, value.value());
    }
    if (type == "complex")
    {
        std::optional<Complex> value = complex_number();
        if (!value)
        {
            return {};
        }
        return std::make_shared<SettingNode>(name, value.value());
    }
    return {};
}

bool Descent::default_param_block()
{
    std::string type;
    if (!check(TokenType::PARAM))
    {
        type = str();
        advance();
    }

    if (!check(TokenType::PARAM))
    {
        return false;
    }
    advance();

    if (!check(TokenType::IDENTIFIER))
    {
        return false;
    }
    const std::string name{str()};
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    Expr body;
    if (check({TokenType::IDENTIFIER, TokenType::DEFAULT}))
    {
        const std::string setting{str()};
        advance();

        if (!check(TokenType::ASSIGN))
        {
            return false;
        }
        advance();

        std::optional<Expr> value;
        if (setting == "caption" || setting == "hint" || setting == "text")
        {
            value = param_string(setting);
        }
        else if (setting == "default")
        {
            value = param_default(type);
        }
        else if (setting == "enabled" || setting == "visible")
        {
            value = param_bool_expr(setting);
        }
        else if (setting == "enum")
        {
            value = param_enum();
        }
        else if (setting == "expanded" || setting == "exponential" || setting == "selectable")
        {
            value = param_bool(setting);
        }
        else if (setting == "max" || setting == "min")
        {
            value = param_number(type, setting);
        }
        if (!value)
        {
            return false;
        }
        body = value.value();
        advance();
    }

    while (check(TokenType::TERMINATOR))
    {
        advance();
    }

    if (!check(TokenType::END_PARAM))
    {
        return false;
    }
    advance();

    if (!check({TokenType::TERMINATOR, TokenType::END_OF_INPUT}))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<ParamBlockNode>(type, name, body);
    return true;
}

bool Descent::default_section()
{
    if (check({TokenType::TYPE_BOOL, TokenType::TYPE_INT,   //
            TokenType::TYPE_FLOAT, TokenType::TYPE_COMPLEX, //
            TokenType::TYPE_COLOR, TokenType::PARAM}))
    {
        return default_param_block();
    }

    const bool is_center{check(TokenType::CENTER)};
    if (!(check(TokenType::IDENTIFIER) || is_center))
    {
        return false;
    }
    const std::string name{str()};
    advance(); // consume setting name

    if (!check(TokenType::ASSIGN))
    {
        return false;
    }
    advance(); // consume assignment operator

    if (std::find(std::begin(s_default_number_settings), std::end(s_default_number_settings), name) !=
        std::end(s_default_number_settings))
    {
        return default_number_setting(name);
    }

    if (is_center)
    {
        return default_complex_setting(name);
    }

    if (name == "helpfile" || name == "helptopic" || name == "title")
    {
        return default_string_setting(name);
    }

    if (name == "method")
    {
        return default_method_setting();
    }

    if (name == "perturb")
    {
        return default_perturb_setting();
    }

    if (name == "precision")
    {
        return default_precision_setting();
    }

    if (name == "rating")
    {
        return default_rating_setting();
    }

    if (name == "render")
    {
        return default_render_setting();
    }

    return false;
}

bool Descent::switch_section()
{
    if (!check(TokenType::IDENTIFIER))
    {
        return false;
    }
    const std::string name{str()};
    advance();

    if (!check(TokenType::ASSIGN))
    {
        return false;
    }
    advance();

    if (name == "type")
    {
        if (!check(TokenType::STRING))
        {
            return false;
        }
        const std::string value{str()};
        advance();

        if (!check(TokenType::TERMINATOR))
        {
            return false;
        }
        advance();

        m_ast->type_switch = std::make_shared<SettingNode>(name, value);
        return true;
    }

    // dest_param = builtin
    const std::string value{str()};
    if (const auto it = std::find(std::begin(s_builtin_vars), std::end(s_builtin_vars), m_curr.type);
        it != std::end(s_builtin_vars))
    {
    }
    // dest_param = param
    else if (!check(TokenType::IDENTIFIER))
    {
        return false;
    }
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->type_switch = std::make_shared<SettingNode>(name, SwitchParam{value});
    return true;
}

std::optional<bool> Descent::section_formula()
{
    const auto is_token = [this](TokenType tok) { return check(tok); };
    const auto is_section = [is_token]
    {
        return std::find_if(s_sections.begin(), s_sections.end(), is_token) != s_sections.end();
    };
    while (is_section())
    {
        TokenType section{m_curr.type};
        advance(); // consume section name

        if (!check(TokenType::COLON))
        {
            return false;
        }
        advance(); // consume colon

        if (!check(TokenType::TERMINATOR))
        {
            return false;
        }
        advance(); // consume newline

        if (section == TokenType::BUILTIN)
        {
            if (m_ast->initialize || m_ast->iterate || m_ast->bailout //
                || m_ast->perturb_initialize || m_ast->perturb_iterate //
                || m_ast->defaults || m_ast->type_switch //
                || !builtin_section())
            {
                return false;
            }
        }
        else if (section == TokenType::DEFAULT)
        {
            if (m_ast->type_switch //
                || !default_section())
            {
                return false;
            }
        }
        else if (section == TokenType::SWITCH)
        {
            if (!switch_section())
            {
                return false;
            }
        }
        else if (Expr result = sequence())
        {
            switch (section)
            {
            case TokenType::GLOBAL:
                if (m_ast->builtin                                           //
                    || m_ast->initialize || m_ast->iterate || m_ast->bailout //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate   //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->per_image = result;
                break;

            case TokenType::BUILTIN:
                if (m_ast->initialize || m_ast->iterate || m_ast->bailout  //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->builtin = result;
                break;

            case TokenType::INIT:
                if (m_ast->builtin                                         //
                    || m_ast->iterate || m_ast->bailout                    //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->initialize = result;
                break;

            case TokenType::LOOP:
                if (m_ast->builtin                                         //
                    || m_ast->bailout                                      //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->iterate = result;
                break;

            case TokenType::BAILOUT:
                if (m_ast->builtin                                         //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->bailout = result;
                break;

            case TokenType::PERTURB_INIT:
                if (m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->perturb_initialize = result;
                break;

            case TokenType::PERTURB_LOOP:
                if (m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->perturb_iterate = result;
                break;

            case TokenType::DEFAULT:
                if (m_ast->type_switch)
                {
                    return false;
                }
                m_ast->defaults = result;
                break;

            case TokenType::SWITCH:
                m_ast->type_switch = result;
                break;

            default:
                return false;
            }
        }
    }

    if (check(TokenType::END_OF_INPUT))
    {
        return true;
    }

    return {}; // wasn't a section-ized formula
}

// If parsing failed, return nullptr instead of partially constructed AST
FormulaSectionsPtr Descent::parse()
{
    advance();

    if (const std::optional result = section_formula(); result)
    {
        return result.value() ? m_ast : nullptr;
    }

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
        // Built-in variables and functions are not user identifiers
        static constexpr std::string_view builtins[]{
            "p1", "p2", "p3", "p4", "p5",               //
            "pixel", "lastsqr", "rand", "pi", "e",      //
            "maxit", "scrnmax", "scrnpix", "whitesq",   //
            "ismand", "center", "magxmag", "rotskew",   //
            "sin", "cos", "sinh", "cosh", "cosxx",      //
            "tan", "cotan", "tanh", "cotanh", "sqr",    //
            "log", "exp", "abs", "conj", "real",        //
            "imag", "flip", "fn1", "fn2", "fn3",        //
            "fn4", "srand", "asin", "acos", "asinh",    //
            "acosh", "atan", "atanh", "sqrt", "cabs",   //
            "floor", "ceil", "trunc", "round", "ident", //
            "one", "zero",                              //
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
    while (check({TokenType::COMMA, TokenType::TERMINATOR}))
    {
        while (check({TokenType::COMMA, TokenType::TERMINATOR}))
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
            break;
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
    if (check({TokenType::END_IF, TokenType::ELSE, TokenType::ELSE_IF}))
    {
        return nullptr; // Empty block
    }

    // Parse statements in the block
    std::vector<Expr> statements;

    while (!check({TokenType::END_IF, TokenType::ELSE, TokenType::ELSE_IF}))
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
    if (statements.size() == 1)
    {
        return statements[0];
    }
    return std::make_shared<StatementSeqNode>(statements);
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
    while (left && check({TokenType::LOGICAL_AND, TokenType::LOGICAL_OR}))
    {
        std::string op;
        if (check(TokenType::LOGICAL_AND))
        {
            op = "&&";
        }
        else if (check(TokenType::LOGICAL_OR))
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
        check({TokenType::LESS_THAN, TokenType::LESS_EQUAL,    //
            TokenType::GREATER_THAN, TokenType::GREATER_EQUAL, //
            TokenType::EQUAL, TokenType::NOT_EQUAL}))
    {
        std::string op;
        if (check(TokenType::LESS_THAN))
            op = "<";
        else if (check(TokenType::LESS_EQUAL))
            op = "<=";
        else if (check(TokenType::GREATER_THAN))
            op = ">";
        else if (check(TokenType::GREATER_EQUAL))
            op = ">=";
        else if (check(TokenType::EQUAL))
            op = "==";
        else if (check(TokenType::NOT_EQUAL))
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

    while (left && check({TokenType::PLUS, TokenType::MINUS}))
    {
        char op = (check(TokenType::PLUS)) ? '+' : '-';
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

    while (left && check({TokenType::MULTIPLY, TokenType::DIVIDE}))
    {
        char op = check(TokenType::MULTIPLY) ? '*' : '/';
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
    if (check({TokenType::PLUS, TokenType::MINUS}))
    {
        char op = check(TokenType::PLUS) ? '+' : '-';
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

Expr Descent::builtin_var()
{
    if (const auto it = std::find(std::begin(s_builtin_vars), std::end(s_builtin_vars), m_curr.type);
        it != std::end(s_builtin_vars))
    {
        Expr result = std::make_shared<IdentifierNode>(str());
        advance(); // consume the builtin variable
        return result;
    }
    return nullptr;
}

constexpr TokenType s_builtin_fns[]{
    TokenType::SINH, TokenType::COSH, TokenType::COSXX, TokenType::SIN,   //
    TokenType::COS, TokenType::COTANH, TokenType::COTAN, TokenType::TANH, //
    TokenType::TAN, TokenType::SQRT, TokenType::LOG, TokenType::EXP,      //
    TokenType::ABS, TokenType::CONJ, TokenType::REAL, TokenType::IMAG,    //
    TokenType::FLIP, TokenType::FN1, TokenType::FN2, TokenType::FN3,      //
    TokenType::FN4, TokenType::SRAND, TokenType::ASINH, TokenType::ACOSH, //
    TokenType::ASIN, TokenType::ACOS, TokenType::ATANH, TokenType::ATAN,  //
    TokenType::CABS, TokenType::SQR, TokenType::FLOOR, TokenType::CEIL,   //
    TokenType::TRUNC, TokenType::ROUND, TokenType::IDENT, TokenType::ONE, //
    TokenType::ZERO,                                                      //
};

Expr Descent::builtin_function()
{
    if (const auto it = std::find(std::begin(s_builtin_fns), std::end(s_builtin_fns), m_curr.type);
        it != std::end(s_builtin_fns))
    {
        const std::string name{str()};
        advance(); // consume the function name
        if (Expr args = function_call())
        {
            return std::make_shared<FunctionCallNode>(name, args);
        }
    }
    return nullptr;
}

Expr Descent::function_call()
{
    if (check(TokenType::LEFT_PAREN))
    {
        advance(); // consume left paren
        if (const Expr args = assignment(); args && check(TokenType::RIGHT_PAREN))
        {
            advance(); // consume right paren
            return args;
        }
    }
    return nullptr;
}

Expr Descent::number()
{
    if (check(TokenType::NUMBER))
    {
        Expr result = std::make_shared<NumberNode>(num());
        advance(); // consume the number
        return result;
    }
    return nullptr;
}

Expr Descent::identifier()
{
    if (check(TokenType::IDENTIFIER))
    {
        Expr result = std::make_shared<IdentifierNode>(str());
        advance(); // consume the identifier
        return result;
    }
    return nullptr;
}

Expr Descent::primary()
{
    // Check for invalid tokens first
    if (check(TokenType::INVALID))
    {
        return nullptr;
    }

    if (Expr result = number())
    {
        return result;
    }

    if (Expr result = identifier())
    {
        return result;
    }

    if (Expr result = builtin_var())
    {
        return result;
    }

    if (Expr result = builtin_function())
    {
        return result;
    }

    if (check(TokenType::LEFT_PAREN))
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
    if (check(TokenType::MODULUS))
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
