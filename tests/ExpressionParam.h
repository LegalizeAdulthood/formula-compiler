// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Formula.h>

#include <array>
#include <string>

namespace formula::test
{

struct ExpressionParam
{
    std::string_view name;
    std::string_view text;
    Section section;
    double expected_re;
    double expected_im;
};

inline void PrintTo(const ExpressionParam &param, std::ostream *os)
{
    *os << param.name;
}

extern std::array<ExpressionParam, 99> g_expression_params;

} // namespace formula::test
