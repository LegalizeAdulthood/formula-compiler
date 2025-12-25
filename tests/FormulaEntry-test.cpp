#include <formula/FormulaEntry.h>

#include <gtest/gtest.h>

#include <sstream>

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
