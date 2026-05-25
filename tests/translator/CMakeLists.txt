# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(test-formula-translator OBJECT
    GLSLEmitter-test.cpp
)
configure_formula_test_library(test-formula-translator)
target_link_libraries(test-formula-translator PUBLIC formula::facade formula::translator)

find_program(GLSLANG_VALIDATOR glslangValidator)
if (GLSLANG_VALIDATOR)
    target_compile_definitions(test-formula-translator PRIVATE
        FORMULA_GLSLANG_VALIDATOR="${GLSLANG_VALIDATOR}")
endif ()
