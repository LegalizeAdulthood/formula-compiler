# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
find_package(asmjit CONFIG REQUIRED)

add_library(formula
    include/formula/Compiler.h Compiler.cpp
    include/formula/Complex.h Complex.cpp
    include/formula/Formula.h Formula.cpp
    functions.h functions.cpp
    include/formula/Interpreter.h Interpreter.cpp
    include/formula/Lexer.h Lexer.cpp
    include/formula/Node.h Node.cpp
    include/formula/NodeTyper.h
    Parser.h Parser.cpp
    include/formula/Simplifier.h Simplifier.cpp
    include/formula/Visitor.h
)
target_compile_options(formula PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_include_directories(formula PUBLIC include)
target_link_libraries(formula PRIVATE asmjit::asmjit)
target_folder(formula "Libraries")
