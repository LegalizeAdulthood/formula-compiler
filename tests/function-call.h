// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <array>
#include <iostream>

namespace formula::test
{

struct FunctionCallParam
{
    std::string_view expr;
    double real;
    formula::Complex result;
};

inline std::ostream &operator<<(std::ostream &str, const FunctionCallParam &value)
{
    return str << value.expr << '=' << value.real << '/' << value.result;
}

extern std::array<FunctionCallParam, 37> g_calls;

} // namespace formula::test
