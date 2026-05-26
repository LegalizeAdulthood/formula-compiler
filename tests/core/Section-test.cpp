// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/core/Section.h>

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

namespace formula::test
{

TEST(TestSectionParser, splitsColonLabeledSections)
{
    const std::string_view body{"first:\na=1\nsecond:\nb=2\n"};

    const std::vector<Section> sections{split_sections(body)};

    ASSERT_EQ(2U, sections.size());
    EXPECT_EQ("first", sections[0].name);
    EXPECT_EQ("a=1\n", sections[0].body);
    EXPECT_EQ("second", sections[1].name);
    EXPECT_EQ("b=2\n", sections[1].body);
}

TEST(TestSectionParser, dropsTextBeforeFirstSection)
{
    const std::string_view body{"leading: text\nfirst:\na=1\n"};

    const std::vector<Section> sections{split_sections(body)};

    ASSERT_EQ(1U, sections.size());
    EXPECT_EQ("first", sections[0].name);
    EXPECT_EQ("a=1\n", sections[0].body);
}

TEST(TestSectionParser, leavesColonsInsideBodyLines)
{
    const std::string_view body{"first:\nvalue=\"a:b\"\nnot: a label\nsecond:\n"};

    const std::vector<Section> sections{split_sections(body)};

    ASSERT_EQ(2U, sections.size());
    EXPECT_EQ("value=\"a:b\"\nnot: a label\n", sections[0].body);
    EXPECT_EQ("", sections[1].body);
}

TEST(TestSectionParser, acceptsHyphenatedSectionNames)
{
    const std::string_view body{"one-two_3:\nbody\n"};

    const std::vector<Section> sections{split_sections(body)};

    ASSERT_EQ(1U, sections.size());
    EXPECT_EQ("one-two_3", sections[0].name);
    EXPECT_EQ("body\n", sections[0].body);
}

TEST(TestSectionParser, preservesCrLfInSectionBodies)
{
    const std::string_view body{"first:\r\na=1\r\nsecond:\r\nb=2\r\n"};

    const std::vector<Section> sections{split_sections(body)};

    ASSERT_EQ(2U, sections.size());
    EXPECT_EQ("a=1\r\n", sections[0].body);
    EXPECT_EQ("b=2\r\n", sections[1].body);
}

TEST(TestSectionParser, recordsSourceRanges)
{
    const std::string_view body{"  first:\r\nbody\r\n  second:\n"};
    SourceLocation start;
    start.filename = "test.ufm";
    start.line = 10;
    start.column = 5;

    const std::vector<Section> sections{split_sections(body, start)};

    ASSERT_EQ(2U, sections.size());
    EXPECT_EQ("test.ufm", sections[0].name_range.begin.filename);
    EXPECT_EQ(10U, sections[0].name_range.begin.line);
    EXPECT_EQ(7U, sections[0].name_range.begin.column);
    EXPECT_EQ(10U, sections[0].name_range.end.line);
    EXPECT_EQ(12U, sections[0].name_range.end.column);
    EXPECT_EQ(11U, sections[0].body_range.begin.line);
    EXPECT_EQ(1U, sections[0].body_range.begin.column);
    EXPECT_EQ(12U, sections[0].body_range.end.line);
    EXPECT_EQ(1U, sections[0].body_range.end.column);
    EXPECT_EQ(12U, sections[1].name_range.begin.line);
    EXPECT_EQ(3U, sections[1].name_range.begin.column);
    EXPECT_EQ(13U, sections[1].body_range.begin.line);
    EXPECT_EQ(1U, sections[1].body_range.begin.column);
}

} // namespace formula::test
