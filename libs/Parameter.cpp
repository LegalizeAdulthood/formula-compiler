// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Parameter.h>

#include <iterator>
#include <optional>
#include <string>
#include <utility>

namespace formula::parameter
{

#define PARSE_ERROR_CASE(name_) \
    case ParseErrorCode::name_: \
        return #name_

std::string to_string(ParseErrorCode code)
{
    switch (code)
    {
        PARSE_ERROR_CASE(NONE);
        PARSE_ERROR_CASE(EXPECTED_SECTION_LABEL);
        PARSE_ERROR_CASE(EXPECTED_ASSIGNMENT);
        PARSE_ERROR_CASE(EXPECTED_VALUE);
        PARSE_ERROR_CASE(UNTERMINATED_QUOTED_STRING);
    }
    return "ParseErrorCode(" + std::to_string(static_cast<int>(code)) + ")";
}

namespace
{

struct ProcessedLine
{
    std::string text;
    std::size_t line{};
};

struct CommentStrippedLine
{
    std::string text;
    bool in_quote{};
};

struct ParsedValue
{
    std::string text;
    std::size_t end{};
};

bool is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

std::size_t skip_space(std::string_view text, std::size_t pos)
{
    while (pos < text.length() && is_space(text[pos]))
    {
        ++pos;
    }
    return pos;
}

std::string_view trim(std::string_view text)
{
    while (!text.empty() && is_space(text.front()))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_space(text.back()))
    {
        text.remove_suffix(1);
    }
    return text;
}

bool ends_with_continuation(std::string_view text)
{
    text = trim(text);
    return !text.empty() && text.back() == '\\';
}

CommentStrippedLine strip_comment(std::string_view line, bool in_quote)
{
    std::string result;
    bool escaped{};
    for (char ch : line)
    {
        if (escaped)
        {
            result.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\' && in_quote)
        {
            result.push_back(ch);
            escaped = true;
            continue;
        }
        if (ch == '"')
        {
            in_quote = !in_quote;
            result.push_back(ch);
            continue;
        }
        if (ch == ';' && !in_quote)
        {
            break;
        }
        result.push_back(ch);
    }
    return {std::move(result), in_quote};
}

std::vector<std::string> split_physical_lines(std::string_view input)
{
    std::vector<std::string> result;
    std::string line;
    for (std::size_t i = 0; i < input.length();)
    {
        const char ch{input[i]};
        if (ch == '\r' || ch == '\n')
        {
            result.push_back(std::move(line));
            line.clear();
            ++i;
            if (ch == '\r' && i < input.length() && input[i] == '\n')
            {
                ++i;
            }
            continue;
        }
        line.push_back(ch);
        ++i;
    }
    if (!line.empty() || input.empty())
    {
        result.push_back(std::move(line));
    }
    return result;
}

std::vector<ProcessedLine> preprocess_lines(std::string_view input)
{
    std::vector<ProcessedLine> result;
    std::string current;
    std::size_t current_line{};
    bool continuing{};
    bool quote_continues{};
    const std::vector<std::string> physical_lines{split_physical_lines(input)};

    for (std::size_t i = 0; i < physical_lines.size(); ++i)
    {
        if (!continuing)
        {
            current_line = i + 1;
        }

        CommentStrippedLine stripped{strip_comment(physical_lines[i], quote_continues)};
        std::string piece{std::move(stripped.text)};
        if (continuing)
        {
            piece.erase(0, piece.find_first_not_of(" \t"));
        }
        current.append(piece);

        if (ends_with_continuation(current))
        {
            current.erase(current.find_last_not_of(" \t") + 1);
            current.pop_back();
            continuing = true;
            quote_continues = stripped.in_quote;
            continue;
        }

        result.push_back({std::move(current), current_line});
        current.clear();
        continuing = false;
        quote_continues = false;
    }

    if (!current.empty())
    {
        result.push_back({std::move(current), current_line});
    }

    return result;
}

SourceLocation location_at(SourceLocation start, std::size_t line, std::size_t column)
{
    SourceLocation result{std::move(start)};
    if (line > 1)
    {
        result.line += line - 1;
        result.column = column;
    }
    else
    {
        result.column += column - 1;
    }
    return result;
}

bool is_section_label(std::string_view line)
{
    line = trim(line);
    if (line.empty() || line.back() != ':')
    {
        return false;
    }
    line.remove_suffix(1);
    return !line.empty() && line.find_first_of(" \t=") == std::string_view::npos;
}

std::string section_name(std::string_view line)
{
    line = trim(line);
    line.remove_suffix(1);
    return std::string{line};
}

std::optional<ParsedValue> parse_quoted_value(std::string_view line, std::size_t value_start)
{
    std::string value;
    bool escaped{};
    for (std::size_t pos = value_start + 1; pos < line.length(); ++pos)
    {
        const char ch{line[pos]};
        if (escaped)
        {
            if (ch == '"' || ch == '\\')
            {
                value.push_back(ch);
            }
            else
            {
                value.push_back('\\');
                value.push_back(ch);
            }
            escaped = false;
            continue;
        }
        if (ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (ch == '"')
        {
            return ParsedValue{std::move(value), pos + 1};
        }
        value.push_back(ch);
    }
    return std::nullopt;
}

class BodyParser
{
public:
    BodyParser(std::string_view input, SourceLocation source_location) :
        m_lines(preprocess_lines(input)),
        m_source_location(std::move(source_location))
    {
    }

    BasicParameterEntry parse_basic()
    {
        BasicParameterEntry result;
        for (const ProcessedLine &line : m_lines)
        {
            parse_basic_line(result, line);
        }
        result.diagnostics = std::move(m_diagnostics);
        return result;
    }

    ExtendedParameterEntry parse_extended()
    {
        ExtendedParameterEntry result;
        for (const ProcessedLine &line : m_lines)
        {
            parse_extended_line(result, line);
        }
        result.diagnostics = std::move(m_diagnostics);
        return result;
    }

private:
    void parse_basic_line(BasicParameterEntry &entry, const ProcessedLine &processed)
    {
        const std::string_view line{processed.text};
        const std::string_view stripped{trim(line)};
        if (stripped.empty())
        {
            return;
        }

        const std::size_t first_non_space{line.find_first_not_of(" \t")};
        if (is_section_label(stripped))
        {
            error(ParseErrorCode::EXPECTED_ASSIGNMENT, processed.line, first_non_space + 1);
            return;
        }

        std::vector<Parameter> assignments{parse_assignments(line, processed.line)};
        entry.assignments.insert(entry.assignments.end(), std::make_move_iterator(assignments.begin()),
            std::make_move_iterator(assignments.end()));
    }

    void parse_extended_line(ExtendedParameterEntry &entry, const ProcessedLine &processed)
    {
        const std::string_view line{processed.text};
        const std::string_view stripped{trim(line)};
        if (stripped.empty())
        {
            return;
        }

        const std::size_t first_non_space{line.find_first_not_of(" \t")};
        if (is_section_label(stripped))
        {
            m_current_section = &entry.sections.emplace_back();
            m_current_section->name = section_name(stripped);
            return;
        }

        if (m_current_section == nullptr)
        {
            error(ParseErrorCode::EXPECTED_SECTION_LABEL, processed.line, first_non_space + 1);
            return;
        }

        std::vector<Parameter> assignments{parse_assignments(line, processed.line)};
        m_current_section->assignments.insert(m_current_section->assignments.end(),
            std::make_move_iterator(assignments.begin()), std::make_move_iterator(assignments.end()));
    }

    std::vector<Parameter> parse_assignments(std::string_view line, std::size_t line_number)
    {
        std::vector<Parameter> result;
        std::size_t pos{};
        while (pos < line.length())
        {
            pos = skip_space(line, pos);
            if (pos >= line.length())
            {
                break;
            }

            const std::size_t assignment_start{pos};
            while (pos < line.length() && line[pos] != '=' && !is_space(line[pos]))
            {
                ++pos;
            }
            const std::size_t key_end{pos};
            if (assignment_start == key_end)
            {
                error(ParseErrorCode::EXPECTED_ASSIGNMENT, line_number, assignment_start + 1);
                break;
            }

            if (pos >= line.length() || line[pos] != '=')
            {
                error(ParseErrorCode::EXPECTED_ASSIGNMENT, line_number, assignment_start + 1);
                break;
            }
            ++pos;
            const std::size_t value_start{pos};
            if (value_start >= line.length() || is_space(line[value_start]))
            {
                error(ParseErrorCode::EXPECTED_VALUE, line_number, value_start + 1);
                break;
            }

            Parameter assignment;
            assignment.key = std::string{line.substr(assignment_start, key_end - assignment_start)};

            if (line[value_start] == '"')
            {
                std::optional<ParsedValue> value{parse_quoted_value(line, value_start)};
                if (!value)
                {
                    error(ParseErrorCode::UNTERMINATED_QUOTED_STRING, line_number, value_start + 1);
                    break;
                }
                assignment.value = std::move(value->text);
                pos = value->end;
            }
            else
            {
                while (pos < line.length() && !is_space(line[pos]))
                {
                    ++pos;
                }
                assignment.value = std::string{line.substr(value_start, pos - value_start)};
            }

            result.push_back(std::move(assignment));
        }
        return result;
    }

    void error(ParseErrorCode code, std::size_t line, std::size_t column)
    {
        m_diagnostics.push_back(ParseDiagnostic{code, location_at(m_source_location, line, column)});
    }

    std::vector<ProcessedLine> m_lines;
    SourceLocation m_source_location;
    ParameterSection *m_current_section{};
    std::vector<ParseDiagnostic> m_diagnostics;
};

} // namespace

BasicParameterEntry parse_basic_parameters(FileEntry file_entry)
{
    return BodyParser{file_entry.body, file_entry.body_range.begin}.parse_basic();
}

ExtendedParameterEntry parse_extended_parameters(FileEntry file_entry)
{
    return BodyParser{file_entry.body, file_entry.body_range.begin}.parse_extended();
}

} // namespace formula::parameter
