# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(formula-core
    include/formula/core/Complex.h
    Complex.cpp
    include/formula/core/Dialect.h
    include/formula/core/FileEntry.h
    FileEntry.cpp
    include/formula/core/functions.h
    functions.cpp
    include/formula/core/Node.h
    Node.cpp
    include/formula/core/NodeTyper.h
    include/formula/core/Section.h
    Section.cpp
    include/formula/core/SourceLocation.h
    include/formula/core/Value.h
    Value.cpp
    include/formula/core/Visitor.h
)
target_include_directories(formula-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_folder(formula-core "Libraries")
add_library(formula::core ALIAS formula-core)
