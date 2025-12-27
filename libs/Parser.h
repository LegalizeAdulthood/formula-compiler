// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>

#include <memory>
#include <string_view>

namespace formula::parser
{
struct Options;

class Parser
{
public:
    virtual ~Parser() = default;

    virtual ast::FormulaSectionsPtr parse() = 0;
};

using ParserPtr = std::shared_ptr<Parser>;

ParserPtr create_parser(std::string_view text, const Options &options);

inline ast::FormulaSectionsPtr parse(std::string_view text, const Options &options)
{
    return create_parser(text, options)->parse();
}

} // namespace formula::parser
