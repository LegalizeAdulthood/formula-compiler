#include <formula/formula.h>

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace
{

int main(const std::vector<std::string_view> &args)
{
    bool compile{};
    std::map<std::string, double> values;
    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i] == "--compile")
        {
            compile = true;
        }
        else if (auto pos = args[i].find('='); pos != std::string_view::npos)
        {
            std::string name{args[i].substr(0, pos)};
            std::string value{args[i].substr(pos + 1)};
            values[name] = std::stod(value);
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
    std::shared_ptr<formula::Formula> formula = formula::parse(line);
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

    std::cout << "Evaluated: " << (compile ? formula->run() : formula->interpret()) << '\n';
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
