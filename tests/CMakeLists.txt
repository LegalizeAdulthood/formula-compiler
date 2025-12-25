# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

# Build the formula data generator tool
add_executable(generate-formula-data "generate-formula-data.cpp" )
target_link_libraries(generate-formula-data PUBLIC formula)
target_folder(generate-formula-data "Tools")

# Generate formula data from id.frm at build time
set(ID_FRM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/id.frm")
set(FORMULA_DATA_H "${CMAKE_CURRENT_BINARY_DIR}/formula-data.h")
set(FORMULA_DATA_CPP "${CMAKE_CURRENT_BINARY_DIR}/formula-data.cpp")

add_custom_command(
    OUTPUT "${FORMULA_DATA_H}" "${FORMULA_DATA_CPP}"
    COMMAND generate-formula-data
        "${ID_FRM_FILE}"
        "${FORMULA_DATA_H}"
        "${FORMULA_DATA_CPP}"
    DEPENDS ${ID_FRM_FILE} generate-formula-data
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating formula-data.h and formula-data.cpp from id.frm"
    VERBATIM
)

add_executable("test-formula"
    "ExpressionParam.h" "ExpressionParam.cpp"
    "node-builders.h"
    "${FORMULA_DATA_H}" "${FORMULA_DATA_CPP}"
    "formula-data-test.cpp"
    "NodeFormatter.h" "NodeFormatter.cpp"
    "function-call.h" "function-call.cpp"
    "compile-test.cpp"
    "interpreter-test.cpp"
    "lexer-test.cpp"
    "parse-id-formulas-test.cpp"
    "parse-test.cpp"
    "simplifier-test.cpp"
    "trim_ws.h" "trim_ws.cpp"
)
target_include_directories("test-formula" PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries("test-formula" PUBLIC formula GTest::gtest_main)
target_folder("test-formula" "Tests")

gtest_discover_tests("test-formula")
