# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
# Script to parse id.frm and generate C++ source file with formula data

# Read the input file line by line
file(STRINGS "${INPUT_FILE}" LINES)

# Initialize output
set(FORMULA_ENTRIES "")
set(FORMULA_COUNT 0)

set(CURRENT_NAME "")
set(CURRENT_BLOCK "")
set(IN_BLOCK FALSE)
set(BRACE_COUNT 0)
set(LINE_NUMBER 0)

foreach(LINE IN LISTS LINES)
    math(EXPR LINE_NUMBER "${LINE_NUMBER} + 1")
    
    if(NOT IN_BLOCK)
        # Look for a line with a name followed by whitespace and {
        # First check if line contains an opening brace
        if(LINE MATCHES "\\{")
            # Extract everything before the {
            string(REGEX MATCH "^([^{]+)\\{" MATCH_RESULT "${LINE}")
            if(MATCH_RESULT)
                set(BEFORE_BRACE "${CMAKE_MATCH_1}")
                string(STRIP "${BEFORE_BRACE}" TRIMMED_NAME)
                
                # Check if we have a valid name (not empty, not just whitespace, not "comment")
                if(TRIMMED_NAME AND NOT TRIMMED_NAME STREQUAL "" AND NOT TRIMMED_NAME STREQUAL "comment")
                    string(STRIP "${TRIMMED_NAME}" TRIMMED_NAME)
                    
                    if(TRIMMED_NAME)
                        set(CURRENT_NAME "${TRIMMED_NAME}")
                        set(IN_BLOCK TRUE)
                        set(CURRENT_BLOCK "${LINE}\n")
                        
                        # Count braces in this line
                        string(REGEX MATCHALL "\\{" OPEN_BRACES "${LINE}")
                        string(REGEX MATCHALL "\\}" CLOSE_BRACES "${LINE}")
                        list(LENGTH OPEN_BRACES OPEN_COUNT)
                        list(LENGTH CLOSE_BRACES CLOSE_COUNT)
                        set(BRACE_COUNT ${OPEN_COUNT})
                        math(EXPR BRACE_COUNT "${BRACE_COUNT} - ${CLOSE_COUNT}")
                        
                        if(BRACE_COUNT EQUAL 0)
                            # Single line block
                            # Escape the content for C++ string literal
                            string(REPLACE "\\" "\\\\" ESCAPED_BLOCK "${CURRENT_BLOCK}")
                            string(REPLACE "\"" "\\\"" ESCAPED_BLOCK "${ESCAPED_BLOCK}")
                            string(REPLACE "\n" "\\n" ESCAPED_BLOCK "${ESCAPED_BLOCK}")
                            
                            # Add to entries
                            if(FORMULA_COUNT GREATER 0)
                                set(FORMULA_ENTRIES "${FORMULA_ENTRIES},\n")
                            endif()
                            set(FORMULA_ENTRIES "${FORMULA_ENTRIES}    {\"${CURRENT_NAME}\", \"${ESCAPED_BLOCK}\"}")
                            math(EXPR FORMULA_COUNT "${FORMULA_COUNT} + 1")
                            
                            # Reset for next block
                            set(IN_BLOCK FALSE)
                            set(CURRENT_NAME "")
                            set(CURRENT_BLOCK "")
                        endif()
                    endif()
                endif()
            endif()
        endif()
    else()
        # We're in a block, accumulate lines and count braces
        set(CURRENT_BLOCK "${CURRENT_BLOCK}${LINE}\n")
        
        # Count braces to handle nested blocks
        string(REGEX MATCHALL "\\{" OPEN_BRACES "${LINE}")
        string(REGEX MATCHALL "\\}" CLOSE_BRACES "${LINE}")
        list(LENGTH OPEN_BRACES OPEN_COUNT)
        list(LENGTH CLOSE_BRACES CLOSE_COUNT)
        math(EXPR BRACE_COUNT "${BRACE_COUNT} + ${OPEN_COUNT} - ${CLOSE_COUNT}")
        
        if(BRACE_COUNT EQUAL 0)
            # Block complete
            # Escape the content for C++ string literal
            string(REPLACE "\\" "\\\\" ESCAPED_BLOCK "${CURRENT_BLOCK}")
            string(REPLACE "\"" "\\\"" ESCAPED_BLOCK "${ESCAPED_BLOCK}")
            string(REPLACE "\n" "\\n" ESCAPED_BLOCK "${ESCAPED_BLOCK}")
            
            # Add to entries
            if(FORMULA_COUNT GREATER 0)
                set(FORMULA_ENTRIES "${FORMULA_ENTRIES},\n")
            endif()
            set(FORMULA_ENTRIES "${FORMULA_ENTRIES}    {\"${CURRENT_NAME}\", \"${ESCAPED_BLOCK}\"}")
            math(EXPR FORMULA_COUNT "${FORMULA_COUNT} + 1")
            
            # Reset for next block
            set(IN_BLOCK FALSE)
            set(CURRENT_NAME "")
            set(CURRENT_BLOCK "")
        endif()
    endif()
endforeach()

# Generate the C++ source file
set(OUTPUT_CONTENT "// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
// Auto-generated file from id.frm - DO NOT EDIT MANUALLY
#include \"formula-data.h\"

namespace formula::test
{

const FormulaEntry formulas[] =
{
${FORMULA_ENTRIES}
};

const size_t formula_count = ${FORMULA_COUNT};

} // namespace formula::test
")

# Write the output file
file(WRITE "${OUTPUT_FILE}" "${OUTPUT_CONTENT}")

message(STATUS "Generated ${OUTPUT_FILE} with ${FORMULA_COUNT} formula entries")
