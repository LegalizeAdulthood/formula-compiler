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
    const size_t count = formula_count;

    EXPECT_GT(count, 0);
}

TEST(FormulaDataTest, allEntriesHaveNamesAndContent)
{
    for (size_t i = 0; i < formula_count; ++i)
    {
        const auto &entry = formulas[i];
        EXPECT_GT(entry.name.length(), 0) << "Entry " << i << " has empty name";
        EXPECT_GT(entry.content.length(), 0) << "Entry " << i << " has empty content";
    }
}

TEST(FormulaDataTest, canFindMandelbrotFormula)
{
    const char *searchName = "Mandelbrot(XAXIS)";
    bool found = false;

    for (size_t i = 0; i < formula_count; ++i)
    {
        if (formulas[i].name == searchName)
        {
            found = true;
            break;
        }
    }

    // Assert
    EXPECT_TRUE(found) << "Could not find " << searchName << " in formula data";
}

TEST(FormulaDataTest, noCommentEntries)
{
    // Arrange & Act & Assert
    for (size_t i = 0; i < formula_count; ++i)
    {
        const auto &entry = formulas[i];
        EXPECT_NE("comment", entry.name) << "Found comment entry at index " << i;
    }
}

} // namespace formula::test
