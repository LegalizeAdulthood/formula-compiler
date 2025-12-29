// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

namespace formula::parser
{

struct Options
{
    bool allow_builtin_assignment{true};
    bool recognize_extensions{true};
};

} // namespace formula::parser
