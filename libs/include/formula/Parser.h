// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>
#include <formula/SourceLocation.h>

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
    INVALID_TOKEN,
    EXPECTED_PRIMARY,
    EXPECTED_ENDIF,
    EXPECTED_STATEMENT_SEPARATOR,
    EXPECTED_COMMA,
    EXPECTED_OPEN_PAREN,
    EXPECTED_CLOSE_PAREN,
    EXPECTED_CLOSE_MODULUS,
    EXPECTED_IDENTIFIER,
    EXPECTED_ASSIGNMENT,
    EXPECTED_INTEGER,
    EXPECTED_FLOATING_POINT,
    EXPECTED_COMPLEX,
    EXPECTED_STRING,
    EXPECTED_TERMINATOR,
    EXPECTED_STATEMENT,
    UNEXPECTED_ASSIGNMENT,
    BUILTIN_VARIABLE_ASSIGNMENT,
    BUILTIN_FUNCTION_ASSIGNMENT,
    INVALID_SECTION,
    INVALID_SECTION_ORDER,
    DUPLICATE_SECTION,
    BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS,
    BUILTIN_SECTION_INVALID_KEY,
    BUILTIN_SECTION_INVALID_TYPE,
    DEFAULT_SECTION_INVALID_KEY,
    DEFAULT_SECTION_INVALID_METHOD,
    SWITCH_SECTION_INVALID_KEY,
};

std::string to_string(ErrorCode code);

struct Diagnostic
{
    ErrorCode code{};
    SourceLocation position;
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
