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

namespace parser
{

std::string to_string(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::NONE:
        return "NONE";
    case ErrorCode::BUILTIN_VARIABLE_ASSIGNMENT:
        return "BUILTIN_VARIABLE_ASSIGNMENT";
    case ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT:
        return "BUILTIN_FUNCTION_ASSIGNMENT";
    case ErrorCode::EXPECTED_PRIMARY:
        return "EXPECTED_PRIMARY";
    case ErrorCode::INVALID_TOKEN:
        return "INVALID_TOKEN";
    case ErrorCode::INVALID_SECTION:
        return "INVALID_SECTION";
    case ErrorCode::INVALID_SECTION_ORDER:
        return "INVALID_SECTION_ORDER";
    case ErrorCode::DUPLICATE_SECTION:
        return "DUPLICATE_SECTION";
    case ErrorCode::INVALID_DEFAULT_METHOD:
        return "INVALID_DEFAULT_METHOD";
    case ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS:
        return "BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS";
    case ErrorCode::EXPECTED_ENDIF:
        return "EXPECTED_ENDIF";
    case ErrorCode::EXPECTED_STATEMENT_SEPARATOR:
        return "EXPECTED_STATEMENT_SEPARATOR";
    case ErrorCode::BUILTIN_SECTION_INVALID_TYPE:
        return "BUILTIN_SECTION_INVALID_TYPE";
    case ErrorCode::EXPECTED_OPEN_PAREN:
        return "EXPECTED_OPEN_PAREN";
    }

    return std::to_string(static_cast<int>(code));
}

} // namespace parser

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
                std::string filename{file.filename().string()};
                const auto emit_diag = [&entry, &filename, &line_starts, &line_lengths](
                                           const Diagnostic &diag, const char *level)
                {
                    std::cout << filename << '(' << diag.position << "): " //
                              << entry.name << ": " << level << ": " << to_string(diag.code) << '\n';
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
