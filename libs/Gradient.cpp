// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Gradient.h>

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace gradient
{

namespace
{

enum class TokenType
{
    END_OF_FILE,
    IDENTIFIER,
    INTEGER,
    QUOTED_STRING,
    EQUALS,
    LEFT_BRACE,
    RIGHT_BRACE,
    COLON,
};

struct Token
{
    TokenType type{TokenType::END_OF_FILE};
    std::string value;
};

class Lexer
{
public:
    explicit Lexer(std::string_view text);

    Token next();
    Token peek();

private:
    void skip_whitespace_and_comments();
    Token scan_quoted_string();
    Token scan_integer();
    Token scan_identifier();

    std::string_view m_text;
    std::size_t m_pos{0};
};

Lexer::Lexer(std::string_view text) :
    m_text(text)
{
}

Token Lexer::next()
{
    skip_whitespace_and_comments();

    if (m_pos >= m_text.size())
    {
        return {TokenType::END_OF_FILE, {}};
    }

    const char ch = m_text[m_pos];

    if (ch == '=')
    {
        ++m_pos;
        return {TokenType::EQUALS, "="};
    }
    if (ch == '{')
    {
        ++m_pos;
        return {TokenType::LEFT_BRACE, "{"};
    }
    if (ch == '}')
    {
        ++m_pos;
        return {TokenType::RIGHT_BRACE, "}"};
    }
    if (ch == '"')
    {
        return scan_quoted_string();
    }
    if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)))
    {
        return scan_integer();
    }
    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_')
    {
        return scan_identifier();
    }

    throw std::runtime_error(std::string("unexpected character: ") + ch);
}

Token Lexer::peek()
{
    const std::size_t saved = m_pos;
    Token t = next();
    m_pos = saved;
    return t;
}

void Lexer::skip_whitespace_and_comments()
{
    while (m_pos < m_text.size())
    {
        const char ch = m_text[m_pos];
        if (std::isspace(static_cast<unsigned char>(ch)))
        {
            ++m_pos;
        }
        else if (ch == ';')
        {
            while (m_pos < m_text.size() && m_text[m_pos] != '\n')
            {
                ++m_pos;
            }
        }
        else
        {
            break;
        }
    }
}

Token Lexer::scan_quoted_string()
{
    ++m_pos;
    std::string value;
    while (m_pos < m_text.size() && m_text[m_pos] != '"')
    {
        value += m_text[m_pos];
        ++m_pos;
    }
    if (m_pos >= m_text.size())
    {
        throw std::runtime_error("unterminated quoted string");
    }
    ++m_pos;
    return {TokenType::QUOTED_STRING, std::move(value)};
}

Token Lexer::scan_integer()
{
    std::string value;
    if (m_text[m_pos] == '-')
    {
        value += '-';
        ++m_pos;
    }
    if (m_pos >= m_text.size() || !std::isdigit(static_cast<unsigned char>(m_text[m_pos])))
    {
        throw std::runtime_error("expected digit");
    }
    while (m_pos < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_pos])))
    {
        value += m_text[m_pos];
        ++m_pos;
    }
    return {TokenType::INTEGER, std::move(value)};
}

Token Lexer::scan_identifier()
{
    std::string value;
    while (m_pos < m_text.size())
    {
        const char ch = m_text[m_pos];
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-')
        {
            value += ch;
            ++m_pos;
        }
        else
        {
            break;
        }
    }
    if (m_pos < m_text.size() && m_text[m_pos] == ':')
    {
        ++m_pos;
        return {TokenType::COLON, std::move(value)};
    }
    return {TokenType::IDENTIFIER, std::move(value)};
}

void expect_token(const Token &token, TokenType type, const char *description)
{
    if (token.type != type)
    {
        throw std::runtime_error(std::string("expected ") + description);
    }
}

Token expect_next(Lexer &tokens, TokenType type, const char *description)
{
    Token token = tokens.next();
    expect_token(token, type, description);
    return token;
}

int parse_int(const Token &token)
{
    expect_token(token, TokenType::INTEGER, "integer");
    return std::stoi(token.value);
}

bool parse_bool(const Token &token)
{
    expect_token(token, TokenType::IDENTIFIER, "yes or no");
    if (token.value == "yes")
    {
        return true;
    }
    if (token.value == "no")
    {
        return false;
    }
    throw std::runtime_error("expected yes or no");
}

Token parse_key(Lexer &tokens)
{
    Token key = expect_next(tokens, TokenType::IDENTIFIER, "key");
    expect_next(tokens, TokenType::EQUALS, "=");
    return key;
}

RGBColor unpack_bgr(int packed)
{
    return {static_cast<std::uint8_t>((packed >> 16) & 0xFF), static_cast<std::uint8_t>((packed >> 8) & 0xFF),
        static_cast<std::uint8_t>(packed & 0xFF)};
}

bool is_section_end(const Token &token)
{
    return token.type == TokenType::COLON || token.type == TokenType::RIGHT_BRACE ||
        token.type == TokenType::END_OF_FILE;
}

void expect_num_nodes(int expected_num_nodes, std::size_t point_count)
{
    if (expected_num_nodes >= 0 && point_count != static_cast<std::size_t>(expected_num_nodes))
    {
        throw std::runtime_error("numnodes does not match control point count");
    }
}

int parse_num_nodes(const Token &token)
{
    const int num_nodes = parse_int(token);
    if (num_nodes < 0)
    {
        throw std::runtime_error("numnodes must be nonnegative");
    }
    return num_nodes;
}

void parse_gradient_key_value(Lexer &tokens, GradientSection &section, int &expected_num_nodes)
{
    const Token key = parse_key(tokens);
    if (key.value == "title")
    {
        section.title = expect_next(tokens, TokenType::QUOTED_STRING, "quoted string").value;
    }
    else if (key.value == "smooth")
    {
        section.smooth = parse_bool(tokens.next());
    }
    else if (key.value == "numnodes")
    {
        expected_num_nodes = parse_num_nodes(tokens.next());
        if (!section.points.empty())
        {
            throw std::runtime_error("numnodes must precede control points");
        }
        section.points.reserve(static_cast<std::size_t>(expected_num_nodes));
    }
    else if (key.value == "rotation")
    {
        section.rotation = parse_int(tokens.next());
    }
    else if (key.value == "linked")
    {
        section.linked = parse_bool(tokens.next());
    }
    else if (key.value == "index")
    {
        ColorControlPoint point;
        point.index = parse_int(tokens.next());
        Token value_key = parse_key(tokens);
        if (value_key.value != "color")
        {
            throw std::runtime_error("expected color");
        }
        point.color = unpack_bgr(parse_int(tokens.next()));
        section.points.push_back(point);
    }
    else
    {
        throw std::runtime_error("unexpected gradient key: " + key.value);
    }
}

void parse_opacity_key_value(
    Lexer &tokens, OpacitySection &section, int &expected_num_nodes, std::string_view value_key_name)
{
    const Token key = parse_key(tokens);
    if (key.value == "smooth")
    {
        section.smooth = parse_bool(tokens.next());
    }
    else if (key.value == "numnodes")
    {
        expected_num_nodes = parse_num_nodes(tokens.next());
        if (!section.points.empty())
        {
            throw std::runtime_error("numnodes must precede control points");
        }
        section.points.reserve(static_cast<std::size_t>(expected_num_nodes));
    }
    else if (key.value == "rotation")
    {
        section.rotation = parse_int(tokens.next());
    }
    else if (key.value == "linked")
    {
        section.linked = parse_bool(tokens.next());
    }
    else if (key.value == "index")
    {
        OpacityControlPoint point;
        point.index = parse_int(tokens.next());
        Token value_key = parse_key(tokens);
        if (value_key.value != value_key_name)
        {
            throw std::runtime_error("expected " + std::string{value_key_name});
        }
        point.opacity = static_cast<std::uint8_t>(parse_int(tokens.next()));
        section.points.push_back(point);
    }
    else
    {
        throw std::runtime_error("unexpected opacity key: " + key.value);
    }
}

GradientSection parse_gradient_section(Lexer &tokens)
{
    GradientSection section;
    int expected_num_nodes{-1};
    while (!is_section_end(tokens.peek()))
    {
        parse_gradient_key_value(tokens, section, expected_num_nodes);
    }
    expect_num_nodes(expected_num_nodes, section.points.size());
    return section;
}

OpacitySection parse_opacity_section(Lexer &tokens, std::string_view value_key_name)
{
    OpacitySection section;
    int expected_num_nodes{-1};
    while (!is_section_end(tokens.peek()))
    {
        parse_opacity_key_value(tokens, section, expected_num_nodes, value_key_name);
    }
    expect_num_nodes(expected_num_nodes, section.points.size());
    return section;
}

} // namespace

GradientEntry parse_gradient(const formula::FileEntry &file_entry)
{
    Lexer tokens{file_entry.body};
    GradientEntry entry;
    entry.name = file_entry.name;
    bool has_gradient{};
    bool has_opacity{};

    while (tokens.peek().type != TokenType::END_OF_FILE)
    {
        Token header = expect_next(tokens, TokenType::COLON, "section header");
        if (header.value == "gradient")
        {
            entry.gradient = parse_gradient_section(tokens);
            has_gradient = true;
        }
        else if (header.value == "opacity")
        {
            entry.opacity = parse_opacity_section(tokens, "opacity");
            has_opacity = true;
        }
        else if (header.value == "alpha")
        {
            entry.opacity = parse_opacity_section(tokens, "alpha");
            has_opacity = true;
        }
        else
        {
            throw std::runtime_error("unexpected section: " + header.value);
        }
    }

    if (!has_gradient || !has_opacity)
    {
        throw std::runtime_error("entry requires gradient and opacity sections");
    }
    return entry;
}

} // namespace gradient
