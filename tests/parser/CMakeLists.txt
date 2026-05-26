# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(test-formula-parser OBJECT
    FormulaEntry-test.cpp
    Gradient-test.cpp
    lexer-test.cpp
    Parameter-test.cpp
    parse-test.cpp
    Preprocessor-test.cpp
    "${TEST_DATA_H}"
)

configure_formula_test_library(test-formula-parser)
