// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/FormulaEntry.h>

#include <cassert>

namespace formula
{

static void strip_trailing(std::string &text)
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

std::vector<FormulaEntry> load_formula_entries(std::istream &in)
{
    std::vector<FormulaEntry> formulas;
    std::string line;
    while (std::getline(in, line))
    {
        const auto open_brace{line.find_last_of("{")};
        if (open_brace == std::string::npos)
        {
            continue;
        }
        // was the opening brace commented out?
        if (const auto semi{line.find_first_of(";")}; semi < open_brace)
        {
            continue;
        }

        std::string name{line};
        name.erase(open_brace);
        strip_trailing(name);

        std::string bracket_value;
        if (const auto close_bracket = name.find_last_of(']'); close_bracket != std::string::npos)
        {
            if (const auto open_bracket = name.find_last_of('[', close_bracket); open_bracket != std::string::npos)
            {
                bracket_value = name.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                name.erase(open_bracket, close_bracket - open_bracket + 1);
                strip_trailing(name);
            }
            else
            {
                // TODO: throw exception?
                assert(false);
            }
        }

        std::string paren_value;
        if (const auto close_paren = name.find_last_of(')'); close_paren != std::string::npos)
        {
            if (const auto open_paren = name.find_last_of('(', close_paren); open_paren != std::string::npos)
            {
                paren_value = name.substr(open_paren + 1, close_paren - open_paren - 1);
                name.erase(open_paren, close_paren - open_paren + 1);
                strip_trailing(name);
            }
            else
            {
                // TODO: throw exception?
                assert(false);
            }
        }

        // skip entries with no name or where the name is "comment".
        if (name.empty() || name == "comment")
        {
            while (line.find("}") == std::string::npos)
            {
                if (!std::getline(in, line))
                {
                    break;
                }
            }
            continue;
        }

        std::string body;
        line.erase(0, open_brace + 1);

        // Check if the closing brace is on the same line
        if (const auto brace = line.find("}"); brace != std::string::npos)
        {
            // Single-line entry - don't append newline
            line.erase(brace);
            body.append(line);
            formulas.push_back({name, paren_value, bracket_value, body});
            continue;
        }

        // Multi-line entry
        const auto accum = [&body, &line]() {
            body.append(line);
            body.append(1, '\n');
        };
        accum();
        bool found_brace{};
        while (std::getline(in, line))
        {
            const auto semi{line.find_first_of(";")};
            if (const auto brace = line.find("}");
                brace != std::string::npos && (semi == std::string::npos || semi > brace))
            {
                found_brace = true;
                line.erase(brace);
                accum();
                break;
            }
            accum();
        }
        if (found_brace)
        {
            formulas.push_back({name, paren_value, bracket_value, body});
        }
    }

    return formulas;
}

} // namespace formula
