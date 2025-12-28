// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <cstdlib>

namespace formula
{

struct SourceLocation
{
    std::size_t line{1};
    std::size_t column{1};
};

inline bool operator==(const SourceLocation &lhs, const SourceLocation &rhs)
{
    return lhs.line == rhs.line && lhs.column == rhs.column;
}

inline bool operator!=(const SourceLocation &lhs, const SourceLocation &rhs)
{
    return !(lhs == rhs);
}

} // namespace formula
