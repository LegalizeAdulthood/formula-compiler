// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/FormulaEntry.h>
#include <formula/Node.h>

#include <vector>

namespace formula
{

std::vector<FormulaReference> collect_formula_references(const ast::FormulaSections &ast);
std::vector<FormulaReference> collect_formula_references(const ClassHeader &header, const ast::FormulaSections &ast);
FormulaEntryReferences collect_formula_entry_references(
    FormulaFileSet &files, std::size_t file_index, std::size_t entry_index);
void collect_formula_file_references(FormulaFileSet &files);

} // namespace formula
