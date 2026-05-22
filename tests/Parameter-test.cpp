// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/FileEntry.h>
#include <formula/Parameter.h>

#include <gtest/gtest.h>

#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace formula::parameter;
using namespace testing;

namespace formula::parameter
{

static std::ostream &operator<<(std::ostream &os, ParseErrorCode code)
{
    return os << to_string(code);
}

} // namespace formula::parameter

namespace formula::test
{

namespace
{

FileEntry entry_with_body(std::string_view body)
{
    FileEntry entry;
    entry.name = "Entry";
    entry.body = std::string{body};
    entry.body_range.begin = SourceLocation{1, 1, "example.upr"};
    return entry;
}

} // namespace

TEST(TestParameterParser, clientCanUseSharedFileScanner)
{
    std::istringstream input{"One { fractal:\n"
                             "title=\"One\" }Next Entry { layer:\n"
                             "caption=\"Layer\" }"};

    const std::vector<FileEntry> entries{load_file_entries(input)};

    ASSERT_EQ(2U, entries.size());
    EXPECT_EQ("One", entries[0].name);
    EXPECT_EQ("Next Entry", entries[1].name);
}

TEST(TestParameterParser, parsesBasicNameValuePairsFromFileEntry)
{
    FileEntry entry{entry_with_body("title=\"Name\" magn=1.5 center=-0.5/0.25")};

    const BasicParameterEntry result{parse_basic_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(3U, result.assignments.size());
    EXPECT_EQ("title", result.assignments[0].key);
    EXPECT_EQ("Name", result.assignments[0].value);
    EXPECT_EQ("magn", result.assignments[1].key);
    EXPECT_EQ("1.5", result.assignments[1].value);
    EXPECT_EQ("center", result.assignments[2].key);
    EXPECT_EQ("-0.5/0.25", result.assignments[2].value);
}

TEST(TestParameterParser, parsesExtendedSectionsFromFileEntry)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Name\"\n"
                                    "layer:\n"
                                    "caption=\"Layer 1\"\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"mmf.ufm\" entry=\"Mandelbrot\" p_power=2\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    EXPECT_EQ("fractal", result.fractal.name);
    ASSERT_EQ(1U, result.fractal.assignments.size());
    EXPECT_EQ("title", result.fractal.assignments[0].key);
    EXPECT_EQ("Name", result.fractal.assignments[0].value);
    ASSERT_EQ(6U, result.layers.size());
    EXPECT_EQ("layer", result.layers[0].name);
    EXPECT_EQ("formula", result.layers[2].name);
    ASSERT_EQ(3U, result.layers[2].assignments.size());
    EXPECT_EQ("p_power", result.layers[2].assignments[2].key);
    EXPECT_EQ("2", result.layers[2].assignments[2].value);
}

TEST(TestParameterParser, compressesAndDecompressesParameterSetBodies)
{
    const std::string body{
        "fractal:\r\n"
        "title=\"Compressed\" magn=2\r\n"
        "layer:\r\n"
        "caption=\"Layer\"\r\n"
        "comments=\"This body is long enough to wrap the compressed payload over multiple lines.\"\r\n"};

    const std::string compressed{compress_parameter_set(body)};
    const std::string decompressed{decompress_parameter_set(compressed)};

    EXPECT_EQ(0U, compressed.rfind("::", 0));
    EXPECT_NE(std::string::npos, compressed.find("\n  "));
    EXPECT_EQ(body, decompressed);
}

TEST(TestParameterParser, parsesCompressedExtendedParameterBodies)
{
    const std::string body{"fractal:\n"
                           "title=\"Compressed\" magn=2\n"
                           "layer:\n"
                           "caption=\"Layer\"\n"
                           "mapping:\n"
                           "formula:\n"
                           "inside:\n"
                           "outside:\n"
                           "gradient:\n"};
    FileEntry entry{entry_with_body(compress_parameter_set(body))};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    EXPECT_EQ("fractal", result.fractal.name);
    ASSERT_EQ(2U, result.fractal.assignments.size());
    EXPECT_EQ("title", result.fractal.assignments[0].key);
    EXPECT_EQ("Compressed", result.fractal.assignments[0].value);
    EXPECT_EQ("magn", result.fractal.assignments[1].key);
    EXPECT_EQ("2", result.fractal.assignments[1].value);
    ASSERT_EQ(6U, result.layers.size());
    EXPECT_EQ("layer", result.layers[0].name);
    ASSERT_EQ(1U, result.layers[0].assignments.size());
    EXPECT_EQ("Layer", result.layers[0].assignments[0].value);
}

TEST(TestParameterParser, reportsInvalidCompressedExtendedParameterBodies)
{
    FileEntry entry{entry_with_body("::AAAA\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::INVALID_COMPRESSED_PARAMETER_SET, result.diagnostics[0].code);
}

TEST(TestParameterParser, preservesRepeatedAssignmentsInOrder)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Name\"\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n"
                                    "index=0 color=4278190080\n"
                                    "index=1 color=4294967295\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(6U, result.layers.size());
    ASSERT_EQ(4U, result.layers[5].assignments.size());
    EXPECT_EQ("index", result.layers[5].assignments[0].key);
    EXPECT_EQ("0", result.layers[5].assignments[0].value);
    EXPECT_EQ("color", result.layers[5].assignments[1].key);
    EXPECT_EQ("4278190080", result.layers[5].assignments[1].value);
    EXPECT_EQ("index", result.layers[5].assignments[2].key);
    EXPECT_EQ("1", result.layers[5].assignments[2].value);
    EXPECT_EQ("color", result.layers[5].assignments[3].key);
    EXPECT_EQ("4294967295", result.layers[5].assignments[3].value);
}

TEST(TestParameterParser, stripsCommentsOutsideQuotedStrings)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Name;date\" ; comment\n"
                                    "magn=1.5 ; tail\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(2U, result.fractal.assignments.size());
    EXPECT_EQ("Name;date", result.fractal.assignments[0].value);
    EXPECT_EQ("1.5", result.fractal.assignments[1].value);
}

TEST(TestParameterParser, joinsLineContinuationsBeforeParsing)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Alpha \\\r\n"
                                    "  Beta\"\n"
                                    "center=-1.234\\\n"
                                    "  567/0\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(2U, result.fractal.assignments.size());
    EXPECT_EQ("Alpha Beta", result.fractal.assignments[0].value);
    EXPECT_EQ("-1.234567/0", result.fractal.assignments[1].value);
}

TEST(TestParameterParser, splitsAssignmentsOnWhitespaceOutsideQuotedStrings)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Name\"\n"
                                    "layer:\n"
                                    "caption=\"Layer 1\" visible=yes alpha=no\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(6U, result.layers.size());
    ASSERT_EQ(3U, result.layers[0].assignments.size());
    EXPECT_EQ("caption", result.layers[0].assignments[0].key);
    EXPECT_EQ("Layer 1", result.layers[0].assignments[0].value);
    EXPECT_EQ("visible", result.layers[0].assignments[1].key);
    EXPECT_EQ("yes", result.layers[0].assignments[1].value);
}

TEST(TestParameterParser, extendedParametersRequireSectionBeforeAssignments)
{
    FileEntry entry{entry_with_body("title=\"Name\"\n"
                                    "fractal:\n"
                                    "magn=1.5\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_SECTION_LABEL, result.diagnostics[0].code);
    ASSERT_EQ(1U, result.fractal.assignments.size());
    EXPECT_EQ("magn", result.fractal.assignments[0].key);
}

TEST(TestParameterParser, extendedParametersRequireFractalFirst)
{
    FileEntry entry{entry_with_body("layer:\n"
                                    "caption=\"Layer\"\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_FRACTAL_SECTION, result.diagnostics[0].code);
    EXPECT_TRUE(result.fractal.name.empty());
    ASSERT_EQ(1U, result.layers.size());
    EXPECT_EQ("layer", result.layers[0].name);
}

TEST(TestParameterParser, extendedParametersRequireLayerAfterFractal)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Name\"\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_LAYER_SECTION, result.diagnostics[0].code);
    EXPECT_EQ("fractal", result.fractal.name);
    EXPECT_TRUE(result.layers.empty());
}

TEST(TestParameterParser, extendedParametersRequireCompleteLayerSequence)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_PARAMETER_SECTION, result.diagnostics[0].code);
}

TEST(TestParameterParser, extendedParametersAllowTransformsAndOpacity)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "transform:\n"
                                    "filename=\"one.uxf\" entry=\"A\"\n"
                                    "transform:\n"
                                    "filename=\"two.uxf\" entry=\"B\"\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n"
                                    "opacity:\n"
                                    "index=0 opacity=255\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(9U, result.layers.size());
    EXPECT_EQ("transform", result.layers[2].name);
    EXPECT_EQ("transform", result.layers[3].name);
    EXPECT_EQ("opacity", result.layers[8].name);
}

TEST(TestParameterParser, extendedParametersAllowAlphaInsteadOfOpacity)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n"
                                    "alpha:\n"
                                    "index=0 alpha=255\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(7U, result.layers.size());
    EXPECT_EQ("alpha", result.layers[6].name);
}

TEST(TestParameterParser, extendedParametersRejectSectionsAfterOpacity)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n"
                                    "opacity:\n"
                                    "alpha:\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_LAYER_SECTION, result.diagnostics[0].code);
}

TEST(TestParameterParser, extendedParametersPreserveUnexpectedSections)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n"
                                    "notes:\n"
                                    "text=\"x\"\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::UNEXPECTED_PARAMETER_SECTION, result.diagnostics[0].code);
    ASSERT_EQ(7U, result.layers.size());
    EXPECT_EQ("notes", result.layers[6].name);
    ASSERT_EQ(1U, result.layers[6].assignments.size());
}

TEST(TestParameterParser, basicParametersRejectSectionLabels)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"Name\"\n")};

    const BasicParameterEntry result{parse_basic_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_ASSIGNMENT, result.diagnostics[0].code);
    ASSERT_EQ(1U, result.assignments.size());
    EXPECT_EQ("title", result.assignments[0].key);
}

TEST(TestParameterParser, reportsUnterminatedQuotedString)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "title=\"no close\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::UNTERMINATED_QUOTED_STRING, result.diagnostics[0].code);
}

TEST(TestParameterParser, entrySourceLocationSeedsDiagnostics)
{
    std::istringstream input{"One {\n"
                             "fractal:\n"
                             "  title\n"
                             "}"};

    std::vector<FileEntry> entries{load_file_entries(input, "example.upr")};
    ASSERT_EQ(1U, entries.size());

    const ExtendedParameterEntry result{parse_extended_parameters(entries[0])};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::EXPECTED_ASSIGNMENT, result.diagnostics[0].code);
    EXPECT_EQ("example.upr", result.diagnostics[0].location.filename);
    EXPECT_EQ(3U, result.diagnostics[0].location.line);
    EXPECT_EQ(3U, result.diagnostics[0].location.column);
}

} // namespace formula::test
