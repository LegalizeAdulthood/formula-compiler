// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/core/FileEntry.h>
#include <formula/core/SourceLocation.h>

#include <string_view>
#include <vector>

namespace formula
{

struct Section
{
    std::string_view name;
    std::string_view body;
    SourceRange name_range;
    SourceRange body_range;
};

std::vector<Section> split_sections(std::string_view body, SourceLocation body_begin = {});

} // namespace formula
