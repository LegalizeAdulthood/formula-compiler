// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/parser/Gradient.h>

#include <test_data.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

using namespace gradient;

namespace
{

std::string read_test_data(const std::filesystem::path &filename)
{
    std::ifstream input{std::filesystem::path{std::string{TEST_DATA}} / filename};
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

GradientEntry parse_one_gradient(std::string_view text)
{
    std::istringstream input{std::string{text}};
    std::vector<formula::FileEntry> entries = formula::load_file_entries(input);
    return parse_gradient(entries.at(0));
}

testing::AssertionResult has_control_point(
    const ColorControlPoint &point, int index, std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    if (point.index != index)
    {
        return testing::AssertionFailure() << "index: expected " << index << ", got " << point.index;
    }
    if (point.color.red != red)
    {
        return testing::AssertionFailure() << "red: expected " << +red << ", got " << +point.color.red;
    }
    if (point.color.green != green)
    {
        return testing::AssertionFailure() << "green: expected " << +green << ", got " << +point.color.green;
    }
    if (point.color.blue != blue)
    {
        return testing::AssertionFailure() << "blue: expected " << +blue << ", got " << +point.color.blue;
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult has_control_point(const OpacityControlPoint &point, int index, std::uint8_t opacity)
{
    if (point.index != index)
    {
        return testing::AssertionFailure() << "index: expected " << index << ", got " << point.index;
    }
    if (point.opacity != opacity)
    {
        return testing::AssertionFailure() << "opacity: expected " << +opacity << ", got " << +point.opacity;
    }
    return testing::AssertionSuccess();
}

} // namespace

TEST(TestGradientParser, parsesGradientSectionKeys)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  title="Vivid"
  smooth=no
  rotation=-12
  linked=yes
  index=0 color=8547071
opacity:
  index=0 opacity=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    const GradientSection &section = entry.gradient;
    EXPECT_EQ("Vivid", section.title);
    EXPECT_FALSE(section.smooth);
    EXPECT_EQ(-12, section.rotation);
    EXPECT_TRUE(section.linked);
}

TEST(TestGradientParser, keepsSemicolonInsideQuotedTitle)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  title="Vivid; not a comment"
  index=0 color=0
opacity:
  index=0 opacity=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    EXPECT_EQ("Vivid; not a comment", entry.gradient.title);
}

TEST(TestGradientParser, parsesGradientControlPoints)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  index=-1 color=8547071
  index=42 color=0
  index=42 color=16777215
opacity:
  index=0 opacity=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    const std::vector<ColorControlPoint> &points = entry.gradient.points;
    ASSERT_EQ(3U, points.size());
    EXPECT_TRUE(has_control_point(points[0], -1, 0x82, 0x6A, 0xFF));
    EXPECT_TRUE(has_control_point(points[1], 42, 0, 0, 0));
    EXPECT_TRUE(has_control_point(points[2], 42, 0xFF, 0xFF, 0xFF));
}

TEST(TestGradientParser, parsesGradientNumNodes)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  numnodes=2 index=0 color=0 index=100 color=16777215
opacity:
  index=0 opacity=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    const GradientSection &section = entry.gradient;
    ASSERT_EQ(2U, section.points.size());
    EXPECT_TRUE(has_control_point(section.points[0], 0, 0, 0, 0));
    EXPECT_TRUE(has_control_point(section.points[1], 100, 0xFF, 0xFF, 0xFF));
}

TEST(TestGradientParser, rejectsGradientNumNodesMismatch)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  numnodes=2 index=0 color=0
opacity:
  index=0 opacity=255
}
)ugr"};

    EXPECT_THROW(parse_one_gradient(ugr), std::runtime_error);
}

TEST(TestGradientParser, parsesOpacitySection)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  index=0 color=0
opacity:
  smooth=no
  rotation=7
  linked=yes
  index=-66 opacity=0
  index=399 opacity=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    const OpacitySection &section = entry.opacity;
    EXPECT_FALSE(section.smooth);
    EXPECT_EQ(7, section.rotation);
    EXPECT_TRUE(section.linked);
    ASSERT_EQ(2U, section.points.size());
    EXPECT_TRUE(has_control_point(section.points[0], -66, 0));
    EXPECT_TRUE(has_control_point(section.points[1], 399, 255));
}

TEST(TestGradientParser, parsesOpacityNumNodes)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  index=0 color=0
opacity:
  numnodes=2 index=0 opacity=0 index=399 opacity=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    const OpacitySection &section = entry.opacity;
    ASSERT_EQ(2U, section.points.size());
    EXPECT_TRUE(has_control_point(section.points[0], 0, 0));
    EXPECT_TRUE(has_control_point(section.points[1], 399, 255));
}

TEST(TestGradientParser, rejectsOpacityNumNodesMismatch)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  index=0 color=0
opacity:
  numnodes=2 index=0 opacity=255
}
)ugr"};

    EXPECT_THROW(parse_one_gradient(ugr), std::runtime_error);
}

TEST(TestGradientParser, acceptsAlphaAsOpacitySection)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  index=0 color=0
alpha:
  smooth=no
  numnodes=2 index=0 alpha=0 index=399 alpha=255
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    const OpacitySection &section = entry.opacity;
    EXPECT_FALSE(section.smooth);
    ASSERT_EQ(2U, section.points.size());
    EXPECT_TRUE(has_control_point(section.points[0], 0, 0));
    EXPECT_TRUE(has_control_point(section.points[1], 399, 255));
}

TEST(TestGradientParser, rejectsOpacityKeyInAlphaSection)
{
    const char *const ugr{R"ugr(
Test {
gradient:
  index=0 color=0
alpha:
  index=0 opacity=255
}
)ugr"};

    EXPECT_THROW(parse_one_gradient(ugr), std::runtime_error);
}

TEST(TestGradientParser, acceptsOpacityBeforeGradient)
{
    const char *const ugr{R"ugr(
Test {
opacity:
  index=0 opacity=255
gradient:
  title="Late"
  index=0 color=0
}
)ugr"};

    GradientEntry entry = parse_one_gradient(ugr);

    EXPECT_EQ("Late", entry.gradient.title);
    ASSERT_EQ(1U, entry.opacity.points.size());
    EXPECT_EQ(255, entry.opacity.points[0].opacity);
}

TEST(TestGradientParser, parsesMultipleEntries)
{
    const char *const ugr{R"ugr(
First {
gradient:
  title="One"
  index=0 color=0
opacity:
  index=0 opacity=255
}

Second {
opacity:
  index=0 opacity=128
gradient:
  title="Two"
  index=100 color=16777215
}
)ugr"};

    std::istringstream input{ugr};
    std::vector<formula::FileEntry> file_entries = formula::load_file_entries(input);
    std::vector<GradientEntry> entries;
    for (const formula::FileEntry &file_entry : file_entries)
    {
        entries.push_back(parse_gradient(file_entry));
    }

    ASSERT_EQ(2U, entries.size());
    EXPECT_EQ("First", entries[0].name);
    EXPECT_EQ("One", entries[0].gradient.title);
    ASSERT_EQ(1U, entries[0].opacity.points.size());
    EXPECT_EQ(255, entries[0].opacity.points[0].opacity);
    EXPECT_EQ("Second", entries[1].name);
    EXPECT_EQ("Two", entries[1].gradient.title);
    ASSERT_EQ(1U, entries[1].gradient.points.size());
    EXPECT_EQ(100, entries[1].gradient.points[0].index);
    ASSERT_EQ(1U, entries[1].opacity.points.size());
    EXPECT_EQ(128, entries[1].opacity.points[0].opacity);
}

TEST(TestGradientParser, parsesRainbowGradientFile)
{
    std::istringstream input{read_test_data("rainbow.ugr")};
    std::vector<formula::FileEntry> file_entries = formula::load_file_entries(input);
    std::vector<GradientEntry> entries;
    for (const formula::FileEntry &file_entry : file_entries)
    {
        entries.push_back(parse_gradient(file_entry));
    }

    ASSERT_EQ(25U, entries.size());

    const GradientEntry &first = entries[0];
    EXPECT_EQ("Rainbow-6BrightBars", first.name);
    EXPECT_EQ("Rainbow - 6 bright bars", first.gradient.title);
    EXPECT_TRUE(first.gradient.smooth);
    EXPECT_EQ(0, first.gradient.rotation);
    EXPECT_FALSE(first.gradient.linked);
    ASSERT_EQ(12U, first.gradient.points.size());
    EXPECT_TRUE(has_control_point(first.gradient.points[0], 0, 0, 0, 0));
    EXPECT_TRUE(has_control_point(first.gradient.points[1], 33, 0x82, 0x69, 0xFF));
    EXPECT_TRUE(first.opacity.smooth);
    EXPECT_EQ(3, first.opacity.rotation);
    ASSERT_EQ(3U, first.opacity.points.size());
    EXPECT_TRUE(has_control_point(first.opacity.points[0], 0, 255));
    EXPECT_TRUE(has_control_point(first.opacity.points[2], 266, 255));

    const GradientEntry &columns = entries[3];
    EXPECT_EQ("Rainbow-8Columns", columns.name);
    EXPECT_EQ("Rainbow - 8 columns", columns.gradient.title);
    ASSERT_EQ(25U, columns.gradient.points.size());
    EXPECT_TRUE(has_control_point(columns.gradient.points.back(), -1, 0x8C, 0x00, 0x48));

    const GradientEntry &tropical = entries[20];
    EXPECT_EQ("Rainbow-Tropical30", tropical.name);
    EXPECT_EQ("Rainbow - tropical 30", tropical.gradient.title);
    EXPECT_TRUE(tropical.gradient.linked);
    EXPECT_EQ(3, tropical.gradient.rotation);
    ASSERT_EQ(30U, tropical.gradient.points.size());
    ASSERT_EQ(30U, tropical.opacity.points.size());

    const GradientEntry &last = entries.back();
    EXPECT_EQ("OrangeAndGreenSolid5Desaturated", last.name);
    EXPECT_EQ("Orange and Green solid 5 desaturated", last.gradient.title);
    ASSERT_EQ(20U, last.gradient.points.size());
    EXPECT_TRUE(has_control_point(last.gradient.points.back(), 399, 0x41, 0x41, 0x41));
}
