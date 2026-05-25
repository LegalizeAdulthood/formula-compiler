# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(test-formula-core OBJECT
    FileEntry-test.cpp
    Value-test.cpp
)
configure_formula_test_library(test-formula-core)
