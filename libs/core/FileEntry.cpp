// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/core/FileEntry.h>

#include <cctype>
#include <istream>
#include <iterator>
#include <optional>
#include <utility>

namespace formula
{

namespace
{

void strip_leading(std::string &text)
{
    if (const auto first_non_space = text.find_first_not_of(" \t\r\n"); first_non_space != 0)
    {
        text.erase(0, first_non_space);
    }
}

void strip_trailing(std::string &text)
{
    if (const auto last_non_space = text.find_last_not_of(" \t\r\n"); last_non_space != std::string::npos)
    {
        text.erase(last_non_space + 1);
    }
    else
    {
        text.clear();
    }
}

class Scanner
{
public:
    Scanner(std::string text, std::string filename) :
        m_text(std::move(text))
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
        result.header_range.begin = m_location;
        result.source_range.begin = m_location;
        std::string header{scan_header()};
        result.header_range.end = m_location;
        parse_header(result, std::move(header));

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

    std::string scan_header()
    {
        std::string result;
        while (!at_end() && current_char() != '{')
        {
            if (current_char() == ';')
            {
                result.append(1, ' ');
                skip_comment();
                continue;
            }
            result.append(1, current_char());
            advance();
        }
        strip_leading(result);
        strip_trailing(result);
        return result;
    }

    void parse_header(FileEntry &entry, std::string header) const
    {
        strip_trailing_bracket(header, entry.bracket_value);
        strip_trailing_paren(header, entry.paren_value);
        strip_trailing(header);
        entry.name = std::move(header);
        entry.name_range = entry.header_range;
    }

    void strip_trailing_bracket(std::string &text, std::string &value) const
    {
        strip_trailing(text);
        if (text.empty() || text.back() != ']')
        {
            return;
        }
        const auto open_bracket{text.find_last_of('[')};
        if (open_bracket == std::string::npos)
        {
            return;
        }
        value = text.substr(open_bracket + 1, text.length() - open_bracket - 2);
        text.erase(open_bracket);
    }

    void strip_trailing_paren(std::string &text, std::string &value) const
    {
        strip_trailing(text);
        if (text.empty() || text.back() != ')')
        {
            return;
        }
        const auto open_paren{text.find_last_of('(')};
        if (open_paren == std::string::npos)
        {
            return;
        }
        value = text.substr(open_paren + 1, text.length() - open_paren - 2);
        text.erase(open_paren);
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

    std::string m_text;
    size_t m_position{};
    SourceLocation m_location;
};

} // namespace

std::vector<FileEntry> load_file_entries(std::istream &in, std::string filename)
{
    std::string text{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    Scanner scanner{std::move(text), std::move(filename)};
    return scanner.entries();
}

} // namespace formula
