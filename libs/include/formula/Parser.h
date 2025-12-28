// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace formula::parser
{
struct Options;

enum class ErrorCode
{
    NONE = 0,
    BUILTIN_VARIABLE_ASSIGNMENT,
    BUILTIN_FUNCTION_ASSIGNMENT,
    EXPECTED_PRIMARY,
    INVALID_TOKEN,
    INVALID_SECTION,
    INVALID_SECTION_ORDER,
    DUPLICATE_SECTION,
    INVALID_DEFAULT_METHOD,
    BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS,
};

struct Diagnostic
{
    ErrorCode code{};
    size_t position{};
};

class Parser
{
public:
    virtual ~Parser() = default;

    virtual ast::FormulaSectionsPtr parse() = 0;

    virtual const std::vector<Diagnostic> &get_warnings() const = 0;
    virtual const std::vector<Diagnostic> &get_errors() const = 0;
};

using ParserPtr = std::shared_ptr<Parser>;

ParserPtr create_parser(std::string_view text, const Options &options);

inline ast::FormulaSectionsPtr parse(std::string_view text, const Options &options)
{
    return create_parser(text, options)->parse();
}

} // namespace formula::parser
