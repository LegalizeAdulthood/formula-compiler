// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Dialect.h>

namespace formula::parser
{

struct Options
{
    bool allow_builtin_assignment{true};
    Dialect dialect{Dialect::EXTENDED};
};

} // namespace formula::parser
