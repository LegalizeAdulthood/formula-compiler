// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/FormulaEntry.h>
#include <formula/ParseOptions.h>
#include <formula/Parser.h>
#include <formula/Preprocessor.h>

#include <cassert>
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
    std::string line;
    while (std::getline(in, line))
    {
        const auto open_brace{line.find_last_of("{")};
        if (open_brace == std::string::npos)
        {
            continue;
        }
        // was the opening brace commented out?
        if (const auto semi{line.find_first_of(";")}; semi < open_brace)
        {
            continue;
        }

        std::string name{line};
        name.erase(open_brace);
        strip_leading(name);
        strip_trailing(name);

        std::string bracket_value;
        if (const auto close_bracket = name.find_last_of(']'); close_bracket != std::string::npos)
        {
            if (const auto open_bracket = name.find_last_of('[', close_bracket); open_bracket != std::string::npos)
            {
                bracket_value = name.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                name.erase(open_bracket, close_bracket - open_bracket + 1);
                strip_trailing(name);
            }
            else
            {
                // TODO: throw exception?
                assert(false);
            }
        }

        std::string paren_value;
        if (const auto close_paren = name.find_last_of(')'); close_paren != std::string::npos)
        {
            if (const auto open_paren = name.find_last_of('(', close_paren); open_paren != std::string::npos)
            {
                paren_value = name.substr(open_paren + 1, close_paren - open_paren - 1);
                name.erase(open_paren, close_paren - open_paren + 1);
                strip_trailing(name);
            }
            else
            {
                // TODO: throw exception?
                assert(false);
            }
        }

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
            while (line.find("}") == std::string::npos)
            {
                if (!std::getline(in, line))
                {
                    break;
                }
            }
            continue;
        }

        std::string body;
        line.erase(0, open_brace + 1);

        // Check if the closing brace is on the same line
        if (const auto brace = line.find("}"); brace != std::string::npos)
        {
            // Single-line entry - don't append newline
            line.erase(brace);
            body.append(line);
            formulas.push_back({name, paren_value, bracket_value, body, is_class});
            continue;
        }

        // Multi-line entry
        const auto accum = [&body, &line]()
        {
            body.append(line);
            body.append(1, '\n');
        };
        accum();
        bool found_brace{};
        while (std::getline(in, line))
        {
            const auto semi{line.find_first_of(";")};
            if (const auto brace = line.find("}");
                brace != std::string::npos && (semi == std::string::npos || semi > brace))
            {
                found_brace = true;
                line.erase(brace);
                accum();
                break;
            }
            accum();
        }
        if (found_brace)
        {
            formulas.push_back({name, paren_value, bracket_value, body, is_class});
        }
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
        for (const parser::Diagnostic &error : parser->get_errors())
        {
            add_diagnostic(result, FormulaFileDiagnosticCode::PARSE_ERROR, filename, error.position,
                parser::to_string(error.code), import_stack);
        }
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
    return result;
}

} // namespace formula
