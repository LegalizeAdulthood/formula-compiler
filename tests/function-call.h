#pragma once

#include <array>
#include <iostream>

struct FunctionCallParam
{
    std::string_view expr;
    double result;
};

inline std::ostream &operator<<(std::ostream &str, const FunctionCallParam &value)
{
    return str << value.expr << '=' << value.result;
}

extern std::array<FunctionCallParam, 37> s_calls;
