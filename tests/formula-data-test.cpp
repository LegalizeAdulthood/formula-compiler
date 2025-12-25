// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Example test demonstrating usage of generated formula data

#include "formula-data.h"

#include <gtest/gtest.h>

namespace formula::test
{

TEST(FormulaDataTest, hasExpectedCount)
{
    const size_t count = g_formula_count;

    EXPECT_GT(count, 0);
}

TEST(FormulaDataTest, allEntriesHaveNamesAndContent)
{
    for (size_t i = 0; i < g_formula_count; ++i)
    {
        const auto &entry = g_formulas[i];
        EXPECT_GT(entry.name.length(), 0) << "Entry " << i << " has empty name";
        EXPECT_GT(entry.body.length(), 0) << "Entry " << i << " has empty content";
    }
}

TEST(FormulaDataTest, canFindMandelbrotFormula)
{
    const char *const search_name = "Mandelbrot";
    bool found = false;

    for (size_t i = 0; i < g_formula_count; ++i)
    {
        if (g_formulas[i].name == search_name)
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "Could not find " << search_name << " in formula data";
}

TEST(FormulaDataTest, noCommentEntries)
{
    for (size_t i = 0; i < g_formula_count; ++i)
    {
        const FormulaEntry &entry = g_formulas[i];
        EXPECT_NE("comment", entry.name) << "Found comment entry at index " << i;
    }
}

} // namespace formula::test
