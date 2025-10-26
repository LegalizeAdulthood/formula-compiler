# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
find_package(asmjit CONFIG REQUIRED)
find_package(boost_parser CONFIG REQUIRED)

add_library(formula
    ast.h ast.cpp
    include/formula/Complex.h
    include/formula/formula.h
    formula.cpp
    functions.h functions.cpp
    Compiler.h Compiler.cpp
    Interpreter.h Interpreter.cpp
    Visitor.h
)
target_compile_options(formula PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_include_directories(formula PUBLIC include)
target_link_libraries(formula PRIVATE asmjit::asmjit Boost::parser)
target_folder(formula "Libraries")
