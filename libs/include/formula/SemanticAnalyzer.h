// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/FormulaEntry.h>
#include <formula/Parameter.h>
#include <formula/ParseOptions.h>
#include <formula/SourceLocation.h>

#include <memory>
#include <string>
#include <vector>

namespace formula::semantic
{

enum class SemanticDiagnosticSeverity
{
    ERROR,
    WARNING,
};

enum class SemanticDiagnosticCode
{
    UNKNOWN_SYMBOL,
    DUPLICATE_SYMBOL,
    UNKNOWN_TYPE,
    INVALID_TYPE_CONVERSION,
    INVALID_ASSIGNMENT_TARGET,
    INVALID_CALL_TARGET,
    INVALID_CALL_ARITY,
    INVALID_ARGUMENT_TYPE,
    INVALID_MEMBER_ACCESS,
    INVALID_ARRAY_ACCESS,
    INVALID_RETURN,
    INVALID_SECTION_RESULT,
    INVALID_PARAMETER_BINDING,
    INVALID_FORMULA_KIND,
    INVALID_BUILTIN_USAGE,
    INCOMPLETE_REFERENCE_GRAPH,
    UNSUPPORTED_SEMANTIC_FEATURE,
};

struct SemanticDiagnostic
{
    SemanticDiagnosticSeverity severity{SemanticDiagnosticSeverity::ERROR};
    SemanticDiagnosticCode code{SemanticDiagnosticCode::UNKNOWN_SYMBOL};
    SourceLocation location;
    std::string message;
    std::string entry_name;
    std::string section_name;
};

enum class SemanticTypeKind
{
    ERROR,
    VOID,
    BOOL,
    INT,
    FLOAT,
    COMPLEX,
    COLOR,
    STRING,
    ARRAY,
    CLASS_OBJECT,
    BUILTIN_OBJECT,
    FUNCTION,
};

struct SemanticType
{
    SemanticTypeKind kind{SemanticTypeKind::ERROR};
    std::string name;
    std::shared_ptr<const SemanticType> element_type;
};

enum class SemanticSymbolKind
{
    LOCAL,
    FUNCTION_PARAMETER,
    FORMULA_PARAMETER,
    FORMULA_CONSTANT,
    USER_FUNCTION,
    BUILTIN_FUNCTION,
    CLASS,
    FIELD,
    METHOD,
    CONSTRUCTOR,
    BUILTIN_CONSTANT,
};

struct SemanticSymbol
{
    SemanticSymbolKind kind{SemanticSymbolKind::LOCAL};
    std::string name;
    SemanticType type;
    SourceLocation location;
};

struct BuiltinRegistry
{
};

struct FormulaSemanticContext
{
    parser::EntryKind entry_kind{parser::EntryKind::FRACTAL};
    std::string source_name;
    std::vector<const RetainedFormulaClass *> retained_classes;
    const BuiltinRegistry *builtins{};
};

struct ParameterSetSemanticContext
{
    const BuiltinRegistry *builtins{};
    std::vector<const FormulaEntry *> referenced_formulas;
    std::vector<const RetainedFormulaClass *> retained_classes;
};

std::vector<SemanticDiagnostic> analyze_formula(
    const ast::FormulaSections &formula, const FormulaSemanticContext &context);

std::vector<SemanticDiagnostic> analyze_parameter_set(const parameter::ExtendedParameterEntry &parameters,
    const parameter::ParameterReferenceSet &references, const ParameterSetSemanticContext &context);

} // namespace formula::semantic
