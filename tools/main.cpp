// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace formula;

namespace
{

int main(const std::vector<std::string_view> &args)
{
    bool compile{};
    std::map<std::string, Complex> values;
    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i] == "--compile")
        {
            compile = true;
        }
        else if (auto pos = args[i].find('='); pos != std::string_view::npos)
        {
            const std::string name{args[i].substr(0, pos)};
            const std::string value{args[i].substr(pos + 1)};
            double re{std::stod(value)};
            double im{};
            if (const auto comma = value.find(','); comma != std::string_view::npos)
            {
                im = std::stod(value.substr(comma + 1));
            }
            values[name] = {re, im};
        }
        else
        {
            std::cerr << "Usage: " << args[0] << " [--assemble | --compile] [name=value] ... [name=value]\n";
            return 1;
        }
    }

    std::cout << "Enter an expression:\n";
    std::string line;
    std::getline(std::cin, line);
    FormulaPtr formula = parse(line);
    if (!formula)
    {
        std::cerr << "Error: Invalid formula\n";
        return 1;
    }
    for (const auto &[name, value] : values)
    {
        formula->set_value(name, value);
    }

    if (compile && !formula->compile())
    {
        std::cerr << "Error: Failed to compile formula\n";
        return 1;
    }

    std::cout << "Evaluated: " << (compile ? formula->run((Section::ITERATE)) : formula->interpret((Section::ITERATE))) << '\n';
    return 0;
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
