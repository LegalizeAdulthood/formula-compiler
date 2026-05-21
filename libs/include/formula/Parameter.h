// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/Dialect.h>
#include <formula/FileEntry.h>
#include <formula/SourceLocation.h>

#include <string>
#include <string_view>
#include <vector>

namespace formula::parameter
{

enum class ParseErrorCode
{
    NONE = 0,
    EXPECTED_SECTION_LABEL,
    EXPECTED_ASSIGNMENT,
    EXPECTED_VALUE,
    UNTERMINATED_QUOTED_STRING,
};

std::string to_string(ParseErrorCode code);

struct ParseDiagnostic
{
    ParseErrorCode code{};
    SourceLocation location;
};

struct Parameter
{
    std::string key;
    std::string value;
};

struct ParameterSection
{
    std::string name;
    std::vector<Parameter> assignments;
};

struct ParameterBody
{
    std::vector<Parameter> assignments;
    std::vector<ParameterSection> sections;
};

struct ParameterEntry
{
    ParameterBody body;
    std::vector<ParseDiagnostic> diagnostics;
};

ParameterEntry parse_parameter_entry(FileEntry file_entry, Dialect dialect);

} // namespace formula::parameter
