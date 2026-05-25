# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(test-formula-translator OBJECT
    GLSLEmitter-test.cpp
)
configure_formula_test_library(test-formula-translator)
target_link_libraries(test-formula-translator PUBLIC formula::translator)
