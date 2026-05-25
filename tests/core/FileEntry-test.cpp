// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/core/FileEntry.h>

#include <gtest/gtest.h>

#include <sstream>

using namespace formula;

namespace formula::test
{

TEST(TestFileEntry, parsesAdjacentEntries)
{
    std::istringstream input{"One { a=1 }Next Entry(XAXIS)[float=y] { b=2 }"};

    const std::vector<FileEntry> entries{load_file_entries(input)};

    ASSERT_EQ(2U, entries.size());
    EXPECT_EQ("One", entries[0].name);
    EXPECT_EQ(" a=1 ", entries[0].body);
    EXPECT_EQ("Next Entry", entries[1].name);
    EXPECT_EQ("XAXIS", entries[1].paren_value);
    EXPECT_EQ("float=y", entries[1].bracket_value);
    EXPECT_EQ(" b=2 ", entries[1].body);
}

TEST(TestFileEntry, parsesHeaderSyntaxElements)
{
    std::istringstream input{"Name(paren-stuff)[bracket-stuff] { body }"};

    const std::vector<FileEntry> entries{load_file_entries(input)};

    ASSERT_EQ(1U, entries.size());
    EXPECT_EQ("Name", entries[0].name);
    EXPECT_EQ("paren-stuff", entries[0].paren_value);
    EXPECT_EQ("bracket-stuff", entries[0].bracket_value);
}

TEST(TestFileEntry, treatsCommentsAsWhitespaceInEntryNames)
{
    std::istringstream input{"; before\n"
                             "Name ; middle\n"
                             " { body }"};

    const std::vector<FileEntry> entries{load_file_entries(input)};

    ASSERT_EQ(1U, entries.size());
    EXPECT_EQ("Name", entries[0].name);
}

TEST(TestFileEntry, ignoresCloseBraceInComments)
{
    std::istringstream input{"Name { keep\n"
                             "; } comment\n"
                             "more\n"
                             "} Tail { done }"};

    const std::vector<FileEntry> entries{load_file_entries(input)};

    ASSERT_EQ(2U, entries.size());
    EXPECT_NE(std::string::npos, entries[0].body.find("more"));
    EXPECT_EQ("Tail", entries[1].name);
}

TEST(TestFileEntry, ignoresCloseBraceInQuotedStrings)
{
    std::istringstream input{"Name { title=\"not } done\" value=1 }"};

    const std::vector<FileEntry> entries{load_file_entries(input)};

    ASSERT_EQ(1U, entries.size());
    EXPECT_EQ(" title=\"not } done\" value=1 ", entries[0].body);
}

TEST(TestFileEntry, recordsSourceRanges)
{
    std::istringstream input{"\n"
                             "Entry {\n"
                             "body\n"
                             "}"};

    const std::vector<FileEntry> entries{load_file_entries(input, "test.ufm")};

    ASSERT_EQ(1U, entries.size());
    EXPECT_EQ("test.ufm", entries[0].source_range.begin.filename);
    EXPECT_EQ(2U, entries[0].source_range.begin.line);
    EXPECT_EQ(1U, entries[0].source_range.begin.column);
    EXPECT_EQ(entries[0].header_range, entries[0].name_range);
    EXPECT_EQ(2U, entries[0].body_range.begin.line);
    EXPECT_EQ(8U, entries[0].body_range.begin.column);
    EXPECT_EQ(4U, entries[0].source_range.end.line);
    EXPECT_EQ(2U, entries[0].source_range.end.column);
}

} // namespace formula::test
