# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(formula-translator
    include/formula/translator/GLSLEmitter.h
    GLSLEmitter.cpp
)
target_include_directories(formula-translator PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(formula-translator PUBLIC formula-core formula-parser formula-semantics)
target_folder(formula-translator "Libraries")
add_library(formula::translator ALIAS formula-translator)
