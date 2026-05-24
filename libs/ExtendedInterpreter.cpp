// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedInterpreter.h>

#include <formula/Parser.h>
#include <formula/ReferenceCollector.h>

#include <stdexcept>
#include <utility>

namespace formula
{

namespace
{

std::string reference_diagnostic_message(const FormulaFileDiagnostic &diagnostic)
{
    switch (diagnostic.code)
    {
    case FormulaFileDiagnosticCode::MISSING_IMPORT:
        return "missing import: " + diagnostic.filename;
    case FormulaFileDiagnosticCode::IMPORT_CYCLE:
        return "import cycle: " + diagnostic.filename;
    case FormulaFileDiagnosticCode::PREPROCESS_ERROR:
        return "preprocess error: " + diagnostic.detail;
    case FormulaFileDiagnosticCode::PARSE_ERROR:
        return "parse error: " + diagnostic.filename;
    case FormulaFileDiagnosticCode::UNRESOLVED_CLASS:
        return "unresolved class: " + diagnostic.detail;
    }
    throw std::runtime_error("unknown formula file diagnostic code");
}

const ast::Expr &section_expr(const ast::FormulaSections &formula, Section section)
{
    switch (section)
    {
    case Section::PER_IMAGE:
        return formula.per_image;
    case Section::BUILTIN:
        return formula.builtin;
    case Section::INITIALIZE:
        return formula.initialize;
    case Section::ITERATE:
        return formula.iterate;
    case Section::BAILOUT:
        return formula.bailout;
    case Section::PERTURB_INITIALIZE:
        return formula.perturb_initialize;
    case Section::PERTURB_ITERATE:
        return formula.perturb_iterate;
    case Section::DEFAULT:
        return formula.defaults;
    case Section::SWITCH:
        return formula.type_switch;
    default:
        throw std::runtime_error("invalid section");
    }
}

} // namespace

ExtendedInterpreter::ExtendedInterpreter(FileEntry entry, ExtendedInterpreterOptions options) :
    m_entry(std::move(entry)),
    m_options(std::move(options))
{
    parse();
    resolve_references();
    analyze();
}

const std::vector<ExtendedInterpreterDiagnostic> &ExtendedInterpreter::diagnostics() const
{
    return m_diagnostics;
}

bool ExtendedInterpreter::ok() const
{
    return m_diagnostics.empty();
}

void ExtendedInterpreter::set_value(std::string_view name, Value value)
{
    m_state.set_formula_value(name, std::move(value));
}

Value ExtendedInterpreter::value(std::string_view name) const
{
    return m_state.value(name);
}

Value ExtendedInterpreter::interpret(Section section)
{
    if (!ok())
    {
        throw std::runtime_error("extended interpreter has diagnostics");
    }
    if (!m_ast)
    {
        throw std::runtime_error("extended interpreter has no AST");
    }
    if (!section_expr(*m_ast, section))
    {
        return {};
    }
    throw std::runtime_error("extended interpreter execution is not implemented");
}

void ExtendedInterpreter::parse()
{
    const parser::ParserPtr parser{parser::create_parser(m_entry.body, m_options.parser)};
    m_ast = parser->parse();
    add_parse_diagnostics(*parser);
}

void ExtendedInterpreter::resolve_references()
{
    if (!m_ast || !m_options.parser.file_importer || m_options.parser.source_filename.empty())
    {
        return;
    }
    m_files = load_formula_file_tree(m_options.parser.source_filename, m_options.parser.file_importer);
    resolve_root_formula_file_references(m_files);
    retain_resolved_imported_classes(m_files);
    add_reference_diagnostics();
}

void ExtendedInterpreter::analyze()
{
    if (!m_ast || !m_diagnostics.empty())
    {
        return;
    }
    std::vector<const RetainedFormulaClass *> retained_classes;
    retained_classes.reserve(m_files.retained_classes.size());
    for (const RetainedFormulaClass &retained : m_files.retained_classes)
    {
        retained_classes.push_back(&retained);
    }
    semantic::FormulaSemanticContext context;
    context.entry_kind = m_options.parser.entry_kind;
    context.source_name = m_options.parser.source_filename.empty() ? m_entry.name : m_options.parser.source_filename;
    context.retained_classes = std::move(retained_classes);
    context.builtins = m_options.builtins;
    add_semantic_diagnostics(semantic::analyze_formula(*m_ast, context));
}

void ExtendedInterpreter::add_parse_diagnostics(const parser::Parser &parser)
{
    for (const parser::Diagnostic &diagnostic : parser.get_errors())
    {
        m_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::PARSE,
            diagnostic.position, m_options.parser.source_filename, parser::to_string(diagnostic.code)});
    }
}

void ExtendedInterpreter::add_reference_diagnostics()
{
    for (const FormulaFileDiagnostic &diagnostic : m_files.diagnostics)
    {
        m_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::REFERENCE,
            diagnostic.location, diagnostic.filename, reference_diagnostic_message(diagnostic)});
    }
}

void ExtendedInterpreter::add_semantic_diagnostics(const std::vector<semantic::SemanticDiagnostic> &diagnostics)
{
    for (const semantic::SemanticDiagnostic &diagnostic : diagnostics)
    {
        if (diagnostic.severity != semantic::SemanticDiagnosticSeverity::ERROR)
        {
            continue;
        }
        m_diagnostics.push_back(ExtendedInterpreterDiagnostic{ExtendedInterpreterDiagnosticKind::SEMANTIC,
            diagnostic.location, diagnostic.entry_name, diagnostic.message});
    }
}

} // namespace formula
