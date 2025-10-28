// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <asmjit/core.h>
#include <asmjit/x86.h>
#include <asmjit/x86/x86compiler.h>

#include <map>
#include <optional>
#include <string>

#define ASMJIT_CHECK(expr_)                       \
    do                                            \
    {                                             \
        if (const asmjit::Error err = expr_; err) \
        {                                         \
            return err;                           \
        }                                         \
    } while (false)

namespace formula::ast
{

class Node;

using SymbolTable = std::map<std::string, Complex>;

struct LabelBinding
{
    asmjit::Label label;
    bool bound;
};

using ConstantBindings = std::map<Complex, LabelBinding>;
using SymbolBindings = std::map<std::string, LabelBinding>;

struct DataSection
{
    asmjit::Section *data{};    // Section for data storage
    ConstantBindings constants; // Map of constants to labels
    SymbolBindings symbols;     // Map of symbols to labels
};

struct EmitterState
{
    SymbolTable symbols;
    DataSection data;
};

using CompileError = std::optional<asmjit::Error>;

CompileError compile(
    const std::shared_ptr<Node> &expr, asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result);

} // namespace formula::ast
