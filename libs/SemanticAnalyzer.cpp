// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/SemanticAnalyzer.h>

namespace formula::semantic
{

std::vector<SemanticDiagnostic> analyze_formula(const ast::FormulaSections &, const FormulaSemanticContext &)
{
    return {};
}

std::vector<SemanticDiagnostic> analyze_parameter_set(const parameter::ExtendedParameterEntry &,
    const parameter::ParameterReferenceSet &, const ParameterSetSemanticContext &)
{
    return {};
}

} // namespace formula::semantic
