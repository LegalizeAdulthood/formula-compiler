// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Node.h>

#include <string>

namespace formula::codegen
{

std::string emit_shader(const ast::FormulaSections &formula);

} // namespace formula::codegen
