// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gradient
{

struct RGBColor
{
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

struct ColorControlPoint
{
    int index;
    RGBColor color;
};

struct OpacityControlPoint
{
    int index;
    std::uint8_t opacity;
};

struct GradientSection
{
    std::string title;
    int rotation{};
    bool smooth{true};
    bool linked{};
    std::vector<ColorControlPoint> points;
};

struct OpacitySection
{
    bool smooth{true};
    int rotation{};
    bool linked{};
    std::vector<OpacityControlPoint> points;
};

struct GradientEntry
{
    std::string name;
    GradientSection gradient;
    OpacitySection opacity;
};

struct GradientFile
{
    std::vector<GradientEntry> entries;
};

GradientFile parse_gradient(std::string_view text);

} // namespace gradient
