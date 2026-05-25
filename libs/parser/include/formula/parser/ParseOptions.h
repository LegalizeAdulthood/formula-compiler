// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/core/Dialect.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace formula::parser
{

enum class EntryKind
{
    FRACTAL,
    COLORING,
    TRANSFORMATION,
    CLASS,
};

struct Options
{
    Dialect dialect{Dialect::EXTENDED};
    EntryKind entry_kind{EntryKind::FRACTAL};
    std::function<std::optional<std::string>(std::string_view)> file_importer;
    std::string source_filename;
};

} // namespace formula::parser
