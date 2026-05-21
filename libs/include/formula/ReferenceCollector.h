// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/FormulaEntry.h>
#include <formula/Node.h>
#include <formula/SourceLocation.h>

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

std::vector<FormulaReference> collect_formula_references(const ast::FormulaSections &ast);
std::vector<FormulaReference> collect_formula_references(const ClassHeader &header, const ast::FormulaSections &ast);

} // namespace formula
