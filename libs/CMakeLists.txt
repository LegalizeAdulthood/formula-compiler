# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
find_package(asmjit CONFIG REQUIRED)
find_package(boost_parser CONFIG REQUIRED)

add_library(formula
    include/formula/Compiler.h Compiler.cpp
    include/formula/Complex.h
    include/formula/Formula.h Formula.cpp
    functions.h functions.cpp
    include/formula/Interpreter.h Interpreter.cpp
    include/formula/Node.h Node.cpp
    include/formula/Simplifier.h Simplifier.cpp
    include/formula/Visitor.h
)
target_compile_options(formula PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_include_directories(formula PUBLIC include)
target_link_libraries(formula PRIVATE asmjit::asmjit Boost::parser)
target_folder(formula "Libraries")
