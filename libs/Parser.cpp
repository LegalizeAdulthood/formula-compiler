// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Parser.h>

#include <formula/Lexer.h>
#include <formula/NodeTyper.h>
#include <formula/ParseOptions.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace formula::ast;

namespace formula::parser
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

class FormulaParser : public Parser
{
public:
    FormulaParser(std::string_view text, const Options &options) :
        m_ast(std::make_shared<FormulaSections>()),
        m_text(text),
        m_lexer(text),
        m_options(options)
    {
    }
    ~FormulaParser() override = default;

    FormulaSectionsPtr parse() override;

    const std::vector<Diagnostic> &get_warnings() const override
    {
        return m_warnings;
    }
    const std::vector<Diagnostic> &get_errors() const override
    {
        return m_errors;
    }

private:
    bool builtin_section();
    std::optional<double> signed_literal();
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
    Expr complex_literal();
    Expr builtin_function();
    Expr complex();
    Expr primary();
    void advance();
    void begin_tracking();
    void end_tracking();
    void backtrack();
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types)
    {
        return std::any_of(types.begin(), types.end(), [this](TokenType t) { return match(t); });
    }
    bool check(TokenType type) const;
    bool check(std::initializer_list<TokenType> types) const
    {
        return std::any_of(types.begin(), types.end(), [this](TokenType t) { return check(t); });
    }
    bool is_user_identifier(const Expr &expr) const;
    bool skip_separators();

    void warning(ErrorCode code) const
    {
        m_warnings.push_back(Diagnostic{code, m_lexer.position()});
    }
    void error(ErrorCode code) const
    {
        m_errors.push_back(Diagnostic{code, m_lexer.position()});
    }

    const std::string &str() const
    {
        return std::get<std::string>(m_curr.value);
    }
    double num() const
    {
        return std::get<double>(m_curr.value);
    }
    int integer() const
    {
        return std::get<int>(m_curr.value);
    }

private:
    FormulaSectionsPtr m_ast;
    std::string_view m_text;
    Lexer m_lexer;
    Token m_curr;
    std::vector<Token> m_backtrack;
    bool m_backtracking{};
    Options m_options{};
    mutable std::vector<Diagnostic> m_warnings;
    mutable std::vector<Diagnostic> m_errors;
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
            result.iterate = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
            result.bailout = expr;
        }
    }
    else
    {
        result.iterate = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
        result.bailout = expr;
    }
}

bool FormulaParser::builtin_section()
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

    if (!check(TokenType::INTEGER))
    {
        return false;
    }
    const int value{integer()};
    advance();

    if (value != 1 && value != 2)
    {
        return false;
    }

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->builtin = std::make_shared<SettingNode>("type", value);
    return true;
}

std::string_view s_default_number_settings[]{
    "angle", "magn", "maxiter", "periodicity", "skew", "stretch", //
};

std::optional<double> FormulaParser::signed_literal()
{
    const bool prefix_op = check({TokenType::PLUS, TokenType::MINUS});
    if (!(check({TokenType::INTEGER, TokenType::NUMBER}) || prefix_op))
    {
        return {};
    }
    const bool negative = check(TokenType::MINUS);
    if (prefix_op)
    {
        advance(); // consume prefix sign
    }
    if (check(TokenType::INTEGER))
    {
        const int value{integer()};
        advance();
        return negative ? -value : value;
    }
    if (check(TokenType::NUMBER))
    {
        const double value{num()};
        advance();
        return negative ? -value : value;
    }
    return {};
}

bool FormulaParser::default_number_setting(const std::string name)
{
    const std::optional num{signed_literal()};
    if (!num)
    {
        return false;
    }
    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<SettingNode>(name, num.value());
    return true;
}

std::optional<Complex> FormulaParser::complex_number()
{
    const auto get_literal = [this]() -> std::optional<double>
    {
        const bool negative = check(TokenType::MINUS);
        if (!check({TokenType::PLUS, TokenType::MINUS, TokenType::INTEGER, TokenType::NUMBER}))
        {
            return {};
        }
        if (check({TokenType::PLUS, TokenType::MINUS}))
        {
            advance();
        }
        if (check(TokenType::INTEGER))
        {
            const int value{integer()};
            advance();
            return negative ? -value : value;
        }
        if (check(TokenType::NUMBER))
        {
            const double value{num()};
            advance();
            return negative ? -value : value;
        }
        return {};
    };
    if (const std::optional literal{get_literal()})
    {
        return Complex{literal.value(), 0.0};
    }

    if (!check(TokenType::LEFT_PAREN))
    {
        return {};
    }
    advance();

    const std::optional real{get_literal()};
    if (!real)
    {
        return {};
    }

    if (!check(TokenType::COMMA))
    {
        return {};
    }
    advance();

    const std::optional imag{get_literal()};
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

bool FormulaParser::default_complex_setting(const std::string name)
{
    std::optional value{complex_number()};
    if (!value)
    {
        return false;
    }
    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<SettingNode>(name, value.value());
    return true;
}

bool FormulaParser::default_string_setting(const std::string name)
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

    m_ast->defaults = std::make_shared<SettingNode>(name, value);
    return true;
}

bool FormulaParser::default_method_setting()
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

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<SettingNode>("method", EnumName{method});
    return true;
}

bool FormulaParser::default_perturb_setting()
{
    if (check({TokenType::TRUE, TokenType::FALSE}))
    {
        const bool value{check(TokenType::TRUE)};
        advance();

        if (!check(TokenType::TERMINATOR))
        {
            return false;
        }
        advance();

        m_ast->defaults = std::make_shared<SettingNode>("perturb", value);
        return true;
    }

    Expr expr = conjunctive();
    if (!expr)
    {
        return false;
    }

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<SettingNode>("perturb", expr);
    return true;
}

bool FormulaParser::default_precision_setting()
{
    Expr expr = conjunctive();
    if (!expr)
    {
        return false;
    }

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<SettingNode>("precision", expr);
    return true;
}

bool FormulaParser::default_rating_setting()
{
    if (!check(TokenType::IDENTIFIER))
    {
        return false;
    }

    if (str() == "recommended" || str() == "average" || str() == "notrecommended")
    {
        const std::string rating{str() == "notrecommended" ? "notRecommended" : str()};
        advance(); // consume rating value

        if (!check(TokenType::TERMINATOR))
        {
            return false;
        }
        advance();

        m_ast->defaults = std::make_shared<SettingNode>("rating", EnumName{rating});
        return true;
    }
    return false;
}

bool FormulaParser::default_render_setting()
{
    if (!check({TokenType::TRUE, TokenType::FALSE}))
    {
        return false;
    }

    const bool value{check(TokenType::TRUE)};
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        return false;
    }
    advance();

    m_ast->defaults = std::make_shared<SettingNode>("render", value);
    return true;
}

std::optional<Expr> FormulaParser::param_string(const std::string &name)
{
    if (!check(TokenType::STRING))
    {
        return {};
    }
    Expr body = std::make_shared<SettingNode>(name, str());
    advance();
    return body;
}

std::optional<Expr> FormulaParser::param_default(const std::string &type)
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
        if (!check(TokenType::INTEGER))
        {
            return {};
        }
        Expr body = std::make_shared<SettingNode>("default", integer());
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

std::optional<Expr> FormulaParser::param_bool_expr(const std::string &name)
{
    Expr expr = conjunctive();
    if (!expr)
    {
        return {};
    }
    return std::make_shared<SettingNode>(name, expr);
}

std::optional<Expr> FormulaParser::param_enum()
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

std::optional<Expr> FormulaParser::param_bool(const std::string &name)
{
    if (!check({TokenType::TRUE, TokenType::FALSE}))
    {
        return {};
    }
    Expr body = std::make_shared<SettingNode>(name, check(TokenType::TRUE));
    advance();
    return body;
}

std::optional<Expr> FormulaParser::param_number(const std::string &type, const std::string &name)
{
    if (type == "int")
    {
        std::optional<double> value = signed_literal();
        if (!value)
        {
            return {};
        }
        return std::make_shared<SettingNode>(name, static_cast<int>(value.value()));
    }
    if (type == "float")
    {
        std::optional<double> value = signed_literal();
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

bool FormulaParser::default_param_block()
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

    skip_separators();

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

bool FormulaParser::default_section()
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

bool FormulaParser::switch_section()
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

std::optional<bool> FormulaParser::section_formula()
{
    if (check(TokenType::COLON))
    {
        return {};
    }
    const auto is_token = [this](TokenType tok)
    {
        return check(tok);
    };
    const auto is_section = [is_token]
    {
        return std::find_if(s_sections.begin(), s_sections.end(), is_token) != s_sections.end();
    };
    while (is_section())
    {
        TokenType section{m_curr.type};
        advance(); // consume section name and colon (handled by lexer)

        if (!check(TokenType::TERMINATOR))
        {
            return false;
        }
        advance(); // consume newline

        if (section == TokenType::BUILTIN)
        {
            if (m_ast->per_image || m_ast->builtin                       //
                || m_ast->initialize || m_ast->iterate || m_ast->bailout //
                || m_ast->perturb_initialize || m_ast->perturb_iterate   //
                || m_ast->defaults || m_ast->type_switch                 //
                || !builtin_section())
            {
                return false;
            }
        }
        else if (section == TokenType::DEFAULT)
        {
            if (m_ast->defaults || m_ast->type_switch //
                || !default_section())
            {
                return false;
            }
        }
        else if (section == TokenType::SWITCH)
        {
            if (m_ast->type_switch //
                || !switch_section())
            {
                return false;
            }
        }
        else if (Expr result = sequence())
        {
            switch (section)
            {
            case TokenType::GLOBAL:
                if (m_ast->per_image || m_ast->builtin                       //
                    || m_ast->initialize || m_ast->iterate || m_ast->bailout //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate   //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->per_image = result;
                break;

            case TokenType::BUILTIN:
                if (m_ast->builtin                                           //
                    || m_ast->initialize || m_ast->iterate || m_ast->bailout //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate   //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->builtin = result;
                break;

            case TokenType::INIT:
                if (m_ast->builtin                                           //
                    || m_ast->initialize || m_ast->iterate || m_ast->bailout //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate   //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->initialize = result;
                break;

            case TokenType::LOOP:
                if (m_ast->builtin                                         //
                    || m_ast->iterate || m_ast->bailout                    //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->iterate = result;
                break;

            case TokenType::BAILOUT:
                if (m_ast->builtin                                         //
                    || m_ast->bailout                                      //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->bailout = result;
                break;

            case TokenType::PERTURB_INIT:
                if (m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->perturb_initialize = result;
                break;

            case TokenType::PERTURB_LOOP:
                if (m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->perturb_iterate = result;
                break;

            case TokenType::DEFAULT:
                if (m_ast->defaults || m_ast->type_switch)
                {
                    return false;
                }
                m_ast->defaults = result;
                break;

            case TokenType::SWITCH:
                if (m_ast->type_switch)
                {
                    return false;
                }
                m_ast->type_switch = result;
                break;

            default:
                return false;
            }
        }
    }

    // some unknown section name?
    if (check(TokenType::COLON))
    {
        return false;
    }

    if (check(TokenType::END_OF_INPUT))
    {
        return true;
    }

    return {}; // wasn't a section-ized formula
}

// If parsing failed, return nullptr instead of partially constructed AST
FormulaSectionsPtr FormulaParser::parse()
{
    advance();

    skip_separators();
    if (const std::optional result = section_formula(); result)
    {
        return result.value() ? m_ast : nullptr;
    }

    Expr result;
    if (check(TokenType::COLON))
    {
        // no init section, so make an empty statement sequence
        result = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
    }
    else
    {
        result = sequence();
    }
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
    else
    {
        m_ast->initialize = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
    }

    // Check if we have multiple top-level newline-separated statements (StatementSeqNode)
    split_iterate_bailout(*m_ast, result);

    return m_ast;
}

void FormulaParser::advance()
{
    m_curr = m_lexer.get_token();
    if (check(TokenType::INVALID))
    {
        error(ErrorCode::INVALID_TOKEN);
    }
    if (m_backtracking)
    {
        m_backtrack.push_back(m_curr);
    }
}

void FormulaParser::begin_tracking()
{
    m_backtrack.clear();
    m_backtracking = true;
}

void FormulaParser::end_tracking()
{
    m_backtrack.clear();
    m_backtracking = false;
}

void FormulaParser::backtrack()
{
    for (Token &t : m_backtrack)
    {
        m_lexer.put_token(t);
    }
    end_tracking();
}

bool FormulaParser::match(TokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool FormulaParser::check(TokenType type) const
{
    return m_curr.type == type;
}

// Only allow IdentifierNode for assignment target
bool FormulaParser::is_user_identifier(const Expr &expr) const
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
        if (std::find(std::begin(builtins), std::end(builtins), node->name()) == std::end(builtins))
        {
            return true;
        }

        // identifier matches a builtin variable
        if (m_options.allow_builtin_assignment)
        {
            warning(ErrorCode::BUILTIN_VARIABLE_ASSIGNMENT);
            return true;
        }

        error(ErrorCode::BUILTIN_VARIABLE_ASSIGNMENT);
    }

    return false;
}

// Accept either comma or terminator (newline) as separator
bool FormulaParser::skip_separators()
{
    bool found{};
    while (match({TokenType::COMMA, TokenType::TERMINATOR}))
    {
        // skip separating tokens
        found = true;
    }
    return found;
}

Expr FormulaParser::sequence()
{
    skip_separators();

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
        skip_separators();

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

Expr FormulaParser::statement()
{
    if (check(TokenType::IF))
    {
        return if_statement();
    }
    return conjunctive();
}

Expr FormulaParser::if_statement()
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

Expr FormulaParser::if_statement_no_endif()
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

    // Accept comma or newline after condition
    if (!skip_separators())
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
        if (!skip_separators())
        {
            return nullptr;
        }
        else_block = block();
    }

    // Don't consume endif here - let the caller handle it
    return std::make_shared<IfStatementNode>(condition, then_block, else_block);
}

Expr FormulaParser::block()
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

        // Check for statement separator
        if (match({TokenType::COMMA, TokenType::TERMINATOR}))
        {
            // Skip any whitespace after comma
            skip_separators();
            // Continue parsing more statements
            continue;
        }

        // No newline means we should be at block end
        break;
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

Expr FormulaParser::assignment()
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

Expr FormulaParser::conjunctive()
{
    Expr left = comparative();

    // Handle logical operators: && and ||
    // Left-associative
    while (left && check({TokenType::LOGICAL_AND, TokenType::LOGICAL_OR}))
    {
        const std::string op{check(TokenType::LOGICAL_AND) ? "&&" : "||"};
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
Expr FormulaParser::comparative()
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

Expr FormulaParser::additive()
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

Expr FormulaParser::term()
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

Expr FormulaParser::unary()
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

Expr FormulaParser::power()
{
    Expr left = primary();

    // Left-associative: parse from left to right using a loop
    while (left && check(TokenType::POWER))
    {
        advance();              // consume '^'
        Expr right = primary(); // Parse the next primary, not recursive power()
        if (!right)
        {
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, '^', right);
    }

    return left;
}

Expr FormulaParser::builtin_var()
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
    TokenType::COSXX,                                                      //
    TokenType::COS, TokenType::SIN, TokenType::TAN, TokenType::COTAN,      //
    TokenType::COSH, TokenType::SINH, TokenType::TANH, TokenType::COTANH,  //
    TokenType::SQRT, TokenType::SQR,                                       //
    TokenType::LOG, TokenType::EXP,                                        //
    TokenType::CONJ, TokenType::REAL, TokenType::IMAG, TokenType::FLIP,    //
    TokenType::FN1, TokenType::FN2, TokenType::FN3, TokenType::FN4,        //
    TokenType::SRAND,                                                      //
    TokenType::ASIN, TokenType::ACOS, TokenType::ATAN,                     //
    TokenType::ACOSH, TokenType::ASINH, TokenType::ATANH,                  //
    TokenType::ABS, TokenType::CABS,                                       //
    TokenType::FLOOR, TokenType::CEIL, TokenType::TRUNC, TokenType::ROUND, //
    TokenType::IDENT, TokenType::ZERO, TokenType::ONE,                     //
};

Expr FormulaParser::builtin_function()
{
    if (const auto it = std::find(std::begin(s_builtin_fns), std::end(s_builtin_fns), m_curr.type);
        it != std::end(s_builtin_fns))
    {
        begin_tracking();
        const Token curr{m_curr};
        const std::string name{str()};
        advance(); // consume the function name
        if (Expr args = function_call())
        {
            end_tracking();
            return std::make_shared<FunctionCallNode>(name, args);
        }
        backtrack();
        m_curr = curr;
    }
    return nullptr;
}

Expr FormulaParser::complex()
{
    double re;
    double im;
    if (check({TokenType::PLUS, TokenType::MINUS, TokenType::INTEGER, TokenType::NUMBER}))
    {
        bool negate{check(TokenType::MINUS)};
        if (check({TokenType::PLUS, TokenType::MINUS}))
        {
            advance();
        }
        if (check({TokenType::INTEGER, TokenType::NUMBER}))
        {
            if (check(TokenType::INTEGER))
            {
                re = static_cast<double>(integer());
            }
            else
            {
                re = num();
            }
            if (negate)
            {
                re = -re;
            }
            advance();

            if (check(TokenType::COMMA))
            {
                advance();

                negate = check(TokenType::MINUS);
                if (check({TokenType::PLUS, TokenType::MINUS, TokenType::INTEGER, TokenType::NUMBER}))
                {
                    if (check({TokenType::PLUS, TokenType::MINUS}))
                    {
                        advance();
                    }

                    if (check(TokenType::INTEGER))
                    {
                        im = static_cast<double>(integer());
                    }
                    else
                    {
                        im = num();
                    }
                    if (negate)
                    {
                        im = -im;
                    }
                    advance();

                    if (check(TokenType::RIGHT_PAREN))
                    {
                        advance();
                        return std::make_shared<LiteralNode>(Complex{re, im});
                    }
                }
            }
        }
    }

    return nullptr;
}

Expr FormulaParser::function_call()
{
    if (check(TokenType::LEFT_PAREN))
    {
        advance(); // consume left paren
        if (const Expr expr = complex_literal())
        {
            return expr;
        }

        if (const Expr args = conjunctive(); args && check(TokenType::RIGHT_PAREN))
        {
            advance(); // consume right paren
            return args;
        }
    }
    return nullptr;
}

Expr FormulaParser::number()
{
    if (check({TokenType::PLUS, TokenType::MINUS, TokenType::INTEGER, TokenType::NUMBER}))
    {
        bool negate{check(TokenType::MINUS)};
        if (check({TokenType::PLUS, TokenType::MINUS}))
        {
            advance();
        }
        if (check(TokenType::NUMBER))
        {
            const double val = num();
            Expr result = std::make_shared<LiteralNode>(negate ? -val : val);
            advance(); // consume the number
            return result;
        }
        if (check(TokenType::INTEGER))
        {
            const int val{integer()};
            Expr result = std::make_shared<LiteralNode>(negate ? -val : val);
            advance();
            return result;
        }
    }
    return nullptr;
}

Expr FormulaParser::identifier()
{
    if (check(TokenType::IDENTIFIER))
    {
        Expr result = std::make_shared<IdentifierNode>(str());
        advance(); // consume the identifier
        return result;
    }

    // Also allow some reserved words as identifiers in expression context
    // TODO: only allow true and false to be identifiers in legacy mode
    if (check({TokenType::TYPE_COLOR, TokenType::TRUE, TokenType::FALSE}))
    {
        Expr result = std::make_shared<IdentifierNode>(str());
        advance(); // consume the type token
        return result;
    }

    if (std::find(std::begin(s_builtin_fns), std::end(s_builtin_fns), m_curr.type) != std::end(s_builtin_fns))
    {
        if (m_options.allow_builtin_assignment)
        {
            Expr result = std::make_shared<IdentifierNode>(str());
            advance(); // consume the type token
            warning(ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT);
            return result;
        }

        error(ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT);
    }

    return nullptr;
}

Expr FormulaParser::complex_literal()
{
    begin_tracking();
    const Token curr{m_curr};
    if (Expr result = complex())
    {
        end_tracking();
        return result;
    }
    backtrack();
    m_curr = curr;
    return nullptr;
}
Expr FormulaParser::primary()
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

    if (Expr result = builtin_function())
    {
        return result;
    }

    if (Expr result = builtin_var())
    {
        return result;
    }

    // Should be after we checked for builtin variables and functions, so
    // we can detect legacy code that attempts to use these names as variables.
    if (Expr result = identifier())
    {
        return result;
    }

    if (check(TokenType::LEFT_PAREN))
    {
        advance();
        if (Expr expr = complex_literal())
        {
            return expr;
        }

        // Allow full expressions including assignment in parens
        if (Expr expr = conjunctive(); expr && check(TokenType::RIGHT_PAREN))
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
        Expr expr = conjunctive(); // Parse the inner expression
        if (expr && check(TokenType::MODULUS))
        {
            advance(); // consume closing '|'
            return std::make_shared<UnaryOpNode>('|', expr);
        }
        return nullptr; // missing closing '|'
    }

    error(ErrorCode::EXPECTED_PRIMARY);
    return nullptr;
}

} // namespace

ParserPtr create_parser(std::string_view text, const Options &options)
{
    return std::make_shared<FormulaParser>(text, options);
}

} // namespace formula::parser
