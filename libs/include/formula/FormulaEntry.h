// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace formula
{

struct FormulaEntry
{
    std::string name;
    std::string paren_value;
    std::string bracket_value;
    std::string body;
};

std::vector<FormulaEntry> load_formula_entries(std::istream &in);

} // namespace formula
