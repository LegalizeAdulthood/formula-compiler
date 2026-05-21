#include <formula/FormulaEntry.h>

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace formula;
using namespace testing;

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
    ASSERT_EQ(1U, result.files[1].classes.size());
    EXPECT_EQ("Base", result.files[1].classes[0].name);
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
