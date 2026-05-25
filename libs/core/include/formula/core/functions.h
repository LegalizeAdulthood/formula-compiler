// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/core/Complex.h>

#include <map>
#include <string>
#include <string_view>

namespace formula
{

using RealFunction = double(double);
using ComplexFunction = Complex(const Complex &);

double evaluate(std::string_view name, double value);
Complex evaluate(std::string_view name, Complex value);

RealFunction *lookup_real(std::string_view name);
ComplexFunction *lookup_complex(std::string_view name);
bool is_function_selector(std::string_view name);
bool is_selectable_function(std::string_view name);
std::string select_function(std::string_view name, const std::map<std::string, std::string> &functions);

} // namespace formula
