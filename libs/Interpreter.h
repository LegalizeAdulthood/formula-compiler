// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <map>
#include <string>

namespace formula::ast
{

class Node;

using Dictionary = std::map<std::string, Complex>;

Complex interpret(const std::shared_ptr<Node> &expr, Dictionary &symbols);

} // namespace formula::ast
