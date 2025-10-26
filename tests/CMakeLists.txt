# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable("test-formula"
    "function-call.h" "function-call.cpp"
    "parse-test.cpp"
    "interpret-test.cpp"
    "interpreter-test.cpp"
    "compile-test.cpp"
)
target_link_libraries(test-formula PUBLIC formula GTest::gtest_main)
target_folder(test-formula "Tests")

gtest_discover_tests(test-formula)
