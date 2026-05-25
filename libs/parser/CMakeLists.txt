# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
find_package(ZLIB REQUIRED)

add_library(formula-parser
    include/formula/parser/FormulaEntry.h
    FormulaEntry.cpp
    include/formula/parser/Gradient.h
    Gradient.cpp
    include/formula/parser/Lexer.h
    Lexer.cpp
    include/formula/parser/Parameter.h
    Parameter.cpp
    include/formula/parser/ParseOptions.h
    include/formula/parser/Parser.h
    Parser.cpp
    include/formula/parser/Preprocessor.h
    Preprocessor.cpp
)
target_include_directories(formula-parser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(formula-parser
    PUBLIC formula-core
    PRIVATE ZLIB::ZLIB
)
target_folder(formula-parser "Libraries")
add_library(formula::parser ALIAS formula-parser)
