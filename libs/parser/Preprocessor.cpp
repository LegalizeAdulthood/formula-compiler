// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/parser/Preprocessor.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace formula::preprocessor
{

namespace
{

struct Line
{
    std::string_view text;
    std::string_view newline;
};

struct Conditional
{
    bool parent_active{};
    bool condition_matched{};
    bool active{};
    bool saw_else{};
};

std::string lower(std::string_view text)
{
    std::string result{text};
    std::transform(result.begin(), result.end(), result.begin(),
        [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
    return result;
}

bool is_identifier_start(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_identifier_continue(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_symbol(std::string_view text)
{
    if (text.empty() || !is_identifier_start(text.front()))
    {
        return false;
    }
    return std::all_of(text.begin() + 1, text.end(), is_identifier_continue);
}

bool active(const std::vector<Conditional> &conditionals)
{
    return conditionals.empty() || conditionals.back().active;
}

std::string_view trim_left(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
    {
        text.remove_prefix(1);
    }
    return text;
}

std::string_view trim(std::string_view text)
{
    text = trim_left(text);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t'))
    {
        text.remove_suffix(1);
    }
    return text;
}

std::string first_word(std::string_view &text)
{
    text = trim_left(text);
    const std::size_t begin{};
    std::size_t end{};
    while (end < text.size() && text[end] != ' ' && text[end] != '\t')
    {
        ++end;
    }
    std::string word{text.substr(begin, end)};
    text.remove_prefix(end);
    text = trim_left(text);
    return word;
}

Line next_line(std::string_view input, std::size_t &pos)
{
    const std::size_t start{pos};
    pos = input.find_first_of("\r\n", pos);
    if (pos == std::string_view::npos)
    {
        pos = input.size();
    }
    const std::string_view text = input.substr(start, pos - start);
    if (pos >= input.size())
    {
        return {text, {}};
    }

    const std::size_t newline_start{pos};
    if (input[pos] == '\r' && pos + 1 < input.size() && input[pos + 1] == '\n')
    {
        pos += 2;
    }
    else
    {
        ++pos;
    }
    return {text, input.substr(newline_start, pos - newline_start)};
}

void add_error(std::vector<Diagnostic> &errors, ErrorCode code, std::size_t line, std::string_view filename)
{
    errors.push_back(Diagnostic{code, SourceLocation{line, 1, std::string{filename}}});
}

bool contains_macro(const MacroList &macros, std::string_view macro)
{
    const std::string name{macro};
    const auto it = std::lower_bound(macros.begin(), macros.end(), name);
    return it != macros.end() && *it == name;
}

void define_macro(MacroList &macros, std::string_view macro)
{
    const std::string name{macro};
    const auto it = std::lower_bound(macros.begin(), macros.end(), name);
    if (it == macros.end() || *it != name)
    {
        macros.insert(it, name);
    }
}

void undef_macro(MacroList &macros, std::string_view macro)
{
    const std::string name{macro};
    const auto it = std::lower_bound(macros.begin(), macros.end(), name);
    if (it != macros.end() && *it == name)
    {
        macros.erase(it);
    }
}

MacroList normalize_macros(const MacroList &macros)
{
    MacroList result;
    for (const auto &name : macros)
    {
        define_macro(result, lower(name));
    }
    return result;
}

} // namespace

MacroList predefined_macros(UltraFractalMacros predefined)
{
    switch (predefined)
    {
    case UltraFractalMacros::NONE:
        return {};
    case UltraFractalMacros::ULTRAFRACTAL3:
        return {"ULTRAFRACTAL", "VER20", "VER30"};
    case UltraFractalMacros::ULTRAFRACTAL4:
        return {"ULTRAFRACTAL", "VER20", "VER30", "VER40"};
    case UltraFractalMacros::ULTRAFRACTAL5:
        return {"ULTRAFRACTAL", "VER20", "VER30", "VER40", "VER50"};
    case UltraFractalMacros::ULTRAFRACTAL6:
        return {"ULTRAFRACTAL", "VER20", "VER30", "VER40", "VER50", "VER60"};
    }
    return {};
}

Preprocessor::Preprocessor() :
    Preprocessor{UltraFractalMacros::NONE}
{
}

Preprocessor::Preprocessor(const MacroList &predefined) :
    Preprocessor{predefined, {}}
{
}

Preprocessor::Preprocessor(std::string source_filename) :
    Preprocessor{UltraFractalMacros::NONE, std::move(source_filename)}
{
}

Preprocessor::Preprocessor(const MacroList &predefined, std::string source_filename) :
    m_predefined{normalize_macros(predefined)},
    m_macros{m_predefined},
    m_source_filename{std::move(source_filename)}
{
}

Preprocessor::Preprocessor(UltraFractalMacros predefined) :
    Preprocessor{predefined_macros(predefined)}
{
}

Preprocessor::Preprocessor(UltraFractalMacros predefined, std::string source_filename) :
    Preprocessor{predefined_macros(predefined), std::move(source_filename)}
{
}

std::string Preprocessor::process(std::string_view input)
{
    m_macros = m_predefined;
    m_errors.clear();
    std::string output;
    std::vector<Conditional> conditionals;
    std::size_t pos{};
    std::size_t line_number{1};

    while (pos < input.size())
    {
        const Line line = next_line(input, pos);
        std::string_view body = trim_left(line.text);

        if (!body.empty() && body.front() == '$')
        {
            body.remove_prefix(1);
            const std::string directive = lower(first_word(body));
            const bool parent_active = conditionals.empty() || conditionals.back().active;

            if (directive == "define" || directive == "undef" || directive == "ifdef")
            {
                const std::string symbol = lower(first_word(body));
                if (!is_symbol(symbol) || !trim(body).empty())
                {
                    add_error(m_errors, ErrorCode::EXPECTED_DIRECTIVE_SYMBOL, line_number, m_source_filename);
                }
                else if (directive == "define")
                {
                    if (active(conditionals))
                    {
                        define_macro(m_macros, symbol);
                    }
                }
                else if (directive == "undef")
                {
                    if (active(conditionals))
                    {
                        undef_macro(m_macros, symbol);
                    }
                }
                else
                {
                    const bool enabled = active(conditionals) && contains_macro(m_macros, symbol);
                    conditionals.push_back(Conditional{active(conditionals), enabled, enabled, false});
                }
            }
            else if (directive == "else")
            {
                if (!trim(body).empty())
                {
                    add_error(m_errors, ErrorCode::INVALID_COMPILER_DIRECTIVE, line_number, m_source_filename);
                }
                else if (conditionals.empty())
                {
                    add_error(m_errors, ErrorCode::UNEXPECTED_DIRECTIVE_ELSE, line_number, m_source_filename);
                }
                else if (conditionals.back().saw_else)
                {
                    add_error(m_errors, ErrorCode::DUPLICATE_DIRECTIVE_ELSE, line_number, m_source_filename);
                }
                else
                {
                    Conditional &top = conditionals.back();
                    top.saw_else = true;
                    top.active = top.parent_active && !top.condition_matched;
                    top.condition_matched = true;
                }
            }
            else if (directive == "endif")
            {
                if (!trim(body).empty())
                {
                    add_error(m_errors, ErrorCode::INVALID_COMPILER_DIRECTIVE, line_number, m_source_filename);
                }
                else if (conditionals.empty())
                {
                    add_error(m_errors, ErrorCode::UNEXPECTED_DIRECTIVE_ENDIF, line_number, m_source_filename);
                }
                else
                {
                    conditionals.pop_back();
                }
            }
            else
            {
                add_error(m_errors, ErrorCode::INVALID_COMPILER_DIRECTIVE, line_number, m_source_filename);
            }

            ++line_number;
            continue;
        }
        if (active(conditionals))
        {
            output += line.text;
        }
        else
        {
            ++line_number;
            continue;
        }

        output += line.newline;
        ++line_number;
    }

    if (!conditionals.empty())
    {
        add_error(m_errors, ErrorCode::EXPECTED_DIRECTIVE_ENDIF, line_number, m_source_filename);
    }

    return output;
}

#define ERROR_CODE_CASE(name_) \
    case ErrorCode::name_:     \
        return #name_

std::string to_string(ErrorCode code)
{
    switch (code)
    {
        ERROR_CODE_CASE(NONE);
        ERROR_CODE_CASE(EXPECTED_DIRECTIVE_ENDIF);
        ERROR_CODE_CASE(UNEXPECTED_DIRECTIVE_ELSE);
        ERROR_CODE_CASE(UNEXPECTED_DIRECTIVE_ENDIF);
        ERROR_CODE_CASE(DUPLICATE_DIRECTIVE_ELSE);
        ERROR_CODE_CASE(EXPECTED_DIRECTIVE_SYMBOL);
        ERROR_CODE_CASE(INVALID_COMPILER_DIRECTIVE);
    }
    return std::to_string(static_cast<int>(code));
}

#undef ERROR_CODE_CASE

} // namespace formula::preprocessor
