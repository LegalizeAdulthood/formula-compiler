// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/FileEntry.h>
#include <formula/Node.h>
#include <formula/SourceLocation.h>

#include <cstddef>
#include <functional>
#include <optional>
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

enum class ParameterReferenceKind
{
    FRACTAL_FORMULA,
    INSIDE_COLORING,
    OUTSIDE_COLORING,
    TRANSFORM,
};

enum class ParameterReferenceErrorCode
{
    NONE = 0,
    MISSING_FILENAME,
    MISSING_ENTRY,
    UNRESOLVED_ENTRY,
    PARSE_ERROR,
    UNKNOWN_PARAMETER,
    MISSING_REQUIRED_PARAMETER,
    TYPE_MISMATCH,
    INVALID_PARAMETER_FORWARD,
};

struct ParameterReferenceSite
{
    ParameterReferenceKind kind{};
    std::size_t layer_index{};
    std::size_t transform_index{};
};

struct ParameterReference
{
    ParameterReferenceSite site;
    std::string filename;
    std::string entry;
    std::vector<Parameter> parameters;
};

struct ParameterResolvedReference
{
    ParameterReference reference;
    FileEntry file_entry;
    ast::FormulaSectionsPtr ast;
};

struct ParameterReferenceDiagnostic
{
    ParameterReferenceErrorCode code{};
    SourceLocation location;
    std::string detail;
};

struct ParameterReferenceSet
{
    std::vector<ParameterReference> references;
    std::vector<ParameterResolvedReference> resolved;
    std::vector<ParameterReferenceDiagnostic> diagnostics;
};

using ParameterEntryResolver =
    std::function<std::optional<FileEntry>(std::string_view filename, std::string_view entry)>;

BasicParameterEntry parse_basic_parameters(FileEntry file_entry);
ExtendedParameterEntry parse_extended_parameters(FileEntry file_entry);
std::vector<ParameterReference> collect_parameter_references(const ExtendedParameterEntry &parameters);
ParameterReferenceSet resolve_parameter_references(
    const ExtendedParameterEntry &parameters, const ParameterEntryResolver &resolver);
std::string compress_parameter_set(std::string_view body);
std::string decompress_parameter_set(std::string_view body);

} // namespace formula::parameter
