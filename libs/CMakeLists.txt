# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025-2026 Richard Thomson
#
find_package(asmjit CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(formula
    include/formula/Compiler.h Compiler.cpp
    include/formula/Complex.h Complex.cpp
    include/formula/FileEntry.h FileEntry.cpp
    include/formula/Formula.h Formula.cpp
    include/formula/FormulaEntry.h FormulaEntry.cpp
    functions.h functions.cpp
    include/formula/GLSLEmitter.h GLSLEmitter.cpp
    include/formula/Gradient.h Gradient.cpp
    include/formula/Interpreter.h Interpreter.cpp
    include/formula/Lexer.h Lexer.cpp
    include/formula/Node.h Node.cpp
    include/formula/NodeTyper.h
    include/formula/Parameter.h Parameter.cpp
    include/formula/ParseOptions.h
    include/formula/Parser.h Parser.cpp
    include/formula/Preprocessor.h Preprocessor.cpp
    include/formula/ReferenceCollector.h ReferenceCollector.cpp
    include/formula/SemanticAnalyzer.h SemanticAnalyzer.cpp
    include/formula/Simplifier.h Simplifier.cpp
    include/formula/SourceLocation.h
    include/formula/Visitor.h
)
target_compile_options(formula PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_include_directories(formula PUBLIC include)
target_link_libraries(formula PRIVATE asmjit::asmjit ZLIB::ZLIB)
target_folder(formula "Libraries")
