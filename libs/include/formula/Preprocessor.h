// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/SourceLocation.h>

#include <string>
#include <string_view>
#include <vector>

namespace formula::preprocessor
{

enum class ErrorCode
{
    NONE = 0,
    EXPECTED_DIRECTIVE_ENDIF,
    UNEXPECTED_DIRECTIVE_ELSE,
    UNEXPECTED_DIRECTIVE_ENDIF,
    DUPLICATE_DIRECTIVE_ELSE,
    EXPECTED_DIRECTIVE_SYMBOL,
    INVALID_COMPILER_DIRECTIVE,
};

std::string to_string(ErrorCode code);

struct Diagnostic
{
    ErrorCode code{};
    SourceLocation position;
};

using MacroList = std::vector<std::string>;

enum class UltraFractalMacros
{
    NONE,
    ULTRAFRACTAL3,
    ULTRAFRACTAL4,
    ULTRAFRACTAL5,
    ULTRAFRACTAL6,
};

MacroList predefined_macros(UltraFractalMacros predefined);

class Preprocessor
{
public:
    Preprocessor();
    explicit Preprocessor(std::string source_filename);
    explicit Preprocessor(const MacroList &predefined);
    Preprocessor(const MacroList &predefined, std::string source_filename);
    explicit Preprocessor(UltraFractalMacros predefined);
    Preprocessor(UltraFractalMacros predefined, std::string source_filename);

    std::string process(std::string_view input);

    const MacroList &macros() const
    {
        return m_macros;
    }

    const std::vector<Diagnostic> &errors() const
    {
        return m_errors;
    }

private:
    MacroList m_predefined;
    MacroList m_macros;
    std::string m_source_filename;
    std::vector<Diagnostic> m_errors;
};

} // namespace formula::preprocessor
