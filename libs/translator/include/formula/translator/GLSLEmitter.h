// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/core/Node.h>

#include <string>

namespace formula::codegen
{

/// Experimental BASIC GLSL emitter.
///
/// This entry point emits debug/example compute shader text. It is not used by
/// the BASIC interpreter or JIT compiler path and is not production-correct.
std::string emit_shader(const ast::FormulaSections &formula);

} // namespace formula::codegen
