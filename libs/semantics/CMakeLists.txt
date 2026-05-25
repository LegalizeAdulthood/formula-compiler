# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(formula-semantics
    include/formula/semantics/ReferenceCollector.h
    ReferenceCollector.cpp
    include/formula/semantics/SemanticAnalyzer.h
    SemanticAnalyzer.cpp
    include/formula/semantics/Simplifier.h
    Simplifier.cpp
)
target_include_directories(formula-semantics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(formula-semantics PUBLIC formula-core formula-parser)
target_folder(formula-semantics "Libraries")
add_library(formula::semantics ALIAS formula-semantics)
