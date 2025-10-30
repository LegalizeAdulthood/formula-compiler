# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
cmake_minimum_required(VERSION 3.23...3.31)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(misc_target)
include(set_cxx_standard)
include(target_folder)
include(vs_startup_project)

set_cxx_standard(17)

set(FORMULA_COMPILER_PROJECT_NAME "Formula Compiler" CACHE "STRING" "Default project name for IDEs")
project("${FORMULA_COMPILER_PROJECT_NAME}" CXX)

include(CTest)

add_subdirectory(libs)
add_subdirectory(tools)

vs_startup_project(formula-compiler)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

misc_target(FILES
    "CMakePresets.json")
misc_group_sources("CMake Scripts" FILES
    "cmake/misc_target.cmake"
    "cmake/set_cxx_standard.cmake"
    "cmake/target_folder.cmake"
    "cmake/vs_startup_project.cmake")
misc_group_sources("Editing" FILES
    ".clang-format"
    ".editorconfig")
misc_group_sources("Git" FILES
    ".gitignore"
    ".gitmodules")
misc_group_sources("GitHub" FILES
    ".github/copilot-instructions.md"
    ".github/workflows/cmake.yml"
    "LICENSE.txt"
    "ReadMe.md")
misc_group_sources("VcPkg" FILES
    "vcpkg.json")
