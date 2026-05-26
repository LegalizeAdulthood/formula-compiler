// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/core/Section.h>

#include <cctype>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace formula
{

namespace
{

bool is_name_char(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '-';
}

std::size_t line_end_position(std::string_view body, std::size_t position)
{
    while (position < body.size() && body[position] != '\r' && body[position] != '\n')
    {
        ++position;
    }
    return position;
}

std::size_t next_line_position(std::string_view body, std::size_t line_end)
{
    if (line_end >= body.size())
    {
        return line_end;
    }
    if (body[line_end] == '\r' && line_end + 1 < body.size() && body[line_end + 1] == '\n')
    {
        return line_end + 2;
    }
    return line_end + 1;
}

SourceLocation location_at(std::string_view body, SourceLocation body_begin, std::size_t position)
{
    SourceLocation result{std::move(body_begin)};
    std::size_t pos{};
    while (pos < position && pos < body.size())
    {
        if (body[pos] == '\r')
        {
            ++pos;
            if (pos < position && pos < body.size() && body[pos] == '\n')
            {
                ++pos;
            }
            ++result.line;
            result.column = 1;
            continue;
        }
        if (body[pos] == '\n')
        {
            ++pos;
            ++result.line;
            result.column = 1;
            continue;
        }
        ++pos;
        ++result.column;
    }
    return result;
}

std::optional<std::pair<std::size_t, std::size_t>> section_name_range(std::string_view body, std::size_t line_start)
{
    const std::size_t line_end{line_end_position(body, line_start)};
    std::size_t name_start{line_start};
    while (name_start < line_end && (body[name_start] == ' ' || body[name_start] == '\t'))
    {
        ++name_start;
    }

    std::size_t name_end{name_start};
    while (name_end < line_end && is_name_char(body[name_end]))
    {
        ++name_end;
    }
    if (name_end == name_start || name_end >= line_end || body[name_end] != ':')
    {
        return std::nullopt;
    }
    for (std::size_t position{name_end + 1}; position < line_end; ++position)
    {
        if (body[position] != ' ' && body[position] != '\t')
        {
            return std::nullopt;
        }
    }
    return std::pair{name_start, name_end};
}

} // namespace

std::vector<Section> split_sections(std::string_view body, SourceLocation body_begin)
{
    std::vector<Section> result;
    Section *current{};
    std::size_t position{};
    while (position < body.size())
    {
        const std::size_t line_start{position};
        const std::size_t line_end{line_end_position(body, line_start)};
        const std::size_t next_line{next_line_position(body, line_end)};
        if (const std::optional<std::pair<std::size_t, std::size_t>> name_range{section_name_range(body, line_start)})
        {
            if (current != nullptr)
            {
                current->body = body.substr(static_cast<std::size_t>(current->body.data() - body.data()),
                    line_start - static_cast<std::size_t>(current->body.data() - body.data()));
                current->body_range.end = location_at(body, body_begin, line_start);
            }

            Section section;
            section.name = body.substr(name_range->first, name_range->second - name_range->first);
            section.body = body.substr(next_line, 0);
            section.name_range = {
                location_at(body, body_begin, name_range->first), location_at(body, body_begin, name_range->second)};
            section.body_range = {location_at(body, body_begin, next_line), location_at(body, body_begin, next_line)};
            result.push_back(section);
            current = &result.back();
        }
        position = next_line;
    }

    if (current != nullptr)
    {
        const std::size_t body_start{static_cast<std::size_t>(current->body.data() - body.data())};
        current->body = body.substr(body_start);
        current->body_range.end = location_at(body, body_begin, body.size());
    }
    return result;
}

} // namespace formula
