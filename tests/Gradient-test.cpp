#include <formula/Gradient.h>

#include <gtest/gtest.h>

using namespace gradient;

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

    GradientFile file = parse_gradient(ugr);

    ASSERT_EQ(1U, file.entries.size());
    const GradientSection &section = file.entries[0].gradient;
    EXPECT_EQ("Vivid", section.title);
    EXPECT_FALSE(section.smooth);
    EXPECT_EQ(-12, section.rotation);
    EXPECT_TRUE(section.linked);
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

    GradientFile file = parse_gradient(ugr);

    ASSERT_EQ(1U, file.entries.size());
    const auto &points = file.entries[0].gradient.points;
    ASSERT_EQ(3U, points.size());
    EXPECT_EQ(-1, points[0].index);
    EXPECT_EQ(0x82, points[0].color.red);
    EXPECT_EQ(0x6A, points[0].color.green);
    EXPECT_EQ(0xFF, points[0].color.blue);
    EXPECT_EQ(42, points[1].index);
    EXPECT_EQ(0, points[1].color.red);
    EXPECT_EQ(0, points[1].color.green);
    EXPECT_EQ(0, points[1].color.blue);
    EXPECT_EQ(42, points[2].index);
    EXPECT_EQ(0xFF, points[2].color.red);
    EXPECT_EQ(0xFF, points[2].color.green);
    EXPECT_EQ(0xFF, points[2].color.blue);
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

    GradientFile file = parse_gradient(ugr);

    ASSERT_EQ(1U, file.entries.size());
    const OpacitySection &section = file.entries[0].opacity;
    EXPECT_FALSE(section.smooth);
    EXPECT_EQ(7, section.rotation);
    EXPECT_TRUE(section.linked);
    ASSERT_EQ(2U, section.points.size());
    EXPECT_EQ(-66, section.points[0].index);
    EXPECT_EQ(0, section.points[0].opacity);
    EXPECT_EQ(399, section.points[1].index);
    EXPECT_EQ(255, section.points[1].opacity);
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

    GradientFile file = parse_gradient(ugr);

    ASSERT_EQ(1U, file.entries.size());
    EXPECT_EQ("Late", file.entries[0].gradient.title);
    ASSERT_EQ(1U, file.entries[0].opacity.points.size());
    EXPECT_EQ(255, file.entries[0].opacity.points[0].opacity);
}
