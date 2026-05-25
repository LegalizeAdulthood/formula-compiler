# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(formula-interpreter
    include/formula/interpreter/ExtendedInterpreter.h
    ExtendedInterpreter.cpp
    include/formula/interpreter/ExtendedRuntime.h
    ExtendedRuntime.cpp
    include/formula/interpreter/Interpreter.h
    Interpreter.cpp
)
target_include_directories(formula-interpreter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(formula-interpreter
    PUBLIC formula-api formula-core formula-parser formula-semantics
)
target_folder(formula-interpreter "Libraries")
add_library(formula::interpreter ALIAS formula-interpreter)
