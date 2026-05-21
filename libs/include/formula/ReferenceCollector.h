// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/FormulaEntry.h>
#include <formula/Node.h>
#include <formula/SourceLocation.h>

#include <cstddef>
#include <string>
#include <vector>

namespace formula
{

enum class FormulaReferenceKind
{
    BASE_CLASS,
    DECLARATION,
    FUNCTION_RETURN,
    FUNCTION_ARGUMENT,
    NEW_OBJECT,
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

std::vector<FormulaReference> collect_formula_references(const ast::FormulaSections &ast);
std::vector<FormulaReference> collect_formula_references(const ClassHeader &header, const ast::FormulaSections &ast);
FormulaEntryReferences collect_formula_entry_references(
    FormulaFileSet &files, std::size_t file_index, std::size_t entry_index);

} // namespace formula
