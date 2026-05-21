// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/Formula.h>
#include <formula/FormulaEntry.h>
#include <formula/ParseOptions.h>
#include <formula/Parser.h>
#include <formula/ReferenceCollector.h>

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
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_EQ("XAXIS", entries[0].paren_value);
    EXPECT_TRUE(entries[0].bracket_value.empty());
    EXPECT_EQ("\n"
              "\n",
        entries[0].body);
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
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_TRUE(entries[0].paren_value.empty());
    EXPECT_EQ("float=y", entries[0].bracket_value);
    EXPECT_EQ("\n"
              "\n",
        entries[0].body);
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
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_EQ("XAXIS", entries[0].paren_value);
    EXPECT_EQ("float=y", entries[0].bracket_value);
    EXPECT_EQ("\n"
              "\n",
        entries[0].body);
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
    EXPECT_EQ("Derived", entries[0].name);
    EXPECT_EQ("Test.ufm:Base", entries[0].paren_value);
    EXPECT_TRUE(entries[0].is_class);
    EXPECT_EQ("\n"
              "public:\n"
              "\n",
        entries[0].body);
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
        {"main.ufm", "class Derived(base.ufm:Base) {\n}\n"},
        {"base.ufm", "class Base {\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"common.ulb\"\nz=0:z=z+1,|z|<4\n}\n"},
        {"common.ulb", "class Texture {\npublic:\nint value\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"common.ulb\"\nz=0:z=z+1,|z|<4\n}\n"},
        {"common.ulb", "class Texture {\npublic:\nint value\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"common.ulb\"\nz=0:z=z+1,|z|<4\n}\n"},
        {"common.ulb", "class Texture {\npublic:\nint value\n}\n"},
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
        {"main.ufm", "class Broken {\nif true\nint value\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"first.ulb\"\nz=0:z=z+1,|z|<4\n}\n"},
        {"first.ulb", "class First {\nimport \"second.ulb\"\npublic:\nint value\n}\n"},
        {"second.ulb", "class Second {\npublic:\nint value\n}\n"},
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
        {"main.ufm", "Formula {\n$ifdef nope\nimport \"skip.ulb\"\n$endif\nz=0:z=z+1,|z|<4\n}\n"},
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
        {"main.ufm", "Formula {\n$ifdef nope\nz=0:z=z+1,|z|<4\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"bad.ulb\"\nz=0:z=z+1,|z|<4\n}\n"},
        {"bad.ulb", "class Bad {\nimport\npublic:\nint value\n}\n"},
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
        {"main.ufm", "class Derived(missing.ufm:Base) {\n}\n"},
    };

    auto result{load_formula_file_tree(
        "main.ufm", [&files](std::string_view filename) { return files.at(std::string{filename}); })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(FormulaFileDiagnosticCode::MISSING_IMPORT, result.diagnostics[0].code);
    EXPECT_EQ("missing.ufm", result.diagnostics[0].filename);
    ASSERT_EQ(2U, result.diagnostics[0].import_stack.size());
    EXPECT_EQ("main.ufm", result.diagnostics[0].import_stack[0]);
    EXPECT_EQ("missing.ufm", result.diagnostics[0].import_stack[1]);
}

TEST(TestFormulaEntry, fileTreeReportsImportCycle)
{
    std::unordered_map<std::string, std::string> files{
        {"a.ufm", "class A(b.ufm:B) {\n}\n"},
        {"b.ufm", "class B(a.ufm:A) {\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"common.ulb\"\nz=0:z=z+1,|z|<4\n}\n"},
        {"common.ulb", "class Texture {\npublic:\nint value\n}\n"},
    };
    Options options;
    options.source_filename = "main.ufm";
    options.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };

    LoadedFormula result{load_formula("import \"common.ulb\"\nz=0:z=z+1,|z|<4", options)};

    ASSERT_TRUE(result.ast);
    ASSERT_TRUE(result.files.diagnostics.empty());
    ASSERT_EQ(2U, result.files.files.size());
    EXPECT_EQ("main.ufm", result.files.files[0].filename);
    EXPECT_EQ("common.ulb", result.files.files[1].filename);
    ASSERT_EQ(1U, result.files.class_index.size());
    EXPECT_EQ("Texture", result.files.class_index[0].class_name);
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
    EXPECT_EQ("texture", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::NEW_OBJECT, references[1].kind);
    EXPECT_EQ("texture", references[1].class_name);
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
    EXPECT_EQ("texture", references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::FUNCTION_ARGUMENT, references[1].kind);
    EXPECT_EQ("texture", references[1].class_name);
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
    EXPECT_EQ("bailout", references[0].class_name);
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
        {"main.ufm", "Formula {\nimport \"common.ulb\"\nglobal:\nTexture tex = new Texture()\nloop:\nz = pixel\n}\n"},
        {"common.ulb", "class Texture {\npublic:\nint value\n}\n"},
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
    EXPECT_EQ("texture", references.references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::NEW_OBJECT, references.references[1].kind);
    EXPECT_EQ("texture", references.references[1].class_name);
    EXPECT_TRUE(result.diagnostics.empty());
}

TEST(TestFormulaEntry, entryReferenceCollectorIncludesClassBase)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm", "class Derived(Base) {\npublic:\nint value\n}\n"},
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
        {"main.ufm", "class Broken {\nif true\nint value\n}\n"},
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
        {"main.ufm", "Formula {\nimport \"common.ulb\"\nglobal:\nTexture tex = new Texture()\nloop:\nz = pixel\n}\n"},
        {"common.ulb", "class Texture(Base) {\npublic:\nint value\n}\n"},
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
    EXPECT_EQ("texture", result.entry_references[0].references[0].class_name);
    EXPECT_EQ(FormulaReferenceKind::NEW_OBJECT, result.entry_references[0].references[1].kind);
    EXPECT_EQ("texture", result.entry_references[0].references[1].class_name);
    EXPECT_EQ("common.ulb", result.entry_references[1].filename);
    ASSERT_EQ(1U, result.entry_references[1].references.size());
    EXPECT_EQ(FormulaReferenceKind::BASE_CLASS, result.entry_references[1].references[0].kind);
    EXPECT_EQ("Base", result.entry_references[1].references[0].class_name);
    EXPECT_TRUE(result.diagnostics.empty());
}

TEST(TestFormulaEntry, singleLine)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_EQ("XAXIS", entries[0].paren_value);
    EXPECT_EQ("float=y", entries[0].bracket_value);
    EXPECT_EQ("z=c:z=z*z+c,|z|>4", entries[0].body);
}

TEST(TestFormulaEntry, bodyEndsWithCloseBrace)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{
z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_EQ("XAXIS", entries[0].paren_value);
    EXPECT_EQ("float=y", entries[0].bracket_value);
    EXPECT_EQ("\n"
              "z=c:z=z*z+c,|z|>4\n",
        entries[0].body);
}

TEST(TestFormulaEntry, commentAfterOpenBrace)
{
    const char *const frm{R"entry(Mandelbrot(XAXIS)[float=y]{  ; comment here
z=c:z=z*z+c,|z|>4})entry"};
    std::istringstream str{frm};

    auto entries{load_formula_entries(str)};

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_EQ("XAXIS", entries[0].paren_value);
    EXPECT_EQ("float=y", entries[0].bracket_value);
    EXPECT_EQ("  ; comment here\n"
              "z=c:z=z*z+c,|z|>4\n",
        entries[0].body);
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
    EXPECT_EQ("Mandelbrot", entries[0].name);
    EXPECT_NE(entries[0].body.find("frobozz"), std::string::npos);
}
