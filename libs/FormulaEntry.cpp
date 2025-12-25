// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/FormulaEntry.h>

namespace formula
{

std::vector<FormulaEntry> load_formula_entries(std::istream &in)
{
    std::vector<FormulaEntry> formulas;
    std::string line;
    while (std::getline(in, line))
    {
        const auto open_brace{line.find("{")};
        if (open_brace == std::string::npos)
        {
            continue;
        }

        std::string name{line};
        if (const auto end = name.find_first_of(" \t{"); end != std::string::npos)
        {
            name.erase(end);
        }

        std::string bracket_value;
        if (const auto close_bracket = name.find_last_of(']'); close_bracket != std::string::npos)
        {
            if (const auto open_bracket = name.find_last_of('[', close_bracket); open_bracket != std::string::npos)
            {
                bracket_value = name.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                name.erase(open_bracket, close_bracket - open_bracket + 1);
            }
            else
            {
                // TODO: throw exception?
            }
        }

        std::string paren_value;
        if (const auto close_paren = name.find_last_of(')'); close_paren != std::string::npos)
        {
            if (const auto open_paren = name.find_last_of('(', close_paren); open_paren != std::string::npos)
            {
                paren_value = name.substr(open_paren + 1, close_paren - open_paren - 1);
                name.erase(open_paren, close_paren - open_paren + 1);
            }
            else
            {
                // TODO: throw exception?
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
        const auto accum = [&body, &line]() {
            body.append(line);
            body.append(1, '\n');
        };
        accum();
        bool found_brace{};
        while (std::getline(in, line))
        {
            if (const auto brace = line.find("}"); brace != std::string::npos)
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
