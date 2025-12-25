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
            formulas.push_back({name, "", "", body});
        }
    }

    return formulas;
}

} // namespace formula
