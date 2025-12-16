// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Tool to parse id.frm and generate C++ source files with formula data

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct FormulaEntry
{
    std::string name;
    std::string body;
};

std::string escape_for_cpp(const std::string &text)
{
    std::string result;
    result.reserve(text.size() * 2);

    for (size_t i = 0; i < text.size(); ++i)
    {
        char ch = text[i];
        switch (ch)
        {
        case '\\':
            result += "\\\\";
            break;
        case '"':
            result += "\\\"";
            break;
        case '\n':
            result.append(R"(\n")").append(1, '\n').append(8, ' ').append(1, '"');
            break;
        case '\r':
            // Skip carriage returns
            break;
        default:
            result += ch;
            break;
        }
    }

    return result;
}

std::string trim(const std::string &str)
{
    const auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); });
    const auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base();

    return (start < end) ? std::string(start, end) : std::string();
}

std::vector<FormulaEntry> parse_formulas(const std::string &filename)
{
    std::vector<FormulaEntry> formulas;
    std::ifstream file(filename, std::ios::binary);

    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << '\n';
        return formulas;
    }

    std::string line;
    while (std::getline(file, line))
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
                if (!std::getline(file, line))
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
        while (std::getline(file, line))
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
            formulas.push_back({name, body});
        }
    }

    return formulas;
}

bool generate_header(const std::string &filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Error: Could not create file " << filename << '\n';
        return false;
    }

    file << //
R"(// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Auto-generated file from id.frm - DO NOT EDIT MANUALLY
#pragma once

#include "FormulaEntry.h"

#include <cstdint>

namespace formula::test
{

extern const FormulaEntry g_formulas[];
extern const std::size_t g_formula_count;

} // namespace formula::test
)";

    return true;
}

bool generate_source(const std::string &filename, const std::vector<FormulaEntry> &formulas)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Error: Could not create file " << filename << '\n';
        return false;
    }

    file << R"(// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Auto-generated file from id.frm - DO NOT EDIT MANUALLY
#include "formula-data.h"

namespace formula::test
{

// clang-format off
const FormulaEntry g_formulas[] =
{
)";

    for (size_t i = 0; i < formulas.size(); ++i)
    {
        const auto &entry = formulas[i];
        file << "    {\"" << entry.name << "\", \"" << escape_for_cpp(entry.body) << "\"}";
        if (i < formulas.size() - 1)
        {
            file << ",";
        }
        file << "\n";
    }

    file << R"(};
// clang-format on

const std::size_t g_formula_count = )"
         << formulas.size() << R"(;

} // namespace formula::test
)";

    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <input.frm> <output.h> <output.cpp>\n";
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string header_file = argv[2];
    const std::string source_file = argv[3];

    std::cout << "Parsing " << input_file << "...\n";
    const std::vector formulas{parse_formulas(input_file)};
    std::cout << "Found " << formulas.size() << " formulas\n";

    if (formulas.empty())
    {
        std::cerr << "Error: No formulas found in " << input_file << '\n';
        return 1;
    }

    std::cout << "Generating " << header_file << "...\n";
    if (!generate_header(header_file))
    {
        return 1;
    }

    std::cout << "Generating " << source_file << "...\n";
    if (!generate_source(source_file, formulas))
    {
        return 1;
    }

    std::cout << "Successfully generated formula data files with " << formulas.size() << " entries\n";
    return 0;
}
