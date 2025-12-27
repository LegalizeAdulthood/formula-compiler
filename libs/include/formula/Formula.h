// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <memory>
#include <string_view>

namespace formula
{

namespace ast
{
class Node;
using Expr = std::shared_ptr<Node>;
} // namespace ast

namespace parser
{
struct Options;
} // namespace parser

enum class Section
{
    NONE = 0,
    PER_IMAGE,          // global:
    BUILTIN,            // builtin:
    INITIALIZE,         // init:
    ITERATE,            // loop:
    BAILOUT,            // bailout:
    PERTURB_INITIALIZE, // perturbinit:
    PERTURB_ITERATE,    // perturbloop:
    DEFAULT,            // default:
    SWITCH,             // switch:
    NUM_SECTIONS
};

inline int operator+(Section value)
{
    return static_cast<int>(value);
}

class Formula
{
public:
    virtual ~Formula() = default;

    virtual void set_value(std::string_view name, Complex value) = 0;
    virtual Complex get_value(std::string_view name) const = 0;
    virtual const ast::Expr &get_section(Section section) const = 0;
    virtual Complex interpret(Section part) = 0;
    virtual bool compile() = 0;
    virtual Complex run(Section part) = 0;
};

using FormulaPtr = std::shared_ptr<Formula>;

FormulaPtr create_formula(std::string_view text, const parser::Options &options);

} // namespace formula
