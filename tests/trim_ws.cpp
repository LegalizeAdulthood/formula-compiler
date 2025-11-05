// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "trim_ws.h"

namespace formula::test
{

std::string trim_ws(std::string s)
{
    for (auto nl = s.find('\n'); nl != std::string::npos; nl = s.find('\n', nl))
    {
        s[nl] = ' ';
    }
    while (s.back() == '\n' || s.back() == ' ')
    {
        s.pop_back();
    }
    return s;
}

} // namespace formula::test
