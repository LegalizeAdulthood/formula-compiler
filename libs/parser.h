// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>

#include <string_view>

namespace formula::parser
{

ast::FormulaSectionsPtr parse(std::string_view text);

} // namespace formula::parser
