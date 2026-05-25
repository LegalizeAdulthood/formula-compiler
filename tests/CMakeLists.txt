# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025-2026 Richard Thomson
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

function(configure_formula_test_library target)
    target_link_libraries("${target}" PUBLIC test-formula-util GTest::gtest)
    target_folder("${target}" "Tests/Libraries")
endfunction()

add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(interpreter)
add_subdirectory(parser)
add_subdirectory(facade)
add_subdirectory(semantics)
add_subdirectory(translator)
add_subdirectory(util)

add_executable("test-formula")
target_link_libraries("test-formula" PRIVATE
    test-formula-compiler
    test-formula-core
    test-formula-facade
    test-formula-interpreter
    test-formula-parser
    test-formula-semantics
    test-formula-translator
    test-formula-util
    GTest::gtest_main
)
target_folder("test-formula" "Tests")

gtest_discover_tests("test-formula")
