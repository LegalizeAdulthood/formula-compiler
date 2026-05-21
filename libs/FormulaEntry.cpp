// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/FormulaEntry.h>

#include <formula/FileEntry.h>
#include <formula/ParseOptions.h>
#include <formula/Parser.h>
#include <formula/Preprocessor.h>

#include <cctype>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace formula
{

enum class LoadState
{
    LOADING,
    LOADED,
};

static void strip_leading(std::string &text)
{
    if (const auto first_non_space = text.find_first_not_of(" \t\r\n"); first_non_space != 0)
    {
        text.erase(0, first_non_space);
    }
}

static void strip_trailing(std::string &text)
{
    if (const auto last_non_space = text.find_last_not_of(" \t\r\n"); last_non_space != std::string::npos)
    {
        text.erase(last_non_space + 1);
    }
    else
    {
        text.clear();
    }
}

static std::string uppercase(std::string text)
{
    for (char &ch : text)
    {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return text;
}

static FormulaEntryFlags entry_flags_from_paren(std::string_view paren_value)
{
    std::string value{paren_value};
    strip_leading(value);
    strip_trailing(value);
    value = uppercase(std::move(value));
    if (value == "INSIDE")
    {
        return FormulaEntryFlags::COLORING_INSIDE;
    }
    if (value == "OUTSIDE")
    {
        return FormulaEntryFlags::COLORING_OUTSIDE;
    }
    return FormulaEntryFlags::NONE;
}

static ClassHeader class_header(const FormulaEntry &entry, std::size_t index)
{
    ClassHeader result{entry.name, {}, {}, index};
    if (!entry.is_class || entry.paren_value.empty())
    {
        return result;
    }

    std::string base{entry.paren_value};
    strip_leading(base);
    strip_trailing(base);

    if (const auto colon = base.find_last_of(':'); colon != std::string::npos)
    {
        result.base_file = base.substr(0, colon);
        result.base_class = base.substr(colon + 1);
        strip_leading(result.base_file);
        strip_trailing(result.base_file);
        strip_leading(result.base_class);
        strip_trailing(result.base_class);
        return result;
    }

    result.base_class = std::move(base);
    return result;
}

static void append_entry_import(FormulaFile &file, std::size_t entry_index, FormulaImportDirective import)
{
    for (FormulaEntryImports &entry_imports : file.entry_imports)
    {
        if (entry_imports.entry_index == entry_index)
        {
            entry_imports.imports.push_back(std::move(import));
            return;
        }
    }
    FormulaEntryImports entry_imports;
    entry_imports.entry_index = entry_index;
    entry_imports.imports.push_back(std::move(import));
    file.entry_imports.push_back(std::move(entry_imports));
}

std::vector<FormulaEntry> load_formula_entries(std::istream &in)
{
    std::vector<FormulaEntry> formulas;
    for (const FileEntry &file_entry : load_file_entries(in))
    {
        std::string name{file_entry.name};
        strip_leading(name);
        strip_trailing(name);

        const std::string &paren_value{file_entry.paren_value};
        const std::string &bracket_value{file_entry.bracket_value};
        const FormulaEntryFlags flags{entry_flags_from_paren(paren_value)};

        bool is_class{};
        if (name.rfind("class", 0) == 0 && (name.size() == 5 || name[5] == ' ' || name[5] == '\t'))
        {
            is_class = true;
            name.erase(0, 5);
            strip_leading(name);
        }

        // skip entries with no name or where the name is "comment".
        if (name.empty() || name == "comment")
        {
            continue;
        }

        formulas.push_back({name, paren_value, bracket_value, file_entry.body, is_class, flags});
    }

    return formulas;
}

FormulaFile load_formula_file(std::istream &in, std::string filename)
{
    FormulaFile result;
    result.filename = std::move(filename);
    result.entries = load_formula_entries(in);
    for (std::size_t index = 0; index < result.entries.size(); ++index)
    {
        if (result.entries[index].is_class)
        {
            ClassHeader header{class_header(result.entries[index], index)};
            if (!header.base_file.empty())
            {
                append_entry_import(result, index,
                    FormulaImportDirective{header.base_file, SourceLocation{1, 1, result.filename}, true});
            }
            result.classes.push_back(std::move(header));
        }
    }
    return result;
}

static void add_diagnostic(FormulaFileSet &result, FormulaFileDiagnosticCode code, std::string_view filename,
    const std::vector<std::string> &import_stack)
{
    result.diagnostics.push_back(FormulaFileDiagnostic{code, std::string{filename}, {}, {}, import_stack});
}

static void add_diagnostic(FormulaFileSet &result, FormulaFileDiagnosticCode code, std::string_view filename,
    SourceLocation location, std::string detail, const std::vector<std::string> &import_stack)
{
    if (location.filename.empty())
    {
        location.filename = std::string{filename};
    }
    result.diagnostics.push_back(
        FormulaFileDiagnostic{code, std::string{filename}, std::move(location), std::move(detail), import_stack});
}

static void rebuild_indexes(FormulaFileSet &result)
{
    result.file_index.clear();
    result.import_graph.clear();
    result.class_index.clear();
    for (std::size_t file_index = 0; file_index < result.files.size(); ++file_index)
    {
        const FormulaFile &file = result.files[file_index];
        result.file_index.push_back(FormulaFileReference{file.filename, file_index});
        for (std::size_t class_index = 0; class_index < file.classes.size(); ++class_index)
        {
            const ClassHeader &klass = file.classes[class_index];
            result.class_index.push_back(
                FormulaClassReference{file.filename, klass.name, file_index, class_index, klass.entry_index});
        }
    }
    for (std::size_t file_index = 0; file_index < result.files.size(); ++file_index)
    {
        const FormulaFile &file = result.files[file_index];
        for (const FormulaEntryImports &entry_imports : file.entry_imports)
        {
            for (const FormulaImportDirective &import : entry_imports.imports)
            {
                std::optional<std::size_t> imported_file_index;
                for (const FormulaFileReference &reference : result.file_index)
                {
                    if (reference.filename == import.filename)
                    {
                        imported_file_index = reference.file_index;
                        break;
                    }
                }
                result.import_graph.push_back(FormulaImportReference{file.filename, file_index,
                    entry_imports.entry_index, import.filename, imported_file_index, import.location, import.implicit});
            }
        }
    }
}

static void add_parser_errors(FormulaFileSet &result, std::string_view filename, const parser::ParserPtr &parser,
    const std::vector<std::string> &import_stack)
{
    for (const parser::Diagnostic &error : parser->get_errors())
    {
        add_diagnostic(result, FormulaFileDiagnosticCode::PARSE_ERROR, filename, error.position,
            parser::to_string(error.code), import_stack);
    }
}

static std::vector<FormulaImportDirective> collect_entry_imports(const FormulaEntry &entry, std::string_view filename,
    FormulaFileSet &result, const std::vector<std::string> &import_stack)
{
    std::vector<FormulaImportDirective> imports;
    if (entry.body.find("import") == std::string::npos)
    {
        return imports;
    }

    parser::Options options;
    options.source_filename = std::string{filename};
    options.entry_kind = entry.is_class ? parser::EntryKind::CLASS : parser::EntryKind::FRACTAL;

    const parser::ParserPtr parser{parser::create_parser(entry.body, options)};
    const ast::FormulaSectionsPtr ast{parser->parse()};
    if (!parser->get_errors().empty())
    {
        add_parser_errors(result, filename, parser, import_stack);
        return imports;
    }
    if (!ast)
    {
        add_diagnostic(result, FormulaFileDiagnosticCode::PARSE_ERROR, filename, import_stack);
        return imports;
    }

    for (const ast::ImportDirective &import : ast->imports)
    {
        imports.push_back(FormulaImportDirective{import.filename, import.location, false});
    }
    return imports;
}

static void load_formula_file_tree(std::string_view filename, const FormulaFileImporter &importer,
    FormulaFileSet &result, std::unordered_map<std::string, LoadState> &states, std::vector<std::string> &import_stack)
{
    const std::string key{filename};
    if (const auto state = states.find(key); state != states.end())
    {
        if (state->second == LoadState::LOADING)
        {
            add_diagnostic(result, FormulaFileDiagnosticCode::IMPORT_CYCLE, filename, import_stack);
        }
        return;
    }

    states.emplace(key, LoadState::LOADING);
    import_stack.push_back(key);

    std::string text;
    try
    {
        text = importer(filename);
    }
    catch (...)
    {
        add_diagnostic(result, FormulaFileDiagnosticCode::MISSING_IMPORT, filename, import_stack);
        states[key] = LoadState::LOADED;
        import_stack.pop_back();
        return;
    }

    preprocessor::Preprocessor preprocessor{preprocessor::UltraFractalMacros::ULTRAFRACTAL6, key};
    text = preprocessor.process(text);
    if (!preprocessor.errors().empty())
    {
        for (const preprocessor::Diagnostic &error : preprocessor.errors())
        {
            add_diagnostic(result, FormulaFileDiagnosticCode::PREPROCESS_ERROR, filename, error.position,
                preprocessor::to_string(error.code), import_stack);
        }
        states[key] = LoadState::LOADED;
        import_stack.pop_back();
        return;
    }

    std::istringstream in{text};
    FormulaFile file{load_formula_file(in, key)};
    std::vector<std::string> imports;
    for (std::size_t index = 0; index < file.entries.size(); ++index)
    {
        std::vector<FormulaImportDirective> explicit_imports{
            collect_entry_imports(file.entries[index], key, result, import_stack)};
        for (FormulaImportDirective &import : explicit_imports)
        {
            append_entry_import(file, index, std::move(import));
        }
    }
    for (const FormulaEntryImports &entry_imports : file.entry_imports)
    {
        for (const FormulaImportDirective &import : entry_imports.imports)
        {
            imports.push_back(import.filename);
        }
    }
    result.files.push_back(std::move(file));

    for (const std::string &imported_filename : imports)
    {
        load_formula_file_tree(imported_filename, importer, result, states, import_stack);
    }

    states[key] = LoadState::LOADED;
    import_stack.pop_back();
}

FormulaFileSet load_formula_file_tree(std::string_view root_filename, const FormulaFileImporter &importer)
{
    FormulaFileSet result;
    if (!importer)
    {
        add_diagnostic(result, FormulaFileDiagnosticCode::MISSING_IMPORT, root_filename, {});
        return result;
    }

    std::unordered_map<std::string, LoadState> states;
    std::vector<std::string> import_stack;
    load_formula_file_tree(root_filename, importer, result, states, import_stack);
    rebuild_indexes(result);
    return result;
}

ast::FormulaSectionsPtr parse_formula_class(FormulaFileSet &files, const FormulaClassReference &klass)
{
    if (klass.file_index >= files.files.size())
    {
        add_diagnostic(files, FormulaFileDiagnosticCode::PARSE_ERROR, klass.filename, {});
        return nullptr;
    }

    const FormulaFile &file = files.files[klass.file_index];
    if (klass.entry_index >= file.entries.size())
    {
        add_diagnostic(files, FormulaFileDiagnosticCode::PARSE_ERROR, file.filename, {});
        return nullptr;
    }

    const FormulaEntry &entry = file.entries[klass.entry_index];
    parser::Options options;
    options.source_filename = file.filename;
    options.entry_kind = parser::EntryKind::CLASS;

    const parser::ParserPtr parser{parser::create_parser(entry.body, options)};
    ast::FormulaSectionsPtr ast{parser->parse()};
    if (!parser->get_errors().empty())
    {
        add_parser_errors(files, file.filename, parser, {});
        return nullptr;
    }
    if (!ast)
    {
        add_diagnostic(files, FormulaFileDiagnosticCode::PARSE_ERROR, file.filename, {});
        return nullptr;
    }
    return ast;
}

static bool same_class_reference(const FormulaClassReference &lhs, const FormulaClassReference &rhs)
{
    return lhs.file_index == rhs.file_index && lhs.class_index == rhs.class_index && lhs.entry_index == rhs.entry_index;
}

ast::FormulaSectionsPtr retain_formula_class(FormulaFileSet &files, const FormulaClassReference &klass)
{
    for (const RetainedFormulaClass &retained : files.retained_classes)
    {
        if (same_class_reference(retained.reference, klass))
        {
            return retained.ast;
        }
    }

    ast::FormulaSectionsPtr ast{parse_formula_class(files, klass)};
    if (!ast)
    {
        return nullptr;
    }

    files.retained_classes.push_back(RetainedFormulaClass{klass, ast});
    return ast;
}

} // namespace formula
