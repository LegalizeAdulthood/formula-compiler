# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(test-formula-interpreter OBJECT
    ExtendedInterpreter-test.cpp
    ExtendedRuntime-test.cpp
    interpreter-test.cpp
)
configure_formula_test_library(test-formula-interpreter)
