# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
set(TEST_DATA_H "${CMAKE_CURRENT_BINARY_DIR}/include/formula/test/test_data.h")

string(CONCAT content
    "#pragma once\n"
    "\n"
    "#include <string_view>\n"
    "\n"
    "namespace formula::test\n"
    "{\n"
    "constexpr std::string_view TEST_DATA = R\"data(${TEST_DATA_DIR})data\";\n"
    "}\n"
)

file(GENERATE
    OUTPUT "${TEST_DATA_H}"
    CONTENT "${content}"
)

add_library(test-formula-parser OBJECT
    FormulaEntry-test.cpp
    Gradient-test.cpp
    lexer-test.cpp
    Parameter-test.cpp
    parse-test.cpp
    Preprocessor-test.cpp
    "${TEST_DATA_H}"
)

target_include_directories(test-formula-parser PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
configure_formula_test_library(test-formula-parser)
