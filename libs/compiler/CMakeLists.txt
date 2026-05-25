# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
find_package(asmjit CONFIG REQUIRED)

add_library(formula-compiler-lib
    include/formula/compiler/Compiler.h
    Compiler.cpp
)
target_include_directories(formula-compiler-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(formula-compiler-lib
    PUBLIC formula-core formula-parser formula-semantics asmjit::asmjit
)
target_compile_options(formula-compiler-lib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_folder(formula-compiler-lib "Libraries")
add_library(formula::compiler ALIAS formula-compiler-lib)
