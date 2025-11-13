# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

# Generate formula data from id.frm at configure time
set(ID_FRM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/id.frm")
set(GENERATOR_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate-formula-data.cmake")
set(FORMULA_DATA_CPP "${CMAKE_CURRENT_BINARY_DIR}/formula-data.cpp")

add_custom_command(
    OUTPUT "${FORMULA_DATA_CPP}"
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${ID_FRM_FILE}
        -DOUTPUT_FILE=${FORMULA_DATA_CPP}
        -P ${GENERATOR_SCRIPT}
    DEPENDS ${ID_FRM_FILE} ${GENERATOR_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating formula-data.cpp from id.frm"
    VERBATIM
)

add_executable("test-formula"
    "node-builders.h"
    "formula-data.h" "${FORMULA_DATA_CPP}"
    "formula-data-test.cpp"
    "NodeFormatter.h" "NodeFormatter.cpp"
    "function-call.h" "function-call.cpp"
    "compile-test.cpp"
    "interpreter-test.cpp"
    "lexer-test.cpp"
    "parse-test.cpp"
    "simplifier-test.cpp"
    "trim_ws.h" "trim_ws.cpp"
)
target_include_directories("test-formula" PRIVATE .)
target_link_libraries("test-formula" PUBLIC formula GTest::gtest_main)
target_folder("test-formula" "Tests")

gtest_discover_tests("test-formula")
