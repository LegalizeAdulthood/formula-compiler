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
    EXPECT_FALSE(entries[0].body.empty());
}
