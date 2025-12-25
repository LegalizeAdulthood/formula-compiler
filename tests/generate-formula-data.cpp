// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Tool to parse id.frm and generate C++ source files with formula data

#include "formula/FormulaEntry.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace formula;

static std::string escape_for_cpp(const std::string &text)
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

static std::vector<FormulaEntry> parse_formulas(const std::string &filename)
{
    std::ifstream in(filename);

    if (!in)
    {
        std::cerr << "Error: Could not open file " << filename << '\n';
        return {};
    }

    return load_formula_entries(in);
}

static bool generate_header(const std::string &filename)
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

#include <formula/FormulaEntry.h>

#include <cstdint>

namespace formula::test
{

extern const FormulaEntry g_formulas[];
extern const std::size_t g_formula_count;

} // namespace formula::test
)";

    return true;
}

static bool generate_source(const std::string &filename, const std::vector<FormulaEntry> &formulas)
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
        file << "    {\"" << entry.name << R"(", "", "", ")" << escape_for_cpp(entry.body) << "\"}";
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
