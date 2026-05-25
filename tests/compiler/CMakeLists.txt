# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(test-formula-compiler OBJECT
    compile-test.cpp
)
configure_formula_test_library(test-formula-compiler)
