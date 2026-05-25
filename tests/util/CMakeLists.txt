# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025-2026 Richard Thomson
#
add_library(test-formula-util OBJECT
    include/formula/test/ExpressionParam.h
    ExpressionParam.cpp
    include/formula/test/function-call.h
    function-call.cpp
    include/formula/test/node-builders.h
    include/formula/test/NodeFormatter.h
    NodeFormatter.cpp
    include/formula/test/trim_ws.h
    trim_ws.cpp
)
target_include_directories(test-formula-util PUBLIC include)
target_link_libraries(test-formula-util PUBLIC formula::facade)
target_folder(test-formula-util "Tests/Libraries")
