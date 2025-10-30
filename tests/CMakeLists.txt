# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable("test-formula"
    "node-builders.h"
    "NodeFormatter.h" "NodeFormatter.cpp"
    "function-call.h" "function-call.cpp"
    "compile-test.cpp"
    "interpreter-test.cpp"
    "parse-test.cpp"
    "simplifier-test.cpp"
)
target_link_libraries(test-formula PUBLIC formula GTest::gtest_main)
target_folder(test-formula "Tests")

gtest_discover_tests(test-formula)
