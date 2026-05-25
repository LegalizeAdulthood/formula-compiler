# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(formula-api INTERFACE)
target_include_directories(formula-api INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(formula-api INTERFACE formula-core formula-parser)
add_library(formula::api ALIAS formula-api)

add_library(formula-facade
    include/formula/facade/Formula.h
    Formula.cpp
)
target_link_libraries(formula-facade
    PUBLIC formula-api formula-interpreter formula-compiler-lib
)
target_folder(formula-facade "Libraries")
add_library(formula::facade ALIAS formula-facade)

add_library(formula INTERFACE)
target_link_libraries(formula INTERFACE
    formula-facade
    formula-translator
)
add_library(formula::formula ALIAS formula)
