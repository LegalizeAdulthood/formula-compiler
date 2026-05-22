// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/FileEntry.h>
#include <formula/SourceLocation.h>

#include <optional>
#include <string>
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
    INVALID_COMPRESSED_PARAMETER_SET,
    EXPECTED_FRACTAL_SECTION,
    EXPECTED_LAYER_SECTION,
    EXPECTED_PARAMETER_SECTION,
    UNEXPECTED_PARAMETER_SECTION,
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

struct BasicParameterEntry
{
    std::vector<Parameter> assignments;
    std::vector<ParseDiagnostic> diagnostics;
};

struct ParameterLayer
{
    ParameterSection layer;
    ParameterSection mapping;
    std::vector<ParameterSection> transforms;
    ParameterSection formula;
    ParameterSection inside;
    ParameterSection outside;
    ParameterSection gradient;
    std::optional<ParameterSection> opacity;
};

struct ExtendedParameterEntry
{
    ParameterSection fractal;
    std::vector<ParameterLayer> layers;
    std::vector<ParseDiagnostic> diagnostics;
};

BasicParameterEntry parse_basic_parameters(FileEntry file_entry);
ExtendedParameterEntry parse_extended_parameters(FileEntry file_entry);
std::string compress_parameter_set(std::string_view body);
std::string decompress_parameter_set(std::string_view body);

} // namespace formula::parameter
