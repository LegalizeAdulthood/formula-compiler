// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>
#include <formula/FormulaEntry.h>
#include <formula/ParseOptions.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace formula;

namespace
{

int main(const std::vector<std::string_view> &args)
{
    std::vector<std::filesystem::path> files;
    for (size_t i = 1; i < args.size(); ++i)
    {
        files.emplace_back(args[i]);
    }

    int total_good{};
    int total_bad{};
    for (const std::filesystem::path &file : files)
    {
        std::ifstream str{file};
        if (!str)
        {
            std::cout << "Error: failed to open file " << file << '\n';
            continue;
        }
        int good{};
        int bad{};
        for (const auto &entry : load_formula_entries(str))
        {
            if (FormulaPtr formula{create_formula(entry.body, ParseOptions{})})
            {
                ++good;
            }
            else
            {
                std::cout << "Error: " << file.stem().string() << '(' << entry.name << "): couldn't parse body\n";
                ++bad;
            }
        }
        std::cout << file.stem().string() << ": " << good << " good, " << bad << " bad\n";
        total_good += good;
        total_bad += bad;
    }
    std::cout << "Total: " << total_good << " good, " << total_bad << " bad\n";

    return total_bad != 0;
}

} // namespace

int main(int argc, char *argv[])
{
    std::vector<std::string_view> args;
    for (int i = 0; i < argc; ++i)
    {
        args.emplace_back(argv[i]);
    }
    return main(args);
}
