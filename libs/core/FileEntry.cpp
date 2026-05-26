// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/core/FileEntry.h>

#include <cctype>
#include <istream>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

namespace formula
{

namespace
{

class Scanner
{
public:
    Scanner(std::string_view text, std::string filename) :
        m_text(text)
    {
        m_location.filename = std::move(filename);
    }

    std::vector<FileEntry> entries()
    {
        std::vector<FileEntry> result;
        while (std::optional<FileEntry> entry = next_entry())
        {
            result.push_back(std::move(*entry));
        }
        return result;
    }

private:
    std::optional<FileEntry> next_entry()
    {
        skip_whitespace_and_comments();
        if (at_end())
        {
            return std::nullopt;
        }

        FileEntry result;
        result.source_range.begin = m_location;
        const std::size_t header_begin_position{m_position};
        const SourceLocation header_begin_location{m_location};
        scan_header();
        const std::size_t header_end_position{m_position};
        parse_header(result, header_begin_position, header_begin_location, header_end_position);

        if (at_end())
        {
            return std::nullopt;
        }

        advance();
        result.body_range.begin = m_location;
        const size_t body_start{m_position};
        const SourceLocation body_end{scan_body()};
        if (at_end())
        {
            return std::nullopt;
        }

        result.body = m_text.substr(body_start, m_position - body_start);
        result.body_range.end = body_end;
        advance();
        result.source_range.end = m_location;
        return result;
    }

    void scan_header()
    {
        while (!at_end() && current_char() != '{')
        {
            if (current_char() == ';')
            {
                skip_comment();
                continue;
            }
            advance();
        }
    }

    void parse_header(FileEntry &entry, std::size_t header_begin_position, SourceLocation header_begin_location,
        std::size_t header_end_position) const
    {
        const std::size_t first{trim_begin(header_begin_position, header_end_position)};
        if (first == header_end_position)
        {
            entry.header_range = {m_location, m_location};
            entry.name_range = entry.header_range;
            return;
        }

        const std::size_t header_end{trim_end(first, header_end_position)};
        entry.header_range = {location_at(header_begin_position, header_begin_location, first),
            location_at(header_begin_position, header_begin_location, header_end)};
        std::size_t name_end{header_end};
        strip_trailing_bracket(first, name_end, entry.bracket_value);
        strip_trailing_paren(first, name_end, entry.paren_value);
        name_end = trim_end(first, name_end);
        if (name_end <= first)
        {
            entry.name_range = {location_at(header_begin_position, header_begin_location, first),
                location_at(header_begin_position, header_begin_location, first)};
            return;
        }

        entry.name = m_text.substr(first, name_end - first);
        entry.name_range = {location_at(header_begin_position, header_begin_location, first),
            location_at(header_begin_position, header_begin_location, name_end)};
    }

    void strip_trailing_bracket(std::size_t begin, std::size_t &end, std::string &value) const
    {
        end = trim_end(begin, end);
        if (end <= begin || m_text[end - 1] != ']')
        {
            return;
        }
        const std::size_t open_bracket{m_text.find_last_of('[', end - 1)};
        if (open_bracket == std::string::npos || open_bracket < begin)
        {
            return;
        }
        value = m_text.substr(open_bracket + 1, end - open_bracket - 2);
        end = open_bracket;
    }

    void strip_trailing_paren(std::size_t begin, std::size_t &end, std::string &value) const
    {
        end = trim_end(begin, end);
        if (end <= begin || m_text[end - 1] != ')')
        {
            return;
        }
        const std::size_t open_paren{m_text.find_last_of('(', end - 1)};
        if (open_paren == std::string::npos || open_paren < begin)
        {
            return;
        }
        value = m_text.substr(open_paren + 1, end - open_paren - 2);
        end = open_paren;
    }

    std::size_t trim_begin(std::size_t begin, std::size_t end) const
    {
        while (begin < end)
        {
            if (m_text[begin] == ';')
            {
                begin = skip_comment(begin, end);
                continue;
            }
            if (!is_space(m_text[begin]))
            {
                return begin;
            }
            begin = next_position(begin);
        }
        return begin;
    }

    std::size_t trim_end(std::size_t begin, std::size_t end) const
    {
        std::size_t last{begin};
        std::size_t position{begin};
        while (position < end)
        {
            if (m_text[position] == ';')
            {
                position = skip_comment(position, end);
                continue;
            }
            const std::size_t next{next_position(position)};
            if (!is_space(m_text[position]))
            {
                last = next;
            }
            position = next;
        }
        return last;
    }

    SourceLocation location_at(
        std::size_t header_begin_position, SourceLocation header_begin_location, std::size_t position) const
    {
        SourceLocation result{header_begin_location};
        std::size_t pos{header_begin_position};
        while (pos < position && pos < m_text.size())
        {
            if (m_text[pos] == '\r')
            {
                ++pos;
                if (pos < position && pos < m_text.size() && m_text[pos] == '\n')
                {
                    ++pos;
                }
                ++result.line;
                result.column = 1;
                continue;
            }
            if (m_text[pos] == '\n')
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

    std::size_t next_position(std::size_t position) const
    {
        if (position < m_text.size() && m_text[position] == '\r' && position + 1 < m_text.size() &&
            m_text[position + 1] == '\n')
        {
            return position + 2;
        }
        return position + 1;
    }

    std::size_t skip_comment(std::size_t position, std::size_t end) const
    {
        while (position < end && m_text[position] != '\n' && m_text[position] != '\r')
        {
            ++position;
        }
        return position;
    }

    SourceLocation scan_body()
    {
        while (!at_end())
        {
            if (current_char() == ';')
            {
                skip_comment();
                continue;
            }
            if (current_char() == '"')
            {
                skip_string();
                continue;
            }
            if (current_char() == '}')
            {
                return m_location;
            }
            advance();
        }
        return m_location;
    }

    void skip_string()
    {
        advance();
        while (!at_end())
        {
            if (current_char() == '\\')
            {
                advance();
                if (!at_end())
                {
                    advance();
                }
                continue;
            }
            if (current_char() == '"')
            {
                advance();
                return;
            }
            advance();
        }
    }

    void skip_whitespace_and_comments()
    {
        while (!at_end())
        {
            if (is_space(current_char()))
            {
                advance();
                continue;
            }
            if (current_char() == ';')
            {
                skip_comment();
                continue;
            }
            break;
        }
    }

    void skip_comment()
    {
        while (!at_end() && current_char() != '\n' && current_char() != '\r')
        {
            advance();
        }
    }

    bool at_end() const
    {
        return m_position >= m_text.length();
    }

    char current_char() const
    {
        if (m_position < m_text.length())
        {
            return m_text[m_position];
        }
        return '\0';
    }

    bool is_space(char ch) const
    {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    }

    void advance()
    {
        if (at_end())
        {
            return;
        }

        const char ch{m_text[m_position]};
        if (ch == '\r')
        {
            ++m_position;
            if (!at_end() && current_char() == '\n')
            {
                ++m_position;
            }
            ++m_location.line;
            m_location.column = 1;
            return;
        }
        if (ch == '\n')
        {
            ++m_position;
            ++m_location.line;
            m_location.column = 1;
            return;
        }

        ++m_position;
        ++m_location.column;
    }

    std::string_view m_text;
    size_t m_position{};
    SourceLocation m_location;
};

} // namespace

std::vector<FileEntry> load_file_entries(std::string_view text, std::string filename)
{
    Scanner scanner{text, std::move(filename)};
    return scanner.entries();
}

std::vector<FileEntry> load_file_entries(std::istream &in, std::string filename)
{
    std::string text{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    return load_file_entries(std::string_view{text}, std::move(filename));
}

} // namespace formula
