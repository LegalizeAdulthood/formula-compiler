// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <cstddef>
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
    bool is_class{};
};

struct ClassHeader
{
    std::string name;
    std::string base_file;
    std::string base_class;
    std::size_t entry_index{};
};

struct FormulaFile
{
    std::string filename;
    std::vector<FormulaEntry> entries;
    std::vector<ClassHeader> classes;
};

std::vector<FormulaEntry> load_formula_entries(std::istream &in);
FormulaFile load_formula_file(std::istream &in, std::string filename = {});

} // namespace formula
