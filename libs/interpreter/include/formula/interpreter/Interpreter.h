// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#pragma once

#include <formula/core/Complex.h>

#include <map>
#include <memory>
#include <random>
#include <string>

namespace formula::ast
{

class Node;

using Dictionary = std::map<std::string, Complex>;

Complex interpret(const std::shared_ptr<Node> &expr, Dictionary &symbols);
Complex interpret(
    const std::shared_ptr<Node> &expr, Dictionary &symbols, const std::map<std::string, std::string> &functions);
Complex interpret(const std::shared_ptr<Node> &expr, Dictionary &symbols,
    const std::map<std::string, std::string> &functions, std::mt19937 *random);

} // namespace formula::ast
