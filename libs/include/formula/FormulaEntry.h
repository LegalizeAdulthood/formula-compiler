// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>
#include <formula/SourceLocation.h>

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

struct FormulaImportDirective
{
    std::string filename;
    SourceLocation location;
    bool implicit{};
};

struct FormulaEntryImports
{
    std::size_t entry_index{};
    std::vector<FormulaImportDirective> imports;
};

struct FormulaFile
{
    std::string filename;
    std::vector<FormulaEntry> entries;
    std::vector<ClassHeader> classes;
    std::vector<FormulaEntryImports> entry_imports;
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
    SourceLocation location;
    std::string detail;
    std::vector<std::string> import_stack;
};

struct FormulaFileReference
{
    std::string filename;
    std::size_t file_index{};
};

struct FormulaClassReference
{
    std::string filename;
    std::string class_name;
    std::size_t file_index{};
    std::size_t class_index{};
    std::size_t entry_index{};
};

struct FormulaFileSet
{
    std::vector<FormulaFile> files;
    std::vector<FormulaFileReference> file_index;
    std::vector<FormulaClassReference> class_index;
    std::vector<FormulaFileDiagnostic> diagnostics;
};

using FormulaFileImporter = std::function<std::string(std::string_view)>;

std::vector<FormulaEntry> load_formula_entries(std::istream &in);
FormulaFile load_formula_file(std::istream &in, std::string filename = {});
FormulaFileSet load_formula_file_tree(std::string_view root_filename, const FormulaFileImporter &importer);
ast::FormulaSectionsPtr parse_formula_class(FormulaFileSet &files, const FormulaClassReference &klass);

} // namespace formula
