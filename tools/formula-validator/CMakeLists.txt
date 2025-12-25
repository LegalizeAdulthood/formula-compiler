# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
add_executable(formula-validator main.cpp)
target_link_libraries(formula-validator PUBLIC formula)
target_folder(formula-validator "Tools")
