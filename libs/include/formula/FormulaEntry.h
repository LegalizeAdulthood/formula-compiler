// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
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

enum class FormulaFileDiagnosticCode
{
    MISSING_IMPORT,
    IMPORT_CYCLE,
    PREPROCESS_ERROR,
    PARSE_ERROR,
};

struct FormulaFileDiagnostic
{
    FormulaFileDiagnosticCode code{};
    std::string filename;
    std::vector<std::string> import_stack;
};

struct FormulaFileSet
{
    std::vector<FormulaFile> files;
    std::vector<FormulaFileDiagnostic> diagnostics;
};

using FormulaFileImporter = std::function<std::string(std::string_view)>;

std::vector<FormulaEntry> load_formula_entries(std::istream &in);
FormulaFile load_formula_file(std::istream &in, std::string filename = {});
FormulaFileSet load_formula_file_tree(std::string_view root_filename, const FormulaFileImporter &importer);

} // namespace formula
