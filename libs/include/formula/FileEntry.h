// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/SourceLocation.h>

#include <iosfwd>
#include <string>
#include <vector>

namespace formula
{

struct SourceRange
{
    SourceLocation begin;
    SourceLocation end;
};

inline bool operator==(const SourceRange &lhs, const SourceRange &rhs)
{
    return lhs.begin == rhs.begin && lhs.end == rhs.end;
}

struct FileEntry
{
    std::string name;
    std::string paren_value;
    std::string bracket_value;
    std::string body;
    SourceRange source_range;
    SourceRange header_range;
    SourceRange name_range;
    SourceRange body_range;
};

std::vector<FileEntry> load_file_entries(std::istream &in, std::string filename = {});

} // namespace formula
