// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Header for accessing formula data generated from id.frm

#pragma once

#include <cstddef>
#include <string_view>

namespace formula::test
{

struct FormulaEntry
{
    std::string_view name;
    std::string_view content;
};

extern const FormulaEntry formulas[];
extern const size_t formula_count;

} // namespace formula::test
