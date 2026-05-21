// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/Node.h>
#include <formula/SourceLocation.h>

#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace formula
{

enum class FormulaEntryFlags : unsigned
{
    NONE = 0,
    COLORING_INSIDE = 1U << 0,
    COLORING_OUTSIDE = 1U << 1,
};

constexpr unsigned operator+(FormulaEntryFlags value)
{
    return static_cast<unsigned>(value);
}

constexpr FormulaEntryFlags operator|(FormulaEntryFlags lhs, FormulaEntryFlags rhs)
{
    return static_cast<FormulaEntryFlags>(+lhs | +rhs);
}

constexpr FormulaEntryFlags operator&(FormulaEntryFlags lhs, FormulaEntryFlags rhs)
{
    return static_cast<FormulaEntryFlags>(+lhs & +rhs);
}

struct FormulaEntry
{
    std::string name;
    std::string paren_value;
    std::string bracket_value;
    std::string body;
    bool is_class{};
    FormulaEntryFlags flags{};
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
    UNRESOLVED_CLASS,
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

struct FormulaImportReference
{
    std::string filename;
    std::size_t file_index{};
    std::size_t entry_index{};
    std::string imported_filename;
    std::optional<std::size_t> imported_file_index;
    SourceLocation location;
    bool implicit{};
};

struct FormulaClassReference
{
    std::string filename;
    std::string class_name;
    std::size_t file_index{};
    std::size_t class_index{};
    std::size_t entry_index{};
};

struct RetainedFormulaClass
{
    FormulaClassReference reference;
    ast::FormulaSectionsPtr ast;
};

enum class FormulaReferenceKind
{
    BASE_CLASS,
    DECLARATION,
    FUNCTION_RETURN,
    FUNCTION_ARGUMENT,
    NEW_OBJECT,
    PARAM_BLOCK,
};

struct FormulaReference
{
    FormulaReferenceKind kind{};
    std::string class_name;
    SourceLocation location;
};

struct FormulaEntryReferences
{
    std::string filename;
    std::size_t file_index{};
    std::size_t entry_index{};
    std::vector<FormulaReference> references;
};

struct FormulaResolvedReference
{
    FormulaEntryReferences entry;
    FormulaReference reference;
    FormulaClassReference klass;
};

struct FormulaFileSet
{
    std::vector<FormulaFile> files;
    std::vector<FormulaFileReference> file_index;
    std::vector<FormulaImportReference> import_graph;
    std::vector<FormulaClassReference> class_index;
    std::vector<RetainedFormulaClass> retained_classes;
    std::vector<FormulaEntryReferences> entry_references;
    std::vector<FormulaResolvedReference> resolved_references;
    std::vector<FormulaFileDiagnostic> diagnostics;
};

using FormulaFileImporter = std::function<std::string(std::string_view)>;

std::vector<FormulaEntry> load_formula_entries(std::istream &in);
FormulaFile load_formula_file(std::istream &in, std::string filename = {});
FormulaFileSet load_formula_file_tree(std::string_view root_filename, const FormulaFileImporter &importer);
ast::FormulaSectionsPtr parse_formula_class(FormulaFileSet &files, const FormulaClassReference &klass);
ast::FormulaSectionsPtr retain_formula_class(FormulaFileSet &files, const FormulaClassReference &klass);

} // namespace formula
