// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/Parser.h>

#include <formula/Lexer.h>
#include <formula/NodeTyper.h>
#include <formula/ParseOptions.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace formula::ast;
using namespace formula::lexer;

namespace formula::parser
{

namespace
{

bool equals_ignore_case(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
        const char left = static_cast<char>(std::tolower(static_cast<unsigned char>(lhs[index])));
        const char right = static_cast<char>(std::tolower(static_cast<unsigned char>(rhs[index])));
        if (left != right)
        {
            return false;
        }
    }
    return true;
}

enum class SettingType
{
    BOOLEAN,
    INTEGER,
    FLOATING_POINT,
    COMPLEX,
    STRING,
    ENUMERATION,
    BOOLEAN_EXPRESSION,
    INTEGER_EXPRESSION
};

enum class EntrySet : unsigned;

constexpr unsigned operator+(EntrySet value)
{
    return static_cast<unsigned>(value);
}

constexpr EntrySet operator|(EntrySet lhs, EntrySet rhs)
{
    return static_cast<EntrySet>(+lhs | +rhs);
}

constexpr EntrySet operator&(EntrySet lhs, EntrySet rhs)
{
    return static_cast<EntrySet>(+lhs & +rhs);
}

enum class EntrySet : unsigned int
{
    NONE = 0,
    // clang-format off
    FRACTAL        = 1U << static_cast<unsigned int>(EntryKind::FRACTAL),
    COLORING       = 1U << static_cast<unsigned int>(EntryKind::COLORING),
    TRANSFORMATION = 1U << static_cast<unsigned int>(EntryKind::TRANSFORMATION),
    CLASS          = 1U << static_cast<unsigned int>(EntryKind::CLASS),
    // clang-format on
    FORMULA = FRACTAL | COLORING | TRANSFORMATION,
    ALL = FORMULA | CLASS,
};

constexpr EntrySet entry_set_for(EntryKind entry_kind)
{
    switch (entry_kind)
    {
    case EntryKind::FRACTAL:
        return EntrySet::FRACTAL;
    case EntryKind::COLORING:
        return EntrySet::COLORING;
    case EntryKind::TRANSFORMATION:
        return EntrySet::TRANSFORMATION;
    case EntryKind::CLASS:
        return EntrySet::CLASS;
    }
    return EntrySet::NONE;
}

struct SettingMetadata
{
    std::string_view name;
    SettingType type;
    EntrySet entries;
};

class FormulaParser : public Parser
{
public:
    FormulaParser(std::string_view text, const Options &options);
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
    Expr default_integer_setting(const std::string &name);
    Expr default_number_setting(const std::string &name);
    std::optional<Complex> complex_number();
    Expr default_complex_setting(const std::string &name);
    Expr default_string_setting(const std::string &name);
    Expr default_enum_setting(const std::string &name);
    Expr default_method_setting();
    Expr default_perturb_setting();
    Expr default_precision_setting();
    Expr default_rating_setting();
    Expr default_render_setting();
    std::optional<Expr> param_string(const std::string &name);
    std::optional<Expr> param_default(const std::string &type);
    std::optional<Expr> param_bool_expr(const std::string &name);
    std::optional<Expr> param_enum();
    std::optional<Expr> param_bool(const std::string &name);
    std::optional<Expr> param_number(const std::string &type, const std::string &name);
    std::optional<Expr> function_block_default();
    Expr default_function_block();
    Expr default_heading_block();
    Expr default_param_block();
    Expr default_setting();
    Expr default_section();
    bool switch_section();
    std::optional<bool> section_formula();
    Expr sequence();
    bool is_extended() const;
    bool is_section_token(TokenType type) const;
    std::optional<size_t> section_rank(TokenType type) const;
    bool is_block_end() const;
    bool is_type_start() const;
    bool check_context(std::string_view keyword) const;
    bool match_context(std::string_view keyword);
    std::string type_name();
    std::vector<Expr> argument_list();
    Expr expression();
    bool consume_import_statement();
    void consume_imports();
    Expr declaration_statement();
    Expr function_declaration(bool is_static = false);
    Expr return_statement();
    Expr while_statement();
    Expr repeat_statement();
    bool is_builtin_var() const;
    bool is_builtin_fn() const;
    bool is_assignable() const;
    Expr foo();
    Expr statement();
    Expr if_statement();
    Expr if_statement_no_endif();
    Expr block();
    Expr variable();
    Expr assignment_statement();
    Expr conjunctive();
    Expr comparative();
    Expr additive();
    Expr term();
    Expr unary();
    Expr power();
    Expr postfix();
    Expr builtin_var();
    Expr builtin_fn();
    std::optional<std::vector<Expr>> function_call();
    Expr number();
    Expr identifier();
    Expr complex_literal();
    std::optional<Expr> builtin_function();
    Expr complex();
    Expr primary();
    void advance();
    void begin_tracking();
    void end_tracking();
    void backtrack();
    void split_iterate_bailout(const Expr &expr);
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool is_user_identifier(const Expr &expr) const;
    bool skip_separators();

    bool peek(TokenType type)
    {
        return m_lexer.peek_token().type == type;
    }

    bool match(std::initializer_list<TokenType> types)
    {
        return std::any_of(types.begin(), types.end(), [this](TokenType t) { return match(t); });
    }

    bool check(std::initializer_list<TokenType> types) const
    {
        return std::any_of(types.begin(), types.end(), [this](TokenType t) { return check(t); });
    }

    void warning(ErrorCode code) const
    {
        m_warnings.push_back(Diagnostic{code, m_lexer.source_location()});
    }
    void error(ErrorCode code) const
    {
        m_errors.push_back(Diagnostic{code, m_lexer.source_location()});
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

    FormulaSectionsPtr m_ast;
    Lexer m_lexer;
    Token m_curr;
    std::vector<Token> m_backtrack;
    bool m_backtracking{};
    Options m_options{};
    mutable std::vector<Diagnostic> m_warnings;
    mutable std::vector<Diagnostic> m_errors;
};

constexpr std::array<TokenType, 18> BUILTIN_VARS{
    TokenType::P1, TokenType::P2, TokenType::P3, TokenType::P4,              //
    TokenType::P5, TokenType::PIXEL, TokenType::LAST_SQR, TokenType::RAND,   //
    TokenType::PI, TokenType::E, TokenType::MAX_ITER, TokenType::SCREEN_MAX, //
    TokenType::SCREEN_PIXEL, TokenType::WHITE_SQUARE, TokenType::IS_MAND,    //
    TokenType::CENTER, TokenType::MAG_X_MAG, TokenType::ROT_SKEW,            //
};

constexpr std::array<TokenType, 37> BUILTIN_FNS{
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

constexpr std::array<TokenType, 9> SECTIONS{
    TokenType::GLOBAL, TokenType::BUILTIN,                //
    TokenType::INIT, TokenType::LOOP, TokenType::BAILOUT, //
    TokenType::PERTURB_INIT, TokenType::PERTURB_LOOP,     //
    TokenType::DEFAULT, TokenType::SWITCH,                //
};

constexpr std::array<TokenType, 14> EXTENDED_SECTIONS{
    TokenType::GLOBAL,       //
    TokenType::BUILTIN,      //
    TokenType::INIT,         //
    TokenType::LOOP,         //
    TokenType::BAILOUT,      //
    TokenType::PERTURB_INIT, //
    TokenType::PERTURB_LOOP, //
    TokenType::DEFAULT,      //
    TokenType::SWITCH,       //
    TokenType::FINAL,        //
    TokenType::TRANSFORM,    //
    TokenType::PUBLIC,       //
    TokenType::PROTECTED,    //
    TokenType::PRIVATE,      //
};

constexpr std::array<SettingMetadata, 15> DEFAULT_SETTINGS{
    SettingMetadata{"angle", SettingType::FLOATING_POINT, EntrySet::FRACTAL}, //
    {"center", SettingType::COMPLEX, EntrySet::FRACTAL},                      //
    {"helpfile", SettingType::STRING, EntrySet::FORMULA},                     //
    {"helptopic", SettingType::STRING, EntrySet::FORMULA},                    //
    {"magn", SettingType::FLOATING_POINT, EntrySet::FRACTAL},                 //
    {"maxiter", SettingType::INTEGER, EntrySet::FRACTAL},                     //
    {"method", SettingType::ENUMERATION, EntrySet::FRACTAL},                  //
    {"periodicity", SettingType::INTEGER, EntrySet::FRACTAL},                 //
    {"perturb", SettingType::BOOLEAN_EXPRESSION, EntrySet::FRACTAL},          //
    {"precision", SettingType::INTEGER_EXPRESSION, EntrySet::FORMULA},        //
    {"rating", SettingType::ENUMERATION, EntrySet::ALL},                      //
    {"render", SettingType::BOOLEAN, EntrySet::FORMULA},                      //
    {"skew", SettingType::FLOATING_POINT, EntrySet::FRACTAL},                 //
    {"stretch", SettingType::FLOATING_POINT, EntrySet::FRACTAL},              //
    {"title", SettingType::STRING, EntrySet::ALL},                            //
};

constexpr bool setting_allowed_for_entry(const SettingMetadata &setting, EntryKind entry_kind)
{
    return (setting.entries & entry_set_for(entry_kind)) != EntrySet::NONE;
}

lexer::Options lexer_options_for_parser(const Options &options)
{
    lexer::Options lexer_options;
    lexer_options.dialect = options.dialect;
    lexer_options.source_filename = options.source_filename;
    return lexer_options;
}

FormulaParser::FormulaParser(std::string_view text, const Options &options) :
    m_ast(std::make_shared<FormulaSections>()),
    m_lexer(text, lexer_options_for_parser(options)),
    m_options(options)
{
}

bool FormulaParser::builtin_section()
{
    if (!check(TokenType::IDENTIFIER))
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return false;
    }
    if (str() != "type")
    {
        error(ErrorCode::BUILTIN_SECTION_INVALID_KEY);
        return false;
    }
    advance(); // advance past type parameter name

    if (!check(TokenType::ASSIGN))
    {
        error(ErrorCode::EXPECTED_ASSIGNMENT);
        return false;
    }
    advance(); // advance past assignment token

    if (!check(TokenType::INTEGER))
    {
        error(ErrorCode::EXPECTED_INTEGER);
        return false;
    }
    const int value{integer()};
    advance();

    if (value != 1 && value != 2)
    {
        error(ErrorCode::BUILTIN_SECTION_INVALID_TYPE);
        return false;
    }

    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return false;
    }
    advance();

    m_ast->builtin = std::make_shared<SettingNode>("type", value);
    return true;
}

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

Expr FormulaParser::default_integer_setting(const std::string &name)
{
    if (!check(TokenType::INTEGER))
    {
        error(ErrorCode::EXPECTED_INTEGER);
        return nullptr;
    }
    const int value{integer()};
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>(name, value);
}

Expr FormulaParser::default_number_setting(const std::string &name)
{
    const std::optional num{signed_literal()};
    if (!num)
    {
        error(ErrorCode::EXPECTED_FLOATING_POINT);
        return nullptr;
    }
    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>(name, num.value());
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

    if (!check(TokenType::OPEN_PAREN))
    {
        error(ErrorCode::EXPECTED_OPEN_PAREN);
        return {};
    }
    advance();

    const std::optional real{get_literal()};
    if (!real)
    {
        error(ErrorCode::EXPECTED_FLOATING_POINT);
        return {};
    }

    if (!check(TokenType::COMMA))
    {
        error(ErrorCode::EXPECTED_COMMA);
        return {};
    }
    advance();

    const std::optional imag{get_literal()};
    if (!imag)
    {
        error(ErrorCode::EXPECTED_FLOATING_POINT);
        return {};
    }

    if (!check(TokenType::CLOSE_PAREN))
    {
        error(ErrorCode::EXPECTED_CLOSE_PAREN);
        return {};
    }
    advance();

    return Complex{real.value(), imag.value()};
}

Expr FormulaParser::default_complex_setting(const std::string &name)
{
    std::optional value{complex_number()};
    if (!value)
    {
        return nullptr;
    }
    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>(name, value.value());
}

Expr FormulaParser::default_string_setting(const std::string &name)
{
    if (!check(TokenType::QUOTED_STRING))
    {
        error(ErrorCode::EXPECTED_STRING);
        return nullptr;
    }
    const std::string value{str()};
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>(name, value);
}

Expr FormulaParser::default_method_setting()
{
    if (!check(TokenType::IDENTIFIER))
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return nullptr;
    }
    const std::string method{str()};
    if (method != "guessing" && method != "multipass" && method != "onepass")
    {
        error(ErrorCode::DEFAULT_SECTION_INVALID_METHOD);
        return nullptr;
    }
    advance(); // consume method value

    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>("method", EnumName{method});
}

Expr FormulaParser::default_perturb_setting()
{
    if (check({TokenType::LIT_TRUE, TokenType::LIT_FALSE}))
    {
        const bool value{check(TokenType::LIT_TRUE)};
        advance();

        if (!check(TokenType::TERMINATOR))
        {
            error(ErrorCode::EXPECTED_TERMINATOR);
            return nullptr;
        }
        advance();

        return std::make_shared<SettingNode>("perturb", value);
    }

    Expr expr = conjunctive();
    if (!expr)
    {
        return nullptr;
    }

    if (!check(TokenType::TERMINATOR))
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>("perturb", expr);
}

Expr FormulaParser::default_precision_setting()
{
    Expr expr = conjunctive();
    if (!expr)
    {
        return nullptr;
    }

    if (!check(TokenType::TERMINATOR))
    {
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>("precision", expr);
}

Expr FormulaParser::default_rating_setting()
{
    if (!check(TokenType::IDENTIFIER))
    {
        return nullptr;
    }

    if (!equals_ignore_case(str(), "recommended") && !equals_ignore_case(str(), "average") &&
        !equals_ignore_case(str(), "notrecommended"))
    {
        return nullptr;
    }

    const std::string rating{equals_ignore_case(str(), "notrecommended") ? "notRecommended" : str()};
    advance(); // consume rating value

    if (!check(TokenType::TERMINATOR))
    {
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>("rating", EnumName{rating});
}

Expr FormulaParser::default_render_setting()
{
    if (!check({TokenType::LIT_TRUE, TokenType::LIT_FALSE}))
    {
        return nullptr;
    }

    const bool value{check(TokenType::LIT_TRUE)};
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        return nullptr;
    }
    advance();

    return std::make_shared<SettingNode>("render", value);
}

std::optional<Expr> FormulaParser::param_string(const std::string &name)
{
    if (!check(TokenType::QUOTED_STRING))
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
        if (!check({TokenType::LIT_TRUE, TokenType::LIT_FALSE}))
        {
            return {};
        }
        Expr body = std::make_shared<SettingNode>("default", check(TokenType::LIT_TRUE));
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
    if (type != "bool" && type != "int" && type != "float" && type != "complex" && type != "color" //
        && !type.empty() && check({TokenType::IDENTIFIER, TokenType::TYPE_IDENTIFIER}))
    {
        Expr body = std::make_shared<SettingNode>("default", EnumName{str()});
        advance();
        return body;
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
    while (check(TokenType::QUOTED_STRING))
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
    if (!check({TokenType::LIT_TRUE, TokenType::LIT_FALSE}))
    {
        return {};
    }
    Expr body = std::make_shared<SettingNode>(name, check(TokenType::LIT_TRUE));
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

std::optional<Expr> FormulaParser::function_block_default()
{
    Expr expr = expression();
    if (!expr)
    {
        return {};
    }
    if (dynamic_cast<const FunctionCallNode *>(expr.get()) == nullptr)
    {
        return {};
    }
    return std::make_shared<SettingNode>("default", expr);
}

Expr FormulaParser::default_function_block()
{
    std::string type;
    if (!check(TokenType::FUNC))
    {
        if (!check({TokenType::TYPE_IDENTIFIER, TokenType::IDENTIFIER}))
        {
            return nullptr;
        }
        type = str();
        advance();
    }

    if (!check(TokenType::FUNC))
    {
        return nullptr;
    }
    advance();

    if (!check({TokenType::IDENTIFIER, TokenType::FN1, TokenType::FN2, TokenType::FN3, TokenType::FN4}))
    {
        return nullptr;
    }
    const std::string name{str()};
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        return nullptr;
    }
    advance();

    std::vector<Expr> settings;
    skip_separators();
    while (!check(TokenType::END_FUNC))
    {
        if (!check({TokenType::IDENTIFIER, TokenType::DEFAULT}))
        {
            return nullptr;
        }
        const std::string setting{str()};
        advance();

        if (!check(TokenType::ASSIGN))
        {
            return nullptr;
        }
        advance();

        std::optional<Expr> value;
        if (setting == "caption" || setting == "hint")
        {
            value = param_string(setting);
        }
        else if (setting == "default")
        {
            value = function_block_default();
        }
        else if (setting == "enabled" || setting == "visible")
        {
            value = param_bool_expr(setting);
        }
        if (!value)
        {
            return nullptr;
        }
        settings.push_back(value.value());
        skip_separators();
    }

    if (!check(TokenType::END_FUNC))
    {
        return nullptr;
    }
    advance();

    if (!check({TokenType::TERMINATOR, TokenType::END_OF_INPUT}))
    {
        return nullptr;
    }
    advance();

    Expr body;
    if (settings.size() == 1)
    {
        body = settings.front();
    }
    else if (!settings.empty())
    {
        body = std::make_shared<StatementSeqNode>(settings);
    }

    return std::make_shared<FunctionBlockNode>(type, name, body);
}

Expr FormulaParser::default_heading_block()
{
    if (!check(TokenType::HEADING))
    {
        return nullptr;
    }
    advance();

    if (!check(TokenType::TERMINATOR))
    {
        return nullptr;
    }
    advance();

    std::vector<Expr> settings;
    skip_separators();
    while (!check(TokenType::END_HEADING))
    {
        if (!check(TokenType::IDENTIFIER))
        {
            return nullptr;
        }
        const std::string setting{str()};
        advance();

        if (!check(TokenType::ASSIGN))
        {
            return nullptr;
        }
        advance();

        std::optional<Expr> value;
        if (setting == "caption" || setting == "text" || setting == "hint")
        {
            value = param_string(setting);
        }
        else if (setting == "enabled" || setting == "visible")
        {
            value = param_bool_expr(setting);
        }
        else if (setting == "expanded" || setting == "show" || setting == "extended")
        {
            value = param_bool(setting);
        }
        if (!value)
        {
            return nullptr;
        }
        settings.push_back(value.value());
        skip_separators();
    }

    if (!check(TokenType::END_HEADING))
    {
        return nullptr;
    }
    advance();

    if (!check({TokenType::TERMINATOR, TokenType::END_OF_INPUT}))
    {
        return nullptr;
    }
    advance();

    Expr body;
    if (settings.size() == 1)
    {
        body = settings.front();
    }
    else if (!settings.empty())
    {
        body = std::make_shared<StatementSeqNode>(settings);
    }

    return std::make_shared<HeadingBlockNode>(body);
}

Expr FormulaParser::default_param_block()
{
    std::string type;
    if (!check(TokenType::PARAM))
    {
        if (!check({TokenType::TYPE_IDENTIFIER, TokenType::IDENTIFIER}))
        {
            return nullptr;
        }
        type = str();
        advance();
    }

    if (!check(TokenType::PARAM))
    {
        return nullptr;
    }
    advance();

    if (!check(TokenType::IDENTIFIER))
    {
        return nullptr;
    }
    const std::string name{str()};
    advance();

    if (type.empty() && check(TokenType::ASSIGN))
    {
        advance();
        std::vector<std::string> path{name};
        if (!check(TokenType::IDENTIFIER))
        {
            return nullptr;
        }
        path.push_back(str());
        advance();
        while (check(TokenType::DOT))
        {
            advance();
            if (!check(TokenType::IDENTIFIER))
            {
                return nullptr;
            }
            path.push_back(str());
            advance();
        }
        if (!check(TokenType::TERMINATOR))
        {
            return nullptr;
        }
        advance();
        return std::make_shared<SettingNode>("param_forward", path);
    }

    if (!check(TokenType::TERMINATOR))
    {
        return nullptr;
    }
    advance();

    std::vector<Expr> settings;
    skip_separators();
    while (!check(TokenType::END_PARAM))
    {
        if (!check({TokenType::IDENTIFIER, TokenType::DEFAULT}))
        {
            return nullptr;
        }
        const std::string setting{str()};
        advance();

        if (!check(TokenType::ASSIGN))
        {
            return nullptr;
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
            return nullptr;
        }
        settings.push_back(value.value());
        skip_separators();
    }

    if (!check(TokenType::END_PARAM))
    {
        return nullptr;
    }
    advance();

    if (!check({TokenType::TERMINATOR, TokenType::END_OF_INPUT}))
    {
        return nullptr;
    }
    advance();

    Expr body;
    if (settings.size() == 1)
    {
        body = settings.front();
    }
    else if (!settings.empty())
    {
        body = std::make_shared<StatementSeqNode>(settings);
    }

    return std::make_shared<ParamBlockNode>(type, name, body);
}

Expr FormulaParser::default_enum_setting(const std::string &name)
{
    if (name == "method")
    {
        return default_method_setting();
    }
    if (name == "rating")
    {
        return default_rating_setting();
    }
    return nullptr;
}

Expr FormulaParser::default_section()
{
    std::vector<Expr> settings;

    // Parse multiple settings until we run out
    while (true)
    {
        while (check(TokenType::TERMINATOR))
        {
            advance();
        }

        if (check(TokenType::END_OF_INPUT) || is_section_token(m_curr.type))
        {
            break;
        }

        if (Expr result = default_setting())
        {
            settings.push_back(result);
        }
        else
        {
            break;
        }
    }

    if (settings.empty())
    {
        return nullptr;
    }

    if (settings.size() == 1)
    {
        return settings.front();
    }
    return std::make_shared<StatementSeqNode>(settings);
}

Expr FormulaParser::default_setting()
{
    if (check(TokenType::HEADING))
    {
        return default_heading_block();
    }

    if (check(TokenType::PARAM) ||
        (check({TokenType::TYPE_IDENTIFIER, TokenType::IDENTIFIER}) && peek(TokenType::PARAM)))
    {
        return default_param_block();
    }

    if (check(TokenType::FUNC) || (check({TokenType::TYPE_IDENTIFIER, TokenType::IDENTIFIER}) && peek(TokenType::FUNC)))
    {
        return default_function_block();
    }

    const bool is_center{check(TokenType::CENTER)};
    if (!(check(TokenType::IDENTIFIER) || is_center))
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return nullptr;
    }
    const std::string name{str()};
    advance(); // consume setting name

    if (!check(TokenType::ASSIGN))
    {
        error(ErrorCode::EXPECTED_ASSIGNMENT);
        return nullptr;
    }
    advance(); // consume assignment operator

    auto it = std::find_if(
        DEFAULT_SETTINGS.begin(), DEFAULT_SETTINGS.end(), [&name](const SettingMetadata &m) { return m.name == name; });
    if (it == DEFAULT_SETTINGS.end())
    {
        error(ErrorCode::DEFAULT_SECTION_INVALID_KEY);
        return nullptr;
    }
    if (!setting_allowed_for_entry(*it, m_options.entry_kind))
    {
        error(ErrorCode::DEFAULT_SECTION_INVALID_KEY);
        return nullptr;
    }

    switch (it->type)
    {
    case SettingType::BOOLEAN:
        return default_render_setting();
    case SettingType::INTEGER:
        return default_integer_setting(name);
    case SettingType::FLOATING_POINT:
        return default_number_setting(name);
    case SettingType::COMPLEX:
        return default_complex_setting(name);
    case SettingType::STRING:
        return default_string_setting(name);
    case SettingType::ENUMERATION:
        return default_enum_setting(name);
    case SettingType::BOOLEAN_EXPRESSION:
        return default_perturb_setting();
    case SettingType::INTEGER_EXPRESSION:
        return default_precision_setting();
    }

    return nullptr;
}

bool FormulaParser::switch_section()
{
    std::vector<Expr> settings;
    while (true)
    {
        while (check(TokenType::TERMINATOR))
        {
            advance();
        }

        if (check(TokenType::END_OF_INPUT) || is_section_token(m_curr.type))
        {
            break;
        }

        if (!check(TokenType::IDENTIFIER))
        {
            error(ErrorCode::EXPECTED_IDENTIFIER);
            return false;
        }
        const std::string name{str()};
        advance();

        if (!check(TokenType::ASSIGN))
        {
            error(ErrorCode::EXPECTED_ASSIGNMENT);
            return false;
        }
        advance();

        if (name == "type")
        {
            if (!check(TokenType::QUOTED_STRING))
            {
                error(ErrorCode::EXPECTED_STRING);
                return false;
            }
            settings.push_back(std::make_shared<SettingNode>(name, str()));
            advance();
        }
        else
        {
            // dest_param = builtin
            std::string value;
            if (const auto it = std::find(BUILTIN_VARS.begin(), BUILTIN_VARS.end(), m_curr.type);
                it != BUILTIN_VARS.end())
            {
                value = str();
            }
            // dest_param = param or #pixel
            else if (!check({TokenType::IDENTIFIER, TokenType::CONSTANT_IDENTIFIER}))
            {
                error(ErrorCode::EXPECTED_IDENTIFIER);
                return false;
            }
            else
            {
                value = str();
            }
            advance();
            settings.push_back(std::make_shared<SettingNode>(name, SwitchParam{value}));
        }

        if (!check(TokenType::TERMINATOR))
        {
            error(ErrorCode::EXPECTED_TERMINATOR);
            return false;
        }
        advance();
    }

    if (settings.empty())
    {
        error(ErrorCode::EXPECTED_TERMINATOR);
        return false;
    }

    if (settings.size() == 1)
    {
        m_ast->type_switch = settings.front();
    }
    else
    {
        m_ast->type_switch = std::make_shared<StatementSeqNode>(settings);
    }
    return true;
}

std::optional<bool> FormulaParser::section_formula()
{
    if (check(TokenType::COLON))
    {
        return {};
    }
    consume_imports();
    std::optional<size_t> last_section_rank;
    while (is_section_token(m_curr.type))
    {
        TokenType section{m_curr.type};
        if (section == TokenType::BUILTIN &&
            (m_ast->per_image || m_ast->initialize || m_ast->iterate || m_ast->bailout))
        {
            error(ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS);
            return false;
        }

        std::optional<size_t> rank{section_rank(section)};
        if (!rank)
        {
            error(ErrorCode::INVALID_SECTION);
            return false;
        }
        if (last_section_rank && *rank < *last_section_rank)
        {
            error(ErrorCode::INVALID_SECTION_ORDER);
            return false;
        }
        last_section_rank = rank;

        advance(); // consume section name and colon (handled by lexer)

        if (!check(TokenType::TERMINATOR))
        {
            error(ErrorCode::EXPECTED_TERMINATOR);
            return false;
        }
        advance(); // consume newline

        if (section == TokenType::BUILTIN)
        {
            if (m_ast->builtin)
            {
                error(ErrorCode::DUPLICATE_SECTION);
                return false;
            }
            if (m_ast->per_image                                          //
                || m_ast->initialize || m_ast->iterate || m_ast->bailout) //
            {
                error(ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS);
                return false;
            }
            if (m_ast->defaults || m_ast->type_switch)
            {
                error(ErrorCode::INVALID_SECTION_ORDER);
                return false;
            }
            if (!builtin_section())
            {
                return false;
            }
        }
        else if (section == TokenType::DEFAULT)
        {
            if (m_ast->defaults)
            {
                error(ErrorCode::DUPLICATE_SECTION);
                return false;
            }
            if (m_ast->type_switch)
            {
                error(ErrorCode::INVALID_SECTION_ORDER);
                return false;
            }
            if (Expr result = default_section())
            {
                m_ast->defaults = result;
            }
            else
            {
                return false;
            }
        }
        else if (section == TokenType::SWITCH)
        {
            if (m_ast->type_switch)
            {
                error(ErrorCode::DUPLICATE_SECTION);
                return false;
            }
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
                if (m_ast->per_image)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->builtin                                           //
                    || m_ast->initialize || m_ast->iterate || m_ast->bailout //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate   //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->per_image = result;
                break;

            case TokenType::BUILTIN:
                if (m_ast->builtin)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->initialize || m_ast->iterate || m_ast->bailout  //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->builtin = result;
                break;

            case TokenType::INIT:
                if (m_ast->initialize)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->builtin)
                {
                    error(ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS);
                    return false;
                }
                if (m_ast->iterate || m_ast->bailout                       //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->initialize = result;
                break;

            case TokenType::LOOP:
                if (m_ast->iterate)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->builtin)
                {
                    error(ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS);
                    return false;
                }
                if (m_ast->bailout                                         //
                    || m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->iterate = result;
                break;

            case TokenType::BAILOUT:
                if (m_ast->bailout)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->builtin)
                {
                    error(ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS);
                    return false;
                }
                if (m_ast->perturb_initialize || m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->bailout = result;
                break;

            case TokenType::PERTURB_INIT:
                if (m_ast->perturb_initialize)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->perturb_iterate //
                    || m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->perturb_initialize = result;
                break;

            case TokenType::PERTURB_LOOP:
                if (m_ast->perturb_iterate)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->defaults || m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->perturb_iterate = result;
                break;

            case TokenType::DEFAULT:
                if (m_ast->defaults)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                if (m_ast->type_switch)
                {
                    error(ErrorCode::INVALID_SECTION_ORDER);
                    return false;
                }
                m_ast->defaults = result;
                break;

            case TokenType::SWITCH:
                if (m_ast->type_switch)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                m_ast->type_switch = result;
                break;

            case TokenType::FINAL:
                if (m_ast->final)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                m_ast->final = result;
                break;

            case TokenType::TRANSFORM:
                if (m_ast->transform)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                m_ast->transform = result;
                break;

            case TokenType::PUBLIC:
                if (m_ast->public_members)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                m_ast->public_members = result;
                break;

            case TokenType::PROTECTED:
                if (m_ast->protected_members)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                m_ast->protected_members = result;
                break;

            case TokenType::PRIVATE:
                if (m_ast->private_members)
                {
                    error(ErrorCode::DUPLICATE_SECTION);
                    return false;
                }
                m_ast->private_members = result;
                break;

            default:
                return false;
            }
        }
        skip_separators();
        consume_imports();
    }

    // some unknown section name?
    if (check(TokenType::COLON))
    {
        error(ErrorCode::INVALID_SECTION);
        return false;
    }

    if (check(TokenType::END_OF_INPUT))
    {
        return true;
    }

    return {}; // wasn't a section-ized formula
}

std::vector<Expr> sequence_items(const Expr &expr)
{
    if (const auto *seq = dynamic_cast<const StatementSeqNode *>(expr.get()); seq)
    {
        return seq->statements();
    }
    return {expr};
}

Expr merge_sequences(const Expr &lhs, const Expr &rhs)
{
    std::vector<Expr> statements = sequence_items(lhs);
    const std::vector<Expr> rhs_statements = sequence_items(rhs);
    statements.insert(statements.end(), rhs_statements.begin(), rhs_statements.end());
    if (statements.size() == 1)
    {
        return statements.front();
    }
    return std::make_shared<StatementSeqNode>(statements);
}

void FormulaParser::split_iterate_bailout(const Expr &expr)
{
    if (const auto *seq = dynamic_cast<StatementSeqNode *>(expr.get()); seq)
    {
        if (seq->statements().size() > 1)
        {
            std::vector<Expr> statements = seq->statements();
            m_ast->bailout = statements.back();
            statements.pop_back();
            m_ast->iterate = std::make_shared<StatementSeqNode>(statements);
        }
        else
        {
            m_ast->iterate = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
            m_ast->bailout = expr;
        }
    }
    else
    {
        m_ast->iterate = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
        m_ast->bailout = expr;
    }
}

// If parsing failed, return nullptr instead of partially constructed AST
FormulaSectionsPtr FormulaParser::parse()
{
    advance();

    skip_separators();
    if (m_options.dialect == Dialect::EXTENDED)
    {
        if (const std::optional result = section_formula(); result)
        {
            return result.value() ? m_ast : nullptr;
        }
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
        if (m_errors.empty())
        {
            error(ErrorCode::EXPECTED_STATEMENT);
        }
        return nullptr;
    }

    if (m_options.entry_kind == EntryKind::COLORING)
    {
        if (!check(TokenType::END_OF_INPUT))
        {
            error(ErrorCode::EXPECTED_STATEMENT);
            return nullptr;
        }
        m_ast->final = result;
        return m_ast;
    }

    if (m_options.entry_kind == EntryKind::TRANSFORMATION)
    {
        if (!check(TokenType::END_OF_INPUT))
        {
            error(ErrorCode::EXPECTED_STATEMENT);
            return nullptr;
        }
        m_ast->transform = result;
        return m_ast;
    }

    if (m_options.entry_kind == EntryKind::CLASS)
    {
        if (is_section_token(m_curr.type))
        {
            const Expr public_prefix{result};
            if (const std::optional sections_result = section_formula(); sections_result)
            {
                if (!sections_result.value())
                {
                    return nullptr;
                }
                m_ast->public_members =
                    m_ast->public_members ? merge_sequences(public_prefix, m_ast->public_members) : public_prefix;
                return m_ast;
            }
        }
        if (!check(TokenType::END_OF_INPUT))
        {
            error(ErrorCode::EXPECTED_STATEMENT);
            return nullptr;
        }
        m_ast->public_members = result;
        return m_ast;
    }

    Expr init;
    if (match(TokenType::COLON))
    {
        init = result;
        result = sequence();
        if (!result)
        {
            if (m_errors.empty())
            {
                error(ErrorCode::EXPECTED_STATEMENT);
            }
            return nullptr;
        }
    }
    else
    {
        init = std::make_shared<StatementSeqNode>(std::vector<Expr>{});
    }

    // it's an error if not all the input was consumed
    if (!check(TokenType::END_OF_INPUT))
    {
        error(ErrorCode::EXPECTED_STATEMENT);
        return nullptr;
    }

    // Check if we have multiple top-level newline-separated statements (StatementSeqNode)
    m_ast->initialize = init;
    split_iterate_bailout(result);

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
        static constexpr std::string_view builtin_vars[]{
            "p1", "p2", "p3", "p4", "p5",             //
            "pixel", "lastsqr", "rand", "pi", "e",    //
            "maxit", "scrnmax", "scrnpix", "whitesq", //
            "ismand", "center", "magxmag", "rotskew", //
        };
        static constexpr std::string_view builtin_fns[]{
            "sin", "cos", "sinh", "cosh", "cosxx",      //
            "tan", "cotan", "tanh", "cotanh", "sqr",    //
            "log", "exp", "abs", "conj", "real",        //
            "imag", "flip", "fn1", "fn2", "fn3",        //
            "fn4", "srand", "asin", "acos", "asinh",    //
            "acosh", "atan", "atanh", "sqrt", "cabs",   //
            "floor", "ceil", "trunc", "round", "ident", //
            "one", "zero",                              //
        };
        const bool builtin_var =
            std::find(std::begin(builtin_vars), std::end(builtin_vars), node->name()) != std::end(builtin_vars);
        const bool builtin_fn =
            std::find(std::begin(builtin_fns), std::end(builtin_fns), node->name()) != std::end(builtin_fns);
        if (!builtin_var && !builtin_fn)
        {
            return true;
        }

        // identifier matches a builtin variable
        if (m_options.allow_builtin_assignment)
        {
            warning(ErrorCode::BUILTIN_VARIABLE_ASSIGNMENT);
            return true;
        }

        m_errors.push_back(Diagnostic{
            builtin_var ? ErrorCode::EXPECTED_STATEMENT : ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT, m_curr.location});
    }

    m_errors.push_back(Diagnostic{ErrorCode::EXPECTED_IDENTIFIER, m_curr.location});
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

bool FormulaParser::is_extended() const
{
    return m_options.dialect == Dialect::EXTENDED;
}

bool FormulaParser::is_section_token(TokenType type) const
{
    if (is_extended())
    {
        return std::find(EXTENDED_SECTIONS.begin(), EXTENDED_SECTIONS.end(), type) != EXTENDED_SECTIONS.end();
    }
    return std::find(SECTIONS.begin(), SECTIONS.end(), type) != SECTIONS.end();
}

std::optional<size_t> FormulaParser::section_rank(TokenType type) const
{
    const auto find_rank = [type](std::initializer_list<TokenType> sections) -> std::optional<size_t>
    {
        size_t rank{};
        for (TokenType section : sections)
        {
            if (section == type)
            {
                return rank;
            }
            ++rank;
        }
        return {};
    };

    switch (m_options.entry_kind)
    {
    case EntryKind::FRACTAL:
        return find_rank({TokenType::GLOBAL, TokenType::BUILTIN, TokenType::INIT, TokenType::LOOP, TokenType::BAILOUT,
            TokenType::PERTURB_INIT, TokenType::PERTURB_LOOP, TokenType::DEFAULT, TokenType::SWITCH});

    case EntryKind::COLORING:
        return find_rank({TokenType::GLOBAL, TokenType::INIT, TokenType::LOOP, TokenType::FINAL, TokenType::DEFAULT});

    case EntryKind::TRANSFORMATION:
        return find_rank({TokenType::GLOBAL, TokenType::TRANSFORM, TokenType::DEFAULT});

    case EntryKind::CLASS:
        return find_rank({TokenType::PUBLIC, TokenType::PROTECTED, TokenType::PRIVATE, TokenType::DEFAULT});
    }

    return {};
}

bool FormulaParser::is_block_end() const
{
    return check({TokenType::END_OF_INPUT, TokenType::END_IF, TokenType::ELSE, TokenType::ELSE_IF, TokenType::END_WHILE,
               TokenType::UNTIL, TokenType::END_FUNC}) ||
        is_section_token(m_curr.type);
}

bool FormulaParser::is_type_start() const
{
    return is_extended() && (check(TokenType::TYPE_IDENTIFIER) || check(TokenType::IDENTIFIER));
}

bool FormulaParser::check_context(std::string_view keyword) const
{
    if (!is_extended())
    {
        return false;
    }
    if (check(TokenType::IDENTIFIER) && equals_ignore_case(str(), keyword))
    {
        return true;
    }
    if (keyword == "const" && check(TokenType::CTX_CONST))
    {
        return true;
    }
    if (keyword == "import" && check(TokenType::CTX_IMPORT))
    {
        return true;
    }
    if (keyword == "new" && check(TokenType::CTX_NEW))
    {
        return true;
    }
    if (keyword == "return" && check(TokenType::CTX_RETURN))
    {
        return true;
    }
    if (keyword == "static" && check(TokenType::CTX_STATIC))
    {
        return true;
    }
    if (keyword == "this" && check(TokenType::CTX_THIS))
    {
        return true;
    }
    return false;
}

bool FormulaParser::match_context(std::string_view keyword)
{
    if (check_context(keyword))
    {
        advance();
        return true;
    }
    return false;
}

std::string FormulaParser::type_name()
{
    if (!is_type_start())
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return {};
    }
    const std::string type{str()};
    advance();
    return type;
}

std::vector<Expr> FormulaParser::argument_list()
{
    std::vector<Expr> args;
    if (!match(TokenType::OPEN_PAREN))
    {
        error(ErrorCode::EXPECTED_OPEN_PAREN);
        return args;
    }
    if (match(TokenType::CLOSE_PAREN))
    {
        return args;
    }
    do
    {
        Expr arg = expression();
        if (!arg)
        {
            return {};
        }
        args.push_back(arg);
    } while (match(TokenType::COMMA));
    if (!match(TokenType::CLOSE_PAREN))
    {
        error(ErrorCode::EXPECTED_CLOSE_PAREN);
        return {};
    }
    return args;
}

Expr FormulaParser::expression()
{
    if (is_extended())
    {
        return assignment_statement();
    }
    return conjunctive();
}

bool FormulaParser::consume_import_statement()
{
    if (!check_context("import"))
    {
        return false;
    }
    const SourceLocation location{m_curr.location};
    advance();
    if (!check(TokenType::QUOTED_STRING))
    {
        error(ErrorCode::EXPECTED_STRING);
        return true;
    }
    m_ast->imports.push_back(ImportDirective{str(), location, false});
    advance();
    return true;
}

void FormulaParser::consume_imports()
{
    while (is_extended() && check_context("import"))
    {
        consume_import_statement();
        skip_separators();
    }
}

Expr FormulaParser::declaration_statement()
{
    const std::string type{type_name()};
    if (type.empty())
    {
        return nullptr;
    }
    if (!check(TokenType::IDENTIFIER))
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return nullptr;
    }
    const std::string name{str()};
    advance();

    std::vector<Expr> dimensions;
    if (match(TokenType::OPEN_BRACKET))
    {
        if (match(TokenType::CLOSE_BRACKET))
        {
            dimensions.push_back(nullptr);
        }
        else
        {
            do
            {
                Expr dimension = expression();
                if (!dimension)
                {
                    return nullptr;
                }
                dimensions.push_back(dimension);
            } while (match(TokenType::COMMA));
            if (!match(TokenType::CLOSE_BRACKET))
            {
                error(ErrorCode::EXPECTED_CLOSE_PAREN);
                return nullptr;
            }
        }
    }

    Expr initializer;
    if (match(TokenType::ASSIGN))
    {
        initializer = expression();
        if (!initializer)
        {
            return nullptr;
        }
    }

    return std::make_shared<DeclarationNode>(type, name, std::move(dimensions), initializer);
}

Expr FormulaParser::function_declaration(bool is_static)
{
    std::string return_type;
    if (!check(TokenType::FUNC))
    {
        return_type = type_name();
    }

    if (!match(TokenType::FUNC))
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return nullptr;
    }
    if (!check(TokenType::IDENTIFIER))
    {
        error(ErrorCode::EXPECTED_IDENTIFIER);
        return nullptr;
    }
    const std::string name{str()};
    advance();

    std::vector<FunctionArgument> args;
    if (match(TokenType::OPEN_PAREN))
    {
        if (!check(TokenType::CLOSE_PAREN))
        {
            do
            {
                FunctionArgument arg;
                arg.is_const = match_context("const");
                arg.type = type_name();
                if (arg.type.empty())
                {
                    return nullptr;
                }
                arg.is_by_ref = match(TokenType::AMPERSAND);
                if (!check(TokenType::IDENTIFIER))
                {
                    error(ErrorCode::EXPECTED_IDENTIFIER);
                    return nullptr;
                }
                arg.name = str();
                advance();
                args.push_back(std::move(arg));
            } while (match(TokenType::COMMA));
        }
        if (!match(TokenType::CLOSE_PAREN))
        {
            error(ErrorCode::EXPECTED_CLOSE_PAREN);
            return nullptr;
        }
    }

    const bool is_const = match_context("const");
    skip_separators();
    Expr body = block();
    if (!match(TokenType::END_FUNC))
    {
        error(ErrorCode::EXPECTED_STATEMENT);
        return nullptr;
    }
    return std::make_shared<FunctionDeclNode>(return_type, name, std::move(args), body, is_const, is_static);
}

Expr FormulaParser::return_statement()
{
    if (!match_context("return"))
    {
        return nullptr;
    }
    if (check({TokenType::TERMINATOR, TokenType::COMMA, TokenType::END_FUNC, TokenType::END_OF_INPUT}))
    {
        return std::make_shared<ReturnNode>(nullptr);
    }
    Expr result = expression();
    if (!result)
    {
        if (m_errors.empty())
        {
            error(ErrorCode::EXPECTED_STATEMENT);
        }
        return nullptr;
    }
    return std::make_shared<ReturnNode>(result);
}

Expr FormulaParser::while_statement()
{
    if (!match(TokenType::WHILE))
    {
        return nullptr;
    }
    Expr condition = expression();
    if (!condition)
    {
        return nullptr;
    }
    if (!skip_separators())
    {
        error(ErrorCode::EXPECTED_STATEMENT_SEPARATOR);
        return nullptr;
    }
    Expr body = block();
    if (!match(TokenType::END_WHILE))
    {
        error(ErrorCode::EXPECTED_STATEMENT);
        return nullptr;
    }
    return std::make_shared<WhileNode>(condition, body);
}

Expr FormulaParser::repeat_statement()
{
    if (!match(TokenType::REPEAT))
    {
        return nullptr;
    }
    skip_separators();
    Expr body = block();
    if (!match(TokenType::UNTIL))
    {
        error(ErrorCode::EXPECTED_STATEMENT);
        return nullptr;
    }
    Expr condition = expression();
    if (!condition)
    {
        return nullptr;
    }
    return std::make_shared<RepeatUntilNode>(body, condition);
}

Expr FormulaParser::sequence()
{
    skip_separators();
    consume_imports();

    if (is_block_end())
    {
        return nullptr;
    }

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
        consume_imports();

        // Check if we've reached end of input
        if (is_block_end())
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

bool FormulaParser::is_builtin_var() const
{
    return std::find(BUILTIN_VARS.begin(), BUILTIN_VARS.end(), m_curr.type) != BUILTIN_VARS.end();
}

bool FormulaParser::is_builtin_fn() const
{
    return std::find(BUILTIN_FNS.begin(), BUILTIN_FNS.end(), m_curr.type) != BUILTIN_FNS.end();
}

bool FormulaParser::is_assignable() const
{
    return check(TokenType::IDENTIFIER) //
        || (m_options.allow_builtin_assignment && (is_builtin_var() || is_builtin_fn()));
}

Expr FormulaParser::statement()
{
    if (is_extended() && check_context("static"))
    {
        advance();
        return function_declaration(true);
    }
    if (is_extended() && check(TokenType::FUNC))
    {
        return function_declaration();
    }
    if (is_extended() && is_type_start())
    {
        begin_tracking();
        const Token curr{m_curr};
        advance();
        const bool typed_function = check(TokenType::FUNC);
        backtrack();
        m_curr = curr;
        if (typed_function)
        {
            return function_declaration();
        }
    }
    if (check(TokenType::IF))
    {
        return if_statement();
    }
    if (is_extended() && check(TokenType::WHILE))
    {
        return while_statement();
    }
    if (is_extended() && check(TokenType::REPEAT))
    {
        return repeat_statement();
    }
    if (is_extended() && check_context("return"))
    {
        return return_statement();
    }
    if (is_extended() && is_type_start())
    {
        begin_tracking();
        const Token curr{m_curr};
        const std::string type{str()};
        advance();
        bool declaration = check(TokenType::IDENTIFIER);
        if (declaration)
        {
            advance();
            declaration = !check(TokenType::OPEN_PAREN);
        }
        backtrack();
        m_curr = curr;
        if (declaration || type == "bool" || type == "int" || type == "float" || type == "complex" || type == "color")
        {
            return declaration_statement();
        }
    }
    return assignment_statement();
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
        error(ErrorCode::EXPECTED_ENDIF);
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

    // Parse condition
    Expr condition = conjunctive();
    if (!condition)
    {
        // conjunctive() will have already recorded the error
        return nullptr;
    }

    // Accept comma or newline after condition
    if (!skip_separators())
    {
        error(ErrorCode::EXPECTED_STATEMENT_SEPARATOR);
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
            // if_statement_no_endif will have already recorded the error
            return nullptr;
        }
    }
    else if (match(TokenType::ELSE))
    {
        if (!skip_separators())
        {
            error(ErrorCode::EXPECTED_STATEMENT_SEPARATOR);
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

    while (!is_block_end())
    {
        consume_imports();
        if (is_block_end())
        {
            break;
        }
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
            skip_separators();
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

Expr FormulaParser::variable()
{
    if (check(TokenType::IDENTIFIER))
    {
        return identifier();
    }

    if (Expr var = builtin_var())
    {
        warning(ErrorCode::BUILTIN_VARIABLE_ASSIGNMENT);
        return var;
    }

    if (Expr var = builtin_fn())
    {
        warning(ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT);
        return var;
    }

    return nullptr;
}

Expr FormulaParser::assignment_statement()
{
    Expr left = conjunctive();
    if (!left)
    {
        return nullptr;
    }

    if (!match(TokenType::ASSIGN))
    {
        return left;
    }

    if (const auto *id = dynamic_cast<const IdentifierNode *>(left.get()); id && !is_user_identifier(left))
    {
        if (m_errors.empty())
        {
            error(ErrorCode::EXPECTED_STATEMENT);
        }
        return nullptr;
    }
    if (!dynamic_cast<const IdentifierNode *>(left.get()) && !dynamic_cast<const IndexNode *>(left.get()) &&
        !dynamic_cast<const MemberAccessNode *>(left.get()) && !dynamic_cast<const ParameterRefNode *>(left.get()) &&
        !dynamic_cast<const ConstantRefNode *>(left.get()))
    {
        error(ErrorCode::EXPECTED_STATEMENT);
        return nullptr;
    }

    Expr right = assignment_statement();
    if (!right)
    {
        return nullptr;
    }
    return std::make_shared<AssignmentNode>(left, right);
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
            // comparative already recorded the error
            return nullptr;
        }

        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

// Handle relational operators: <, <=, >, >=, ==, !=
Expr FormulaParser::comparative()
{
    Expr left = additive();

    while (left &&
        check({TokenType::LESS_THAN, TokenType::LESS_EQUAL,    //
            TokenType::GREATER_THAN, TokenType::GREATER_EQUAL, //
            TokenType::EQUAL, TokenType::NOT_EQUAL}))
    {
        std::string op;
        if (check(TokenType::LESS_THAN))
        {
            op = "<";
        }
        else if (check(TokenType::LESS_EQUAL))
        {
            op = "<=";
        }
        else if (check(TokenType::GREATER_THAN))
        {
            op = ">";
        }
        else if (check(TokenType::GREATER_EQUAL))
        {
            op = ">=";
        }
        else if (check(TokenType::EQUAL))
        {
            op = "==";
        }
        else if (check(TokenType::NOT_EQUAL))
        {
            op = "!=";
        }

        advance();
        if (Expr right = additive())
        {
            left = std::make_shared<BinaryOpNode>(left, op, right);
            continue;
        }

        // additive already recorded the error
        return nullptr;
    }

    return left;
}

Expr FormulaParser::additive()
{
    Expr left = term();

    while (left && check({TokenType::PLUS, TokenType::MINUS}))
    {
        char op = check(TokenType::PLUS) ? '+' : '-';
        advance(); // consume operator
        Expr right = term();
        if (!right)
        {
            // term already recorded the error
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

Expr FormulaParser::term()
{
    Expr left = unary();

    while (left && check({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::PERCENT}))
    {
        char op = check(TokenType::MULTIPLY) ? '*' : (check(TokenType::DIVIDE) ? '/' : '%');
        advance(); // consume operator
        Expr right = unary();
        if (!right)
        {
            // unary already recorded the error
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, op, right);
    }

    return left;
}

Expr FormulaParser::unary()
{
    if (check({TokenType::PLUS, TokenType::MINUS, TokenType::NOT}))
    {
        char op = check(TokenType::PLUS) ? '+' : (check(TokenType::MINUS) ? '-' : '!');
        advance();              // consume operator
        Expr operand = unary(); // Allow chaining: --1
        if (!operand)
        {
            // unary already recorded the error
            return nullptr;
        }
        return std::make_shared<UnaryOpNode>(op, operand);
    }

    return power();
}

Expr FormulaParser::power()
{
    Expr left = postfix();

    // Left-associative: parse from left to right using a loop
    while (left && check(TokenType::POWER))
    {
        advance(); // consume '^'
        Expr right = postfix();
        if (!right)
        {
            // primary already recorded the error
            return nullptr;
        }
        left = std::make_shared<BinaryOpNode>(left, '^', right);
    }

    return left;
}

Expr FormulaParser::postfix()
{
    Expr left = primary();
    while (left)
    {
        if (check(TokenType::OPEN_PAREN))
        {
            const auto error_count = m_errors.size();
            std::vector<Expr> args = argument_list();
            if (m_errors.size() != error_count)
            {
                return nullptr;
            }
            if (const auto *id = dynamic_cast<const IdentifierNode *>(left.get()); id)
            {
                left = std::make_shared<FunctionCallNode>(id->name(), std::move(args));
                continue;
            }
            if (const auto *member = dynamic_cast<const MemberAccessNode *>(left.get()); member)
            {
                left = std::make_shared<FunctionCallNode>(member->member(), std::move(args));
                continue;
            }
            error(ErrorCode::EXPECTED_IDENTIFIER);
            return nullptr;
        }
        if (match(TokenType::OPEN_BRACKET))
        {
            std::vector<Expr> indices;
            if (!check(TokenType::CLOSE_BRACKET))
            {
                do
                {
                    Expr index = expression();
                    if (!index)
                    {
                        return nullptr;
                    }
                    indices.push_back(index);
                } while (match(TokenType::COMMA));
            }
            if (!match(TokenType::CLOSE_BRACKET))
            {
                error(ErrorCode::EXPECTED_CLOSE_PAREN);
                return nullptr;
            }
            left = std::make_shared<IndexNode>(left, std::move(indices));
            continue;
        }
        if (match(TokenType::DOT))
        {
            if (!check(TokenType::IDENTIFIER))
            {
                error(ErrorCode::EXPECTED_IDENTIFIER);
                return nullptr;
            }
            const std::string member{str()};
            advance();
            left = std::make_shared<MemberAccessNode>(left, member);
            continue;
        }
        break;
    }
    return left;
}

Expr FormulaParser::builtin_var()
{
    if (is_builtin_var())
    {
        const std::string name{str()};
        advance(); // consume the builtin variable
        return std::make_shared<IdentifierNode>(name);
    }

    // not an error
    return nullptr;
}

Expr FormulaParser::builtin_fn()
{
    if (is_builtin_fn())
    {
        const std::string name{str()};
        advance();
        return std::make_shared<IdentifierNode>(name);
    }

    // not an error
    return nullptr;
}

std::optional<Expr> FormulaParser::builtin_function()
{
    if (const auto it = std::find(BUILTIN_FNS.begin(), BUILTIN_FNS.end(), m_curr.type); it != BUILTIN_FNS.end())
    {
        begin_tracking();
        const Token curr{m_curr};
        const std::string name{str()};
        advance(); // consume the function name
        const auto error_count = m_errors.size();
        std::optional args{function_call()};
        if (m_errors.size() != error_count)
        {
            end_tracking();
            return Expr{};
        }
        if (args.has_value())
        {
            end_tracking();
            return std::make_shared<FunctionCallNode>(name, args.value());
        }
        backtrack();
        m_curr = curr;
        return {};
    }
    // not an error
    return {};
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

                    if (check(TokenType::CLOSE_PAREN))
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

std::optional<std::vector<Expr>> FormulaParser::function_call()
{
    if (check(TokenType::OPEN_PAREN))
    {
        return argument_list();
    }
    return {};
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
    const auto make_identifier = [this]
    {
        const std::string name{str()};
        advance(); // consume the identifier
        return std::make_shared<IdentifierNode>(name);
    };
    if (check(TokenType::IDENTIFIER))
    {
        return make_identifier();
    }

    // TODO: Also allow some reserved words as identifiers in expression context
    // only allow true and false to be identifiers in legacy mode
    if (m_options.dialect != Dialect::EXTENDED && check({TokenType::LIT_TRUE, TokenType::LIT_FALSE}))
    {
        return make_identifier();
    }

    if (std::find(BUILTIN_FNS.begin(), BUILTIN_FNS.end(), m_curr.type) != BUILTIN_FNS.end())
    {
        if (m_options.allow_builtin_assignment)
        {
            warning(ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT);
            return make_identifier();
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
        error(ErrorCode::EXPECTED_PRIMARY);
        return nullptr;
    }

    if (check(TokenType::ASSIGN))
    {
        // Error: Assignment operator found in invalid context
        error(ErrorCode::UNEXPECTED_ASSIGNMENT);
        return nullptr;
    }

    if (Expr result = number())
    {
        return result;
    }

    if (is_extended() && check({TokenType::LIT_TRUE, TokenType::LIT_FALSE}))
    {
        const bool value{check(TokenType::LIT_TRUE)};
        advance();
        return std::make_shared<LiteralNode>(value);
    }

    if (is_extended() && check(TokenType::QUOTED_STRING))
    {
        Expr result = std::make_shared<LiteralNode>(str());
        advance();
        return result;
    }

    if (is_extended() && check(TokenType::PARAMETER_IDENTIFIER))
    {
        Expr result = std::make_shared<ParameterRefNode>(str());
        advance();
        return result;
    }

    if (is_extended() && check(TokenType::CONSTANT_IDENTIFIER))
    {
        Expr result = std::make_shared<ConstantRefNode>(str());
        advance();
        return result;
    }

    if (is_extended() && match_context("new"))
    {
        std::string type;
        if (check({TokenType::IDENTIFIER, TokenType::TYPE_IDENTIFIER, TokenType::PARAMETER_IDENTIFIER}))
        {
            type = str();
            advance();
        }
        else
        {
            error(ErrorCode::EXPECTED_IDENTIFIER);
            return nullptr;
        }
        std::vector<Expr> args;
        if (check(TokenType::OPEN_PAREN))
        {
            const auto error_count = m_errors.size();
            args = argument_list();
            if (m_errors.size() != error_count)
            {
                return nullptr;
            }
        }
        return std::make_shared<NewNode>(type, std::move(args));
    }

    if (std::optional<Expr> result = builtin_function())
    {
        if (result.value())
        {
            return result.value();
        }
        return nullptr;
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

    if (check(TokenType::OPEN_PAREN))
    {
        advance();
        if (Expr expr = complex_literal())
        {
            return expr;
        }

        // Allow full expressions including assignment in parens
        if (Expr expr = expression())
        {
            if (check(TokenType::CLOSE_PAREN))
            {
                advance(); // consume ')'
                return expr;
            }
            error(ErrorCode::EXPECTED_CLOSE_PAREN);
            return nullptr;
        }

        // conjunctive already recorded the error
        return nullptr;
    }

    // Handle modulus operator |expr|
    if (check(TokenType::MODULUS))
    {
        advance();
        if (Expr expr = expression())
        {
            if (check(TokenType::MODULUS))
            {
                advance(); // consume closing '|'
                return std::make_shared<UnaryOpNode>('|', expr);
            }
            error(ErrorCode::EXPECTED_CLOSE_MODULUS);
            return nullptr; // missing closing '|'
        }
        // conjunctive already recorded the error
        return nullptr;
    }

    error(ErrorCode::EXPECTED_PRIMARY);
    return nullptr;
}

} // namespace

ParserPtr create_parser(std::string_view text, const Options &options)
{
    return std::make_shared<FormulaParser>(text, options);
}

#define ERROR_CODE_CASE(name_) \
    case ErrorCode::name_:     \
        return #name_

std::string to_string(ErrorCode code)
{
    switch (code)
    {
        ERROR_CODE_CASE(NONE);
        ERROR_CODE_CASE(INVALID_TOKEN);
        ERROR_CODE_CASE(EXPECTED_PRIMARY);
        ERROR_CODE_CASE(EXPECTED_ENDIF);
        ERROR_CODE_CASE(EXPECTED_STATEMENT_SEPARATOR);
        ERROR_CODE_CASE(EXPECTED_COMMA);
        ERROR_CODE_CASE(EXPECTED_OPEN_PAREN);
        ERROR_CODE_CASE(EXPECTED_CLOSE_PAREN);
        ERROR_CODE_CASE(EXPECTED_CLOSE_MODULUS);
        ERROR_CODE_CASE(EXPECTED_IDENTIFIER);
        ERROR_CODE_CASE(EXPECTED_ASSIGNMENT);
        ERROR_CODE_CASE(EXPECTED_INTEGER);
        ERROR_CODE_CASE(EXPECTED_FLOATING_POINT);
        ERROR_CODE_CASE(EXPECTED_COMPLEX);
        ERROR_CODE_CASE(EXPECTED_STRING);
        ERROR_CODE_CASE(EXPECTED_TERMINATOR);
        ERROR_CODE_CASE(EXPECTED_STATEMENT);
        ERROR_CODE_CASE(UNEXPECTED_ASSIGNMENT);
        ERROR_CODE_CASE(BUILTIN_VARIABLE_ASSIGNMENT);
        ERROR_CODE_CASE(BUILTIN_FUNCTION_ASSIGNMENT);
        ERROR_CODE_CASE(INVALID_SECTION);
        ERROR_CODE_CASE(INVALID_SECTION_ORDER);
        ERROR_CODE_CASE(DUPLICATE_SECTION);
        ERROR_CODE_CASE(BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS);
        ERROR_CODE_CASE(BUILTIN_SECTION_INVALID_KEY);
        ERROR_CODE_CASE(BUILTIN_SECTION_INVALID_TYPE);
        ERROR_CODE_CASE(DEFAULT_SECTION_INVALID_KEY);
        ERROR_CODE_CASE(DEFAULT_SECTION_INVALID_METHOD);
        ERROR_CODE_CASE(SWITCH_SECTION_INVALID_KEY);
    }

    return std::to_string(static_cast<int>(code));
}

#undef ERROR_CODE_CASE

} // namespace formula::parser
