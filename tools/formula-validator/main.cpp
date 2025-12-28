// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//

#include <formula/Formula.h>
#include <formula/FormulaEntry.h>
#include <formula/ParseOptions.h>
#include <formula/Parser.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace formula;
using namespace formula::parser;

namespace formula
{

inline std::ostream &operator<<(std::ostream &os, const SourceLocation &loc)
{
    return os << loc.line << ':' << loc.column;
}

} // namespace formula

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
        for (const FormulaEntry &entry : load_formula_entries(str))
        {
            ParserPtr parser{create_parser(entry.body, Options{})};
            if (!parser)
            {
                std::cout << "Error: " << file.filename().string() << '(' << entry.name
                          << "): couldn't create parser\n";
                ++bad;
                continue;
            }
            if (parser->parse())
            {
                ++good;
            }
            else
            {
                std::vector<std::size_t> line_starts;
                line_starts.push_back(0);
                std::vector<std::size_t> line_lengths;
                std::size_t line{1U};
                for (size_t i = 0; i < entry.body.length(); i++)
                {
                    if (entry.body[i] == '\n')
                    {
                        line_lengths.push_back(line_lengths.empty() ? i : i - line_starts[line - 1]);
                        line_starts.push_back(i + 1);
                        ++line;
                    }
                }
                line_lengths.push_back(entry.body.length() - line_starts.back());
                const std::string filename{file.filename().string()};
                const auto emit_diag = [&entry, &filename, &line_starts, &line_lengths](
                                           const Diagnostic &diag, const char *level)
                {
                    std::cout << filename << ": " << entry.name << '(' << diag.position << "): " //
                              << level << ": " << to_string(diag.code) << '\n';
                    std::cout << "    "
                              << entry.body.substr(
                                     line_starts[diag.position.line - 1], line_lengths[diag.position.line - 1])
                              << '\n';
                    std::cout << "   " << std::string(diag.position.column, ' ') << "^\n";
                };
                for (const auto &diag : parser->get_warnings())
                {
                    emit_diag(diag, "Warning");
                }
                for (const auto &diag : parser->get_errors())
                {
                    emit_diag(diag, "Error");
                }
                std::cout << "Error: " << filename << '(' << entry.name << "): couldn't parse body\n";
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
