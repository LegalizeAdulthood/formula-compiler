// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/ExtendedRuntime.h>
#include <formula/FileEntry.h>
#include <formula/Formula.h>
#include <formula/Parameter.h>
#include <formula/ParseOptions.h>
#include <formula/SemanticAnalyzer.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace formula
{

namespace parser
{
class Parser;
} // namespace parser

enum class ExtendedInterpreterDiagnosticKind
{
    PARSE,
    REFERENCE,
    SEMANTIC,
};

struct ExtendedInterpreterDiagnostic
{
    ExtendedInterpreterDiagnosticKind kind{ExtendedInterpreterDiagnosticKind::PARSE};
    SourceLocation location;
    std::string filename;
    std::string message;
};

struct ExtendedInterpreterOptions
{
    parser::Options parser;
    const semantic::BuiltinRegistry *builtins{};
    std::size_t max_loop_iterations{1000000};
};

class ExtendedInterpreter
{
public:
    ExtendedInterpreter(FileEntry entry, ExtendedInterpreterOptions options);

    const std::vector<ExtendedInterpreterDiagnostic> &diagnostics() const;
    bool ok() const;

    void set_value(std::string_view name, Value value);
    Value value(std::string_view name) const;
    const std::vector<std::string> &messages() const;

    Value interpret(Section section);

private:
    void parse();
    void resolve_references();
    void analyze();
    void initialize_runtime_state();
    void add_parse_diagnostics(const parser::Parser &parser);
    void add_reference_diagnostics();
    void add_semantic_diagnostics(const std::vector<semantic::SemanticDiagnostic> &diagnostics);

    FileEntry m_entry;
    ExtendedInterpreterOptions m_options;
    ast::FormulaSectionsPtr m_ast;
    FormulaFileSet m_files;
    std::vector<ExtendedInterpreterDiagnostic> m_diagnostics;
    ExtendedRuntimeState m_state;
};

struct PreparedParameterFormula
{
    parameter::ParameterReferenceSite site;
    std::string filename;
    std::string entry;
    ExtendedInterpreter interpreter;
};

struct PreparedParameterSet
{
    std::vector<PreparedParameterFormula> formulas;
    std::vector<ExtendedInterpreterDiagnostic> diagnostics;

    bool ok() const;
};

PreparedParameterSet prepare_parameter_interpreters(
    const parameter::ParameterReferenceSet &references, ExtendedInterpreterOptions options);

} // namespace formula
