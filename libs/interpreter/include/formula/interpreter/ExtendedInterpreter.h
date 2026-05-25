// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/interpreter/ExtendedRuntime.h>
#include <formula/core/FileEntry.h>
#include <formula/facade/Formula.h>
#include <formula/parser/Parameter.h>
#include <formula/parser/ParseOptions.h>
#include <formula/semantics/SemanticAnalyzer.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
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
    BINDING,
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
    const std::vector<semantic::FormulaParameterInfo> &parameters() const;

    void set_value(std::string_view name, Value value);
    void set_parameter(std::string_view name, Value value);
    void set_function_parameter(std::string_view name, std::string_view target);
    void set_plugin_parameter(std::string_view name, std::string_view selector);
    void set_plugin_parameter_value(std::string_view plugin_name, std::string_view nested_name, Value value);
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
    void add_binding_diagnostic(std::string_view name, std::string message);
    void clear_binding_diagnostics(std::string_view name);
    void rebuild_diagnostics();
    bool set_nonselectable_plugin_parameter_value(std::string_view name, Value value);

    FileEntry m_entry;
    ExtendedInterpreterOptions m_options;
    ast::FormulaSectionsPtr m_ast;
    FormulaFileSet m_files;
    std::vector<ExtendedInterpreterDiagnostic> m_construction_diagnostics;
    std::vector<std::pair<std::string, ExtendedInterpreterDiagnostic>> m_binding_diagnostics;
    std::vector<ExtendedInterpreterDiagnostic> m_diagnostics;
    std::vector<semantic::FormulaParameterInfo> m_parameters;
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
