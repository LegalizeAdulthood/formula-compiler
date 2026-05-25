// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/facade/Formula.h>
#include <formula/parser/FormulaEntry.h>
#include <formula/parser/ParseOptions.h>
#include <formula/parser/Parser.h>
#include <formula/semantics/ReferenceCollector.h>

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace formula;
using namespace formula::parser;
using namespace testing;

namespace
{

ast::FormulaSectionsPtr parse_extended(std::string_view text, EntryKind kind = EntryKind::FRACTAL)
{
    Options options;
    options.entry_kind = kind;
    return parse(text, options);
}

} // namespace

TEST(TestFormulaEntry, parenValue)
{
    const char *const frm{R"entry(
Mandelbrot(XAXIS) {
}
)entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_EQ("XAXIS", entries[0].file_entry.paren_value);
    EXPECT_TRUE(entries[0].file_entry.bracket_value.empty());
    EXPECT_EQ("\n", entries[0].file_entry.body);
}

TEST(TestFormulaEntry, bracketValue)
{
    const char *const frm{R"entry(
Mandelbrot [float=y] {
}
)entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_TRUE(entries[0].file_entry.paren_value.empty());
    EXPECT_EQ("float=y", entries[0].file_entry.bracket_value);
    EXPECT_EQ("\n", entries[0].file_entry.body);
}

TEST(TestFormulaEntry, parenBracketValue)
{
    const char *const frm{R"entry(
Mandelbrot(XAXIS) [float=y] {
}
)entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_EQ("XAXIS", entries[0].file_entry.paren_value);
    EXPECT_EQ("float=y", entries[0].file_entry.bracket_value);
    EXPECT_EQ("\n", entries[0].file_entry.body);
}

TEST(TestFormulaEntry, coloringEntryFlags)
{
    const char *const formulas{R"entry(
InsideColor( INSIDE ) {
}
OutsideColor(outside) {
}
BothColor(BOTH) {
}
)entry"};
    std::istringstream str{formulas};

    auto entries{load_formula_entries(str)};

    ASSERT_EQ(3U, entries.size());
    EXPECT_EQ(FormulaEntryFlags::COLORING_INSIDE, entries[0].flags);
    EXPECT_EQ(FormulaEntryFlags::COLORING_OUTSIDE, entries[1].flags);
    EXPECT_EQ(FormulaEntryFlags::NONE, entries[2].flags);
}

TEST(TestFormulaEntry, transformationParenOptionIsNotColoringFlag)
{
    const char *const transform{R"entry(
DiagonalTiling(diagonal tiling) {
}
)entry"};
    std::istringstream str{transform};

    auto entries{load_formula_entries(str)};

    ASSERT_EQ(1U, entries.size());
    EXPECT_EQ("DiagonalTiling", entries[0].file_entry.name);
    EXPECT_EQ("diagonal tiling", entries[0].file_entry.paren_value);
    EXPECT_EQ(FormulaEntryFlags::NONE, entries[0].flags);
}

TEST(TestFormulaEntry, classEntry)
{
    const char *const frm{R"entry(
class Derived(Test.ufm:Base) {
public:
}
)entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Derived", entries[0].file_entry.name);
    EXPECT_EQ("Test.ufm:Base", entries[0].file_entry.paren_value);
    EXPECT_TRUE(entries[0].is_class);
    EXPECT_EQ("\n"
              "public:\n",
        entries[0].file_entry.body);
}

TEST(TestFormulaEntry, fileIndexesClassHeaders)
{
    const char *const frm{R"entry(
Mandelbrot {
}
class Base {
}
class Derived(Test.ufm:Base) {
}
)entry"};
    std::istringstream str{frm};

    auto file{load_formula_file(str, "main.ufm")};

    EXPECT_EQ("main.ufm", file.filename);
    ASSERT_EQ(3U, file.entries.size());
    ASSERT_EQ(2U, file.classes.size());
    EXPECT_EQ("Base", file.classes[0].name);
    EXPECT_TRUE(file.classes[0].base_file.empty());
    EXPECT_TRUE(file.classes[0].base_class.empty());
    EXPECT_EQ(1U, file.classes[0].entry_index);
    EXPECT_EQ("Derived", file.classes[1].name);
    EXPECT_EQ("Test.ufm", file.classes[1].base_file);
    EXPECT_EQ("Base", file.classes[1].base_class);
    EXPECT_EQ(2U, file.classes[1].entry_index);
    ASSERT_EQ(1U, file.entry_imports.size());
    EXPECT_EQ(2U, file.entry_imports[0].entry_index);
    ASSERT_EQ(1U, file.entry_imports[0].imports.size());
    EXPECT_EQ("Test.ufm", file.entry_imports[0].imports[0].filename);
    EXPECT_TRUE(file.entry_imports[0].imports[0].implicit);
}

TEST(TestFormulaEntry, fileIndexesLocalBaseClass)
{
    const char *const frm{R"entry(
class Derived(Base) {
}
)entry"};
    std::istringstream str{frm};

    auto file{load_formula_file(str)};

    ASSERT_EQ(1U, file.classes.size());
    EXPECT_EQ("Derived", file.classes[0].name);
    EXPECT_TRUE(file.classes[0].base_file.empty());
    EXPECT_EQ("Base", file.classes[0].base_class);
}

TEST(TestFormulaEntry, fileTreeLoadsImplicitBaseImports)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Derived(base.ufm:Base) {\n"
            "}\n"},
        {"base.ufm",
            "class Base {\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(2U, result.files.size());
    EXPECT_EQ("main.ufm", result.files[0].filename);
    EXPECT_EQ("base.ufm", result.files[1].filename);
    ASSERT_EQ(2U, result.file_index.size());
    EXPECT_EQ("main.ufm", result.file_index[0].filename);
    EXPECT_EQ(0U, result.file_index[0].file_index);
    EXPECT_EQ("base.ufm", result.file_index[1].filename);
    EXPECT_EQ(1U, result.file_index[1].file_index);
    ASSERT_EQ(2U, result.class_index.size());
    EXPECT_EQ("Derived", result.class_index[0].class_name);
    EXPECT_EQ("main.ufm", result.class_index[0].filename);
    EXPECT_EQ(0U, result.class_index[0].file_index);
    EXPECT_EQ(0U, result.class_index[0].class_index);
    EXPECT_EQ(0U, result.class_index[0].entry_index);
    EXPECT_EQ("Base", result.class_index[1].class_name);
    EXPECT_EQ("base.ufm", result.class_index[1].filename);
    EXPECT_EQ(1U, result.class_index[1].file_index);
    EXPECT_EQ(0U, result.class_index[1].class_index);
    EXPECT_EQ(0U, result.class_index[1].entry_index);
    ASSERT_EQ(1U, result.files[1].classes.size());
    EXPECT_EQ("Base", result.files[1].classes[0].name);
}

TEST(TestFormulaEntry, fileTreeLoadsExplicitImports)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(2U, result.files.size());
    EXPECT_EQ("main.ufm", result.files[0].filename);
    EXPECT_EQ("common.ulb", result.files[1].filename);
    ASSERT_EQ(1U, result.files[1].classes.size());
    EXPECT_EQ("Texture", result.files[1].classes[0].name);
    ASSERT_EQ(1U, result.files[0].entry_imports.size());
    EXPECT_EQ(0U, result.files[0].entry_imports[0].entry_index);
    ASSERT_EQ(1U, result.files[0].entry_imports[0].imports.size());
    EXPECT_EQ("common.ulb", result.files[0].entry_imports[0].imports[0].filename);
    EXPECT_FALSE(result.files[0].entry_imports[0].imports[0].implicit);
    EXPECT_EQ("main.ufm", result.files[0].entry_imports[0].imports[0].location.filename);
}

TEST(TestFormulaEntry, parsesLoadedClassOnDemand)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(1U, result.class_index.size());
    ast::FormulaSectionsPtr klass{parse_formula_class(result, result.class_index[0])};

    ASSERT_TRUE(klass);
    EXPECT_TRUE(klass->public_members);
    EXPECT_TRUE(result.diagnostics.empty());
    EXPECT_TRUE(result.retained_classes.empty());
}

TEST(TestFormulaEntry, retainsLoadedClassOnDemand)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(1U, result.class_index.size());
    ast::FormulaSectionsPtr first{retain_formula_class(result, result.class_index[0])};
    ast::FormulaSectionsPtr second{retain_formula_class(result, result.class_index[0])};

    ASSERT_TRUE(first);
    EXPECT_EQ(first, second);
    ASSERT_EQ(1U, result.retained_classes.size());
    EXPECT_EQ("Texture", result.retained_classes[0].reference.class_name);
    EXPECT_EQ(first, result.retained_classes[0].ast);
}

TEST(TestFormulaEntry, classParseErrorsKeepFilename)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Broken {\n"
            "if true\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(1U, result.class_index.size());
    ast::FormulaSectionsPtr klass{parse_formula_class(result, result.class_index[0])};

    EXPECT_FALSE(klass);
    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(FormulaFileDiagnosticCode::PARSE_ERROR, result.diagnostics[0].code);
    EXPECT_EQ("main.ufm", result.diagnostics[0].filename);
    EXPECT_EQ("main.ufm", result.diagnostics[0].location.filename);
}

TEST(TestFormulaEntry, fileTreeLoadsChainedExplicitImports)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"first.ulb\"\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
        {"first.ulb",
            "class First {\n"
            "import \"second.ulb\"\n"
            "public:\n"
            "int value\n"
            "}\n"},
        {"second.ulb",
            "class Second {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(3U, result.files.size());
    EXPECT_EQ("main.ufm", result.files[0].filename);
    EXPECT_EQ("first.ulb", result.files[1].filename);
    EXPECT_EQ("second.ulb", result.files[2].filename);
}

TEST(TestFormulaEntry, fileTreePreprocessesBeforeIndexing)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "$ifdef nope\n"
            "import \"skip.ulb\"\n"
            "$endif\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(1U, result.files.size());
    EXPECT_EQ("main.ufm", result.files[0].filename);
}

TEST(TestFormulaEntry, fileTreeReportsPreprocessErrors)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "$ifdef nope\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::PREPROCESS_ERROR, result.diagnostics[0].code);
    EXPECT_EQ("main.ufm", result.diagnostics[0].filename);
    EXPECT_EQ("main.ufm", result.diagnostics[0].location.filename);
    EXPECT_EQ(5U, result.diagnostics[0].location.line);
    EXPECT_EQ("EXPECTED_DIRECTIVE_ENDIF", result.diagnostics[0].detail);
    ASSERT_EQ(1U, result.diagnostics[0].import_stack.size());
    EXPECT_EQ("main.ufm", result.diagnostics[0].import_stack[0]);
    EXPECT_TRUE(result.files.empty());
}

TEST(TestFormulaEntry, fileTreeReportsParseErrors)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"bad.ulb\"\n"
            "z=0:z=z+1,|z|<4\n"
            "}\n"},
        {"bad.ulb",
            "class Bad {\n"
            "import\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::PARSE_ERROR, result.diagnostics[0].code);
    EXPECT_EQ("bad.ulb", result.diagnostics[0].filename);
    EXPECT_EQ("bad.ulb", result.diagnostics[0].location.filename);
    EXPECT_EQ(2U, result.diagnostics[0].location.line);
    EXPECT_EQ("EXPECTED_STRING", result.diagnostics[0].detail);
    ASSERT_EQ(2U, result.diagnostics[0].import_stack.size());
    EXPECT_EQ("main.ufm", result.diagnostics[0].import_stack[0]);
    EXPECT_EQ("bad.ulb", result.diagnostics[0].import_stack[1]);
}

TEST(TestFormulaEntry, fileTreeReportsMissingImport)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Derived(missing.ufm:Base) {\n"
            "}\n"},
    };

    auto result{load_formula_file_tree("main.ufm",
        [&files](std::string_view filename) -> std::optional<std::string>
        {
            const auto found = files.find(std::string{filename});
            if (found == files.end())
            {
                return std::nullopt;
            }
            return found->second;
        })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::MISSING_IMPORT, result.diagnostics[0].code);
    EXPECT_EQ("missing.ufm", result.diagnostics[0].filename);
    ASSERT_EQ(2U, result.diagnostics[0].import_stack.size());
    EXPECT_EQ("main.ufm", result.diagnostics[0].import_stack[0]);
    EXPECT_EQ("missing.ufm", result.diagnostics[0].import_stack[1]);
}

TEST(TestFormulaEntry, fileTreePropagatesImporterExceptions)
{
    EXPECT_THROW(load_formula_file_tree("main.ufm",
                     [](std::string_view) -> std::optional<std::string> { throw std::runtime_error{"import failed"}; }),
        std::runtime_error);
}

TEST(TestFormulaEntry, fileTreeReportsImportCycle)
{
    std::unordered_map<std::string, std::string> files{
        {"a.ufm",
            "class A(b.ufm:B) {\n"
            "}\n"},
        {"b.ufm",
            "class B(a.ufm:A) {\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "a.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::IMPORT_CYCLE, result.diagnostics[0].code);
    EXPECT_EQ("a.ufm", result.diagnostics[0].filename);
    ASSERT_EQ(2U, result.diagnostics[0].import_stack.size());
    EXPECT_EQ("a.ufm", result.diagnostics[0].import_stack[0]);
    EXPECT_EQ("b.ufm", result.diagnostics[0].import_stack[1]);
}

TEST(TestFormulaEntry, loadFormulaIncludesFileMetadata)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z=z+1\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };
    Options options;
    options.source_filename = "main.ufm";
    options.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };

    LoadedFormula result{load_formula("import \"common.ulb\"\n"
                                      "z=0:z=z+1,|z|<4",
        options)};

    ASSERT_TRUE(result.ast);
    ASSERT_TRUE(result.files.diagnostics.empty());
    ASSERT_EQ(2U, result.files.files.size());
    EXPECT_EQ("main.ufm", result.files.files[0].filename);
    EXPECT_EQ("common.ulb", result.files.files[1].filename);
    ASSERT_EQ(1U, result.files.import_graph.size());
    EXPECT_EQ("main.ufm", result.files.import_graph[0].filename);
    EXPECT_EQ(0U, result.files.import_graph[0].file_index);
    EXPECT_EQ(0U, result.files.import_graph[0].entry_index);
    EXPECT_EQ("common.ulb", result.files.import_graph[0].imported_filename);
    ASSERT_TRUE(result.files.import_graph[0].imported_file_index);
    EXPECT_EQ(1U, *result.files.import_graph[0].imported_file_index);
    EXPECT_FALSE(result.files.import_graph[0].implicit);
    ASSERT_EQ(1U, result.files.class_index.size());
    EXPECT_EQ("Texture", result.files.class_index[0].class_name);
    ASSERT_EQ(2U, result.files.entry_references.size());
    ASSERT_EQ(1U, result.files.resolved_references.size());
    EXPECT_EQ("Texture", result.files.resolved_references[0].klass.class_name);
    ASSERT_EQ(1U, result.files.retained_classes.size());
    EXPECT_EQ("Texture", result.files.retained_classes[0].reference.class_name);
    EXPECT_EQ("common.ulb", result.files.retained_classes[0].reference.filename);
    ASSERT_TRUE(result.files.retained_classes[0].ast);
    EXPECT_FALSE(result.ast->public_members);
    EXPECT_TRUE(result.files.retained_classes[0].ast->public_members);
}

TEST(TestFormulaEntry, loadFormulaKeepsImportedAstsOutOfMainAst)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z=z+1\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };
    Options options;
    options.source_filename = "main.ufm";
    options.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };

    LoadedFormula result{load_formula("import \"common.ulb\"\n"
                                      "z=0:z=z+1,|z|<4",
        options)};

    ASSERT_TRUE(result.ast);
    ASSERT_EQ(1U, result.files.retained_classes.size());
    ASSERT_TRUE(result.files.retained_classes[0].ast);
    EXPECT_NE(result.ast, result.files.retained_classes[0].ast);
    EXPECT_FALSE(result.ast->public_members);
    EXPECT_TRUE(result.files.retained_classes[0].ast->public_members);
}

TEST(TestFormulaEntry, loadFormulaIncludesUnresolvedReferenceDiagnostics)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "global:\n"
            "Missing missing\n"
            "loop:\n"
            "z=z+1\n"
            "}\n"},
    };
    Options options;
    options.source_filename = "main.ufm";
    options.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };

    LoadedFormula result{load_formula("global:\n"
                                      "Missing missing\n"
                                      "loop:\n"
                                      "z=z+1",
        options)};

    ASSERT_TRUE(result.ast);
    EXPECT_TRUE(result.files.resolved_references.empty());
    ASSERT_EQ(1U, result.files.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.files.diagnostics[0].code);
    EXPECT_EQ("Missing", result.files.diagnostics[0].detail);
}

TEST(TestFormulaEntry, loadFormulaIgnoresUnreferencedImportDiagnostics)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"unused.ulb\"\n"
            "loop:\n"
            "z=z+1\n"
            "}\n"},
        {"unused.ulb",
            "class Unused {\n"
            "public:\n"
            "Missing missing\n"
            "}\n"},
    };
    Options options;
    options.source_filename = "main.ufm";
    options.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };

    LoadedFormula result{load_formula("import \"unused.ulb\"\n"
                                      "z=0:z=z+1,|z|<4",
        options)};

    ASSERT_TRUE(result.ast);
    EXPECT_TRUE(result.files.diagnostics.empty());
    EXPECT_TRUE(result.files.retained_classes.empty());
}

TEST(TestFormulaEntry, loadFormulaWithoutImporterOnlyParsesMainAst)
{
    LoadedFormula result{load_formula("z=0:z=z+1,|z|<4", Options{})};

    ASSERT_TRUE(result.ast);
    EXPECT_TRUE(result.files.files.empty());
    EXPECT_TRUE(result.files.class_index.empty());
    EXPECT_TRUE(result.files.diagnostics.empty());
}

TEST(TestFormulaEntry, referenceCollectorFindsDeclarationAndNewTypes)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "Texture tex = new Texture(1)\n"
                                               "int count = 0\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(2U, references.size());
    EXPECT_EQ(FormulaReferenceKind::DECLARATION, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::NEW_OBJECT, references[1].kind);
    EXPECT_EQ("Texture", references[1].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsFunctionSignatureTypes)
{
    ast::FormulaSectionsPtr ast{parse_extended("public:\n"
                                               "Texture func make(Texture source, int count)\n"
                                               "return source\n"
                                               "endfunc\n",
        EntryKind::CLASS)};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(2U, references.size());
    EXPECT_EQ(FormulaReferenceKind::FUNCTION_RETURN, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::FUNCTION_ARGUMENT, references[1].kind);
    EXPECT_EQ("Texture", references[1].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsBaseClass)
{
    ast::FormulaSectionsPtr ast{parse_extended("public:\n"
                                               "int value\n",
        EntryKind::CLASS)};
    ClassHeader header{"Derived", "base.ulb", "Texture", 0};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(header, *ast)};

    ASSERT_EQ(1U, references.size());
    EXPECT_EQ(FormulaReferenceKind::BASE_CLASS, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsClassParamBlocks)
{
    ast::FormulaSectionsPtr ast{parse_extended("default:\n"
                                               "Bailout param bailoutMethod\n"
                                               "endparam\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(1U, references.size());
    EXPECT_EQ(FormulaReferenceKind::PARAM_BLOCK, references[0].kind);
    EXPECT_EQ("Bailout", references[0].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsClassParamDefault)
{
    ast::FormulaSectionsPtr ast{parse_extended("default:\n"
                                               "Bailout param bailoutMethod\n"
                                               "default=TrapBailout\n"
                                               "endparam\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(2U, references.size());
    EXPECT_EQ(FormulaReferenceKind::PARAM_BLOCK, references[0].kind);
    EXPECT_EQ("Bailout", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::PARAM_DEFAULT, references[1].kind);
    EXPECT_EQ("TrapBailout", references[1].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsClassMemberAccess)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "Texture.value\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(1U, references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsClassMethodAccess)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "Texture.value()\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(1U, references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
}

TEST(TestFormulaEntry, referenceCollectorUsesUppercaseClassMemberHeuristic)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "int Texture\n"
                                               "Texture.value\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(1U, references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsNestedClassMemberTarget)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "Outer.Inner.value\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(1U, references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[0].kind);
    EXPECT_EQ("Outer", references[0].class_name);
}

TEST(TestFormulaEntry, referenceCollectorFindsPlainAndTargetedClassMembers)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "Texture.value\n"
                                               "Texture.size()\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(2U, references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[1].kind);
    EXPECT_EQ("Texture", references[1].class_name);
}

TEST(TestFormulaEntry, referenceCollectorPreservesDuplicateClassMemberReferences)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "Texture.value\n"
                                               "Texture.value\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    ASSERT_EQ(2U, references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[0].kind);
    EXPECT_EQ("Texture", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, references[1].kind);
    EXPECT_EQ("Texture", references[1].class_name);
}

TEST(TestFormulaEntry, referenceCollectorIgnoresLowercaseObjectMembers)
{
    ast::FormulaSectionsPtr ast{parse_extended("global:\n"
                                               "int texture\n"
                                               "texture.value\n"
                                               "loop:\n"
                                               "z = pixel\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    EXPECT_TRUE(references.empty());
}

TEST(TestFormulaEntry, referenceCollectorIgnoresBuiltinParamBlocks)
{
    ast::FormulaSectionsPtr ast{parse_extended("default:\n"
                                               "float param bailout\n"
                                               "endparam\n")};

    ASSERT_TRUE(ast);
    const std::vector<FormulaReference> references{collect_formula_references(*ast)};

    EXPECT_TRUE(references.empty());
}

TEST(TestFormulaEntry, entryReferenceCollectorParsesLoadedFormulaEntry)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex = new Texture()\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    FormulaEntryReferences references{collect_formula_entry_references(result, 0, 0)};

    EXPECT_EQ("main.ufm", references.filename);
    EXPECT_EQ(0U, references.file_index);
    EXPECT_EQ(0U, references.entry_index);
    ASSERT_EQ(2U, references.references.size());
    EXPECT_EQ(FormulaReferenceKind::DECLARATION, references.references[0].kind);
    EXPECT_EQ("Texture", references.references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::NEW_OBJECT, references.references[1].kind);
    EXPECT_EQ("Texture", references.references[1].class_name);
    EXPECT_TRUE(result.diagnostics.empty());
}

TEST(TestFormulaEntry, entryReferenceCollectorIncludesClassBase)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Derived(Base) {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    FormulaEntryReferences references{collect_formula_entry_references(result, 0, 0)};

    EXPECT_EQ("main.ufm", references.filename);
    ASSERT_EQ(1U, references.references.size());
    EXPECT_EQ(FormulaReferenceKind::BASE_CLASS, references.references[0].kind);
    EXPECT_EQ("Base", references.references[0].class_name);
    EXPECT_TRUE(result.diagnostics.empty());
}

TEST(TestFormulaEntry, entryReferenceCollectorReportsParseErrors)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Broken {\n"
            "if true\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    FormulaEntryReferences references{collect_formula_entry_references(result, 0, 0)};

    EXPECT_TRUE(references.references.empty());
    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(FormulaFileDiagnosticCode::PARSE_ERROR, result.diagnostics[0].code);
    EXPECT_EQ("main.ufm", result.diagnostics[0].filename);
    EXPECT_EQ("main.ufm", result.diagnostics[0].location.filename);
}

TEST(TestFormulaEntry, fileReferenceCollectorStoresEntryReferences)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex = new Texture()\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture(Base) {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);

    ASSERT_EQ(2U, result.entry_references.size());
    EXPECT_EQ("main.ufm", result.entry_references[0].filename);
    EXPECT_EQ(0U, result.entry_references[0].file_index);
    EXPECT_EQ(0U, result.entry_references[0].entry_index);
    ASSERT_EQ(2U, result.entry_references[0].references.size());
    EXPECT_EQ(FormulaReferenceKind::DECLARATION, result.entry_references[0].references[0].kind);
    EXPECT_EQ("Texture", result.entry_references[0].references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::NEW_OBJECT, result.entry_references[0].references[1].kind);
    EXPECT_EQ("Texture", result.entry_references[0].references[1].class_name);
    EXPECT_EQ("common.ulb", result.entry_references[1].filename);
    ASSERT_EQ(1U, result.entry_references[1].references.size());
    EXPECT_EQ(FormulaReferenceKind::BASE_CLASS, result.entry_references[1].references[0].kind);
    EXPECT_EQ("Base", result.entry_references[1].references[0].class_name);
    EXPECT_TRUE(result.diagnostics.empty());
}

TEST(TestFormulaEntry, referenceResolverFindsLocalClasses)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"
            "Formula {\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.resolved_references.size());
    EXPECT_EQ("main.ufm", result.resolved_references[0].entry.filename);
    EXPECT_EQ(1U, result.resolved_references[0].entry.entry_index);
    EXPECT_EQ("Texture", result.resolved_references[0].reference.class_name);
    EXPECT_EQ("Texture", result.resolved_references[0].klass.class_name);
    EXPECT_EQ(0U, result.resolved_references[0].klass.file_index);
}

TEST(TestFormulaEntry, referenceResolverFindsImportedClasses)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.resolved_references.size());
    EXPECT_EQ("Texture", result.resolved_references[0].reference.class_name);
    EXPECT_EQ("Texture", result.resolved_references[0].klass.class_name);
    EXPECT_EQ("common.ulb", result.resolved_references[0].klass.filename);
}

TEST(TestFormulaEntry, retainResolvedImportedClassesRetainsDirectImports)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    ASSERT_EQ(1U, result.retained_classes.size());
    EXPECT_EQ("Texture", result.retained_classes[0].reference.class_name);
    EXPECT_EQ("common.ulb", result.retained_classes[0].reference.filename);
    ASSERT_TRUE(result.retained_classes[0].ast);
    EXPECT_TRUE(result.retained_classes[0].ast->public_members);
}

TEST(TestFormulaEntry, retainResolvedImportedClassesIgnoresLocalClasses)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"
            "Formula {\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    EXPECT_TRUE(result.retained_classes.empty());
}

TEST(TestFormulaEntry, retainResolvedImportedClassesSkipsUnreferencedImports)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "import \"unused.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
        {"unused.ulb",
            "class Filter {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    ASSERT_EQ(1U, result.retained_classes.size());
    EXPECT_EQ("Texture", result.retained_classes[0].reference.class_name);
}

TEST(TestFormulaEntry, retainResolvedImportedClassesRetainsNestedImports)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "import \"filters.ulb\"\n"
            "public:\n"
            "Filter filter\n"
            "}\n"},
        {"filters.ulb",
            "class Filter {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    ASSERT_EQ(2U, result.retained_classes.size());
    EXPECT_EQ("Texture", result.retained_classes[0].reference.class_name);
    EXPECT_EQ("Filter", result.retained_classes[1].reference.class_name);
}

TEST(TestFormulaEntry, retainResolvedImportedClassesRetainsSameFileBaseClass)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Derived.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Base {\n"
            "public:\n"
            "int answer\n"
            "}\n"
            "class Derived(Base) {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    ASSERT_EQ(2U, result.retained_classes.size());
    EXPECT_EQ("Derived", result.retained_classes[0].reference.class_name);
    EXPECT_EQ("Base", result.retained_classes[0].base_class);
    EXPECT_EQ("Base", result.retained_classes[1].reference.class_name);
    EXPECT_TRUE(result.retained_classes[1].base_class.empty());
}

TEST(TestFormulaEntry, retainResolvedImportedClassesStoresExplicitImportedBaseClass)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Derived.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Derived(base.ulb:Base) {\n"
            "public:\n"
            "int value\n"
            "}\n"},
        {"base.ulb",
            "class Base {\n"
            "public:\n"
            "int answer\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    ASSERT_EQ(2U, result.retained_classes.size());
    EXPECT_EQ("Derived", result.retained_classes[0].reference.class_name);
    EXPECT_EQ("Base", result.retained_classes[0].base_class);
    EXPECT_EQ("Base", result.retained_classes[1].reference.class_name);
    EXPECT_TRUE(result.retained_classes[1].base_class.empty());
}

TEST(TestFormulaEntry, retainResolvedImportedClassesDoesNotDuplicateReferenceKinds)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex = new Texture()\n"
            "Texture.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int answer\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);

    ASSERT_EQ(1U, result.retained_classes.size());
    EXPECT_EQ("Texture", result.retained_classes[0].reference.class_name);
    ASSERT_TRUE(result.retained_classes[0].ast);
    EXPECT_TRUE(result.retained_classes[0].ast->public_members);
}

TEST(TestFormulaEntry, retainResolvedImportedClassesIsRepeatable)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "import \"filters.ulb\"\n"
            "public:\n"
            "Filter filter\n"
            "}\n"},
        {"filters.ulb",
            "class Filter {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    retain_resolved_imported_classes(result);
    const std::size_t retained_count{result.retained_classes.size()};
    const std::size_t resolved_count{result.resolved_references.size()};
    retain_resolved_imported_classes(result);

    EXPECT_EQ(retained_count, result.retained_classes.size());
    EXPECT_EQ(resolved_count, result.resolved_references.size());
}

TEST(TestFormulaEntry, unresolvedClassMemberReferenceReportsDiagnostic)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "global:\n"
            "Missing.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.diagnostics[0].code);
    EXPECT_EQ("Missing", result.diagnostics[0].detail);
}

TEST(TestFormulaEntry, unresolvedClassMemberReferenceKeepsLocationFilename)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "global:\n"
            "Missing.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.diagnostics[0].code);
    EXPECT_EQ("main.ufm", result.diagnostics[0].filename);
    EXPECT_EQ("main.ufm", result.diagnostics[0].location.filename);
    EXPECT_EQ("Missing", result.diagnostics[0].detail);
}

TEST(TestFormulaEntry, missingClassMemberImportReportsDiagnostic)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"missing.ulb\"\n"
            "global:\n"
            "Missing.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree("main.ufm",
        [&files](std::string_view filename) -> std::optional<std::string>
        {
            const auto found = files.find(std::string{filename});
            if (found == files.end())
            {
                return {};
            }
            return found->second;
        })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::MISSING_IMPORT, result.diagnostics[0].code);
    EXPECT_EQ("missing.ulb", result.diagnostics[0].filename);
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(2U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.diagnostics[1].code);
    EXPECT_EQ("Missing", result.diagnostics[1].detail);
}

TEST(TestFormulaEntry, referenceResolverUsesLastExplicitImportFirst)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"first.ulb\"\n"
            "import \"second.ulb\"\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"first.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
        {"second.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.resolved_references.size());
    EXPECT_EQ("second.ulb", result.resolved_references[0].klass.filename);
}

TEST(TestFormulaEntry, referenceResolverUsesLastExplicitImportFirstForClassMembers)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"first.ulb\"\n"
            "import \"second.ulb\"\n"
            "global:\n"
            "Texture.answer\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
        {"first.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
        {"second.ulb",
            "class Texture {\n"
            "public:\n"
            "int answer\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.resolved_references.size());
    EXPECT_EQ(FormulaReferenceKind::CLASS_MEMBER, result.resolved_references[0].reference.kind);
    EXPECT_EQ("second.ulb", result.resolved_references[0].klass.filename);
}

TEST(TestFormulaEntry, referenceResolverUsesImplicitImportBeforeCurrentFile)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Base {\n"
            "public:\n"
            "int local\n"
            "}\n"
            "class Derived(base.ulb:Base) {\n"
            "public:\n"
            "int value\n"
            "}\n"},
        {"base.ulb",
            "class Base {\n"
            "public:\n"
            "int imported\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.resolved_references.size());
    EXPECT_EQ("Base", result.resolved_references[0].reference.class_name);
    EXPECT_EQ("base.ulb", result.resolved_references[0].klass.filename);
}

TEST(TestFormulaEntry, referenceResolverReportsUnresolvedClasses)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "global:\n"
            "Texture tex\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    EXPECT_TRUE(result.resolved_references.empty());
    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.diagnostics[0].code);
    EXPECT_EQ("main.ufm", result.diagnostics[0].filename);
    EXPECT_EQ("main.ufm", result.diagnostics[0].location.filename);
    EXPECT_EQ("Texture", result.diagnostics[0].detail);
}

TEST(TestFormulaEntry, referenceResolverReportsOnlyUnresolvedClasses)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"
            "Formula {\n"
            "global:\n"
            "Texture tex\n"
            "Missing missing\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);

    ASSERT_EQ(1U, result.resolved_references.size());
    EXPECT_EQ("Texture", result.resolved_references[0].klass.class_name);
    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.diagnostics[0].code);
    EXPECT_EQ("Missing", result.diagnostics[0].detail);
}

TEST(TestFormulaEntry, referenceResolverIsIdempotent)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "global:\n"
            "Texture first\n"
            "Texture second\n"
            "loop:\n"
            "z = pixel\n"
            "}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_TRUE(result.diagnostics.empty());
    collect_formula_file_references(result);
    resolve_formula_file_references(result);
    resolve_formula_file_references(result);

    EXPECT_TRUE(result.resolved_references.empty());
    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::UNRESOLVED_CLASS, result.diagnostics[0].code);
    EXPECT_EQ("Texture", result.diagnostics[0].detail);
}

TEST(TestFormulaEntry, singleLine)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_EQ("XAXIS", entries[0].file_entry.paren_value);
    EXPECT_EQ("float=y", entries[0].file_entry.bracket_value);
    EXPECT_EQ("z=c:z=z*z+c,|z|>4", entries[0].file_entry.body);
}

TEST(TestFormulaEntry, adjacentEntries)
{
    const char *const frm{R"entry(First{1}Second{2})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_EQ(2U, entries.size());
    EXPECT_EQ("First", entries[0].file_entry.name);
    EXPECT_EQ("1", entries[0].file_entry.body);
    EXPECT_EQ("Second", entries[1].file_entry.name);
    EXPECT_EQ("2", entries[1].file_entry.body);
}

TEST(TestFormulaEntry, bodyEndsWithCloseBrace)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{
z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_EQ("XAXIS", entries[0].file_entry.paren_value);
    EXPECT_EQ("float=y", entries[0].file_entry.bracket_value);
    EXPECT_EQ("\n"
              "z=c:z=z*z+c,|z|>4",
        entries[0].file_entry.body);
}

TEST(TestFormulaEntry, commentAfterOpenBrace)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{  ; comment here
z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_EQ("XAXIS", entries[0].file_entry.paren_value);
    EXPECT_EQ("float=y", entries[0].file_entry.bracket_value);
    EXPECT_EQ("  ; comment here\n"
              "z=c:z=z*z+c,|z|>4",
        entries[0].file_entry.body);
}

TEST(TestFormulaEntry, commentBeforeOpenBrace)
{
    const char *const frm{R"entry(; Mandelbrot(XAXIS)[float=y]{  ; comment here
; z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_TRUE(entries.empty()) << "entry was commented out";
}

TEST(TestFormulaEntry, commentBeforeCloseBrace)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{  ; comment here
;} this is a phony close
z=c, frobozz=1964:z=z*z+c,|z|>4
} ; this is the real close)entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty()) << "entry was parsed";
    EXPECT_EQ(1U, entries.size());
    EXPECT_EQ("Mandelbrot", entries[0].file_entry.name);
    EXPECT_NE(entries[0].file_entry.body.find("frobozz"), std::string::npos);
}
