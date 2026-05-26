# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_executable(generate-formula-data generate-formula-data.cpp)
target_link_libraries(generate-formula-data PUBLIC formula)
target_folder(generate-formula-data "Tests/Tools")

set(FORMULA_DATA_H "${CMAKE_CURRENT_BINARY_DIR}/formula-data.h")
set(FORMULA_DATA_CPP "${CMAKE_CURRENT_BINARY_DIR}/formula-data.cpp")

add_custom_command(
    OUTPUT "${FORMULA_DATA_H}" "${FORMULA_DATA_CPP}"
    COMMAND generate-formula-data
        "${ID_FRM_FILE}"
        "${FORMULA_DATA_H}"
        "${FORMULA_DATA_CPP}"
    DEPENDS ${ID_FRM_FILE} generate-formula-data
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating formula-data.h and formula-data.cpp from id.frm"
    VERBATIM
)

add_library(test-formula-facade OBJECT
    formula-data-test.cpp
    parse-id-formulas-test.cpp
    "${FORMULA_DATA_H}"
    "${FORMULA_DATA_CPP}"
)
target_include_directories(test-formula-facade PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(test-formula-facade PUBLIC formula::facade)
configure_formula_test_library(test-formula-facade)
