// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Dialect.h>

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
    bool allow_builtin_assignment{true};
    Dialect dialect{Dialect::EXTENDED};
    EntryKind entry_kind{EntryKind::FRACTAL};
};

} // namespace formula::parser
