// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include "ast.h"

#include <asmjit/x86/x86compiler.h>

namespace formula::ast
{

class Node;

bool compile(
    const std::shared_ptr<Node> &expr, asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result);

} // namespace formula::ast
