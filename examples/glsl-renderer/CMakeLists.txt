# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(glsl-renderer main.cpp)
target_link_libraries(glsl-renderer PRIVATE
    formula::parser
    formula::translator
    glad::glad
    glfw
    OpenGL::GL
)
target_folder(glsl-renderer "Examples")

add_test(NAME glsl-renderer-smoke COMMAND glsl-renderer)
set_tests_properties(glsl-renderer-smoke PROPERTIES
    SKIP_RETURN_CODE 77
)
