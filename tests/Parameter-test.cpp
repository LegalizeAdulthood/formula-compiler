// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/FileEntry.h>
#include <formula/Parameter.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
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

FileEntry formula_entry_with_body(std::string_view filename, std::string_view name, std::string_view body)
{
    FileEntry entry;
    entry.name = std::string{name};
    entry.body = std::string{body};
    entry.body_range.begin = SourceLocation{1, 1, std::string{filename}};
    return entry;
}

std::uint32_t crc32(std::string_view bytes)
{
    std::uint32_t crc{0xffffffffU};
    for (unsigned char byte : bytes)
    {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1U) ^ (crc & 1U ? 0xedb88320U : 0U);
        }
    }
    return crc ^ 0xffffffffU;
}

std::string encode_uf_base64(std::string_view bytes)
{
    static constexpr std::string_view alphabet{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
    std::string result;
    result.reserve(((bytes.size() + 2) / 3) * 4);
    for (std::size_t pos = 0; pos < bytes.size(); pos += 3)
    {
        const std::size_t count{std::min<std::size_t>(3, bytes.size() - pos)};
        const auto b0 = static_cast<unsigned char>(bytes[pos]);
        const auto b1 = count > 1 ? static_cast<unsigned char>(bytes[pos + 1]) : 0;
        const auto b2 = count > 2 ? static_cast<unsigned char>(bytes[pos + 2]) : 0;

        result.push_back(alphabet[b0 & 0x3fU]);
        result.push_back(alphabet[((b0 >> 6U) | ((b1 & 0x0fU) << 2U)) & 0x3fU]);
        result.push_back(count > 1 ? alphabet[((b1 >> 4U) | ((b2 & 0x03U) << 4U)) & 0x3fU] : '=');
        result.push_back(count > 2 ? alphabet[(b2 >> 2U) & 0x3fU] : '=');
    }
    return result;
}

std::string compressed_body_for_zlib_stream(std::string_view stream)
{
    const std::uint32_t crc{crc32(stream)};
    std::string payload;
    payload.push_back(static_cast<char>(crc & 0xffU));
    payload.push_back(static_cast<char>((crc >> 8U) & 0xffU));
    payload.push_back(static_cast<char>((crc >> 16U) & 0xffU));
    payload.push_back(static_cast<char>((crc >> 24U) & 0xffU));
    payload.append(stream);
    return "::" + encode_uf_base64(payload) + "\n";
}

std::string corrupt_first_payload_character(std::string compressed)
{
    for (char &ch : compressed)
    {
        if (ch != ':' && ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
        {
            ch = ch == 'A' ? 'B' : 'A';
            break;
        }
    }
    return compressed;
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
    ASSERT_EQ(1U, result.layers.size());
    EXPECT_EQ("layer", result.layers[0].layer.name);
    EXPECT_EQ("formula", result.layers[0].formula.name);
    ASSERT_EQ(3U, result.layers[0].formula.assignments.size());
    EXPECT_EQ("p_power", result.layers[0].formula.assignments[2].key);
    EXPECT_EQ("2", result.layers[0].formula.assignments[2].value);
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

TEST(TestParameterParser, compressedPayloadLineLengthIsNotSemantic)
{
    const std::string body{"fractal:\n"
                           "layer:\n"
                           "mapping:\n"
                           "formula:\n"
                           "inside:\n"
                           "outside:\n"
                           "gradient:\n"};
    const std::string compressed{compress_parameter_set(body)};
    std::string payload;
    for (char ch : compressed.substr(2))
    {
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
        {
            payload.push_back(ch);
        }
    }
    const std::string rewrapped{
        "::" + payload.substr(0, 5) + "\n  " + payload.substr(5, 7) + "\n" + payload.substr(12) + "\n"};

    EXPECT_EQ(body, decompress_parameter_set(rewrapped));
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
    ASSERT_EQ(1U, result.layers.size());
    EXPECT_EQ("layer", result.layers[0].layer.name);
    ASSERT_EQ(1U, result.layers[0].layer.assignments.size());
    EXPECT_EQ("Layer", result.layers[0].layer.assignments[0].value);
}

TEST(TestParameterParser, reportsInvalidCompressedExtendedParameterBodies)
{
    FileEntry entry{entry_with_body("::AAAA\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::INVALID_COMPRESSED_PARAMETER_SET, result.diagnostics[0].code);
}

TEST(TestParameterParser, reportsCompressedBodiesWithBadBase64)
{
    FileEntry entry{entry_with_body("::!!!!\n")};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::INVALID_COMPRESSED_PARAMETER_SET, result.diagnostics[0].code);
}

TEST(TestParameterParser, reportsCompressedBodiesWithBadCrc)
{
    const std::string body{"fractal:\n"
                           "layer:\n"
                           "mapping:\n"
                           "formula:\n"
                           "inside:\n"
                           "outside:\n"
                           "gradient:\n"};
    FileEntry entry{entry_with_body(corrupt_first_payload_character(compress_parameter_set(body)))};

    const ExtendedParameterEntry result{parse_extended_parameters(entry)};

    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_EQ(ParseErrorCode::INVALID_COMPRESSED_PARAMETER_SET, result.diagnostics[0].code);
}

TEST(TestParameterParser, reportsCompressedBodiesWithBadZlib)
{
    FileEntry entry{entry_with_body(compressed_body_for_zlib_stream("not zlib"))};

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
    ASSERT_EQ(1U, result.layers.size());
    ASSERT_EQ(4U, result.layers[0].gradient.assignments.size());
    EXPECT_EQ("index", result.layers[0].gradient.assignments[0].key);
    EXPECT_EQ("0", result.layers[0].gradient.assignments[0].value);
    EXPECT_EQ("color", result.layers[0].gradient.assignments[1].key);
    EXPECT_EQ("4278190080", result.layers[0].gradient.assignments[1].value);
    EXPECT_EQ("index", result.layers[0].gradient.assignments[2].key);
    EXPECT_EQ("1", result.layers[0].gradient.assignments[2].value);
    EXPECT_EQ("color", result.layers[0].gradient.assignments[3].key);
    EXPECT_EQ("4294967295", result.layers[0].gradient.assignments[3].value);
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
    ASSERT_EQ(1U, result.layers.size());
    ASSERT_EQ(3U, result.layers[0].layer.assignments.size());
    EXPECT_EQ("caption", result.layers[0].layer.assignments[0].key);
    EXPECT_EQ("Layer 1", result.layers[0].layer.assignments[0].value);
    EXPECT_EQ("visible", result.layers[0].layer.assignments[1].key);
    EXPECT_EQ("yes", result.layers[0].layer.assignments[1].value);
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
    EXPECT_TRUE(result.layers.empty());
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
    ASSERT_EQ(1U, result.layers.size());
    ASSERT_EQ(2U, result.layers[0].transforms.size());
    EXPECT_EQ("transform", result.layers[0].transforms[0].name);
    EXPECT_EQ("transform", result.layers[0].transforms[1].name);
    ASSERT_TRUE(result.layers[0].opacity.has_value());
    EXPECT_EQ("opacity", result.layers[0].opacity->name);
}

TEST(TestParameterParser, collectsParameterReferencesWithAssociatedParameters)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "transform:\n"
                                    "filename=\"one.uxf\" entry=\"Transform One\" p_amount=0.25\n"
                                    "transform:\n"
                                    "filename=\"two.uxf\" entry=\"Transform Two\" f_fn1=sin\n"
                                    "formula:\n"
                                    "filename=\"mmf.ufm\" entry=\"Mandelbrot\" p_power=2\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\" p_bailout=yes\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\" f_fn1=ident\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const std::vector<ParameterReference> references{collect_parameter_references(parameters)};

    ASSERT_EQ(5U, references.size());
    EXPECT_EQ(ParameterReferenceKind::FRACTAL_FORMULA, references[0].site.kind);
    EXPECT_EQ(0U, references[0].site.layer_index);
    EXPECT_EQ("mmf.ufm", references[0].filename);
    EXPECT_EQ("Mandelbrot", references[0].entry);
    ASSERT_EQ(1U, references[0].parameters.size());
    EXPECT_EQ("p_power", references[0].parameters[0].key);
    EXPECT_EQ("2", references[0].parameters[0].value);

    EXPECT_EQ(ParameterReferenceKind::INSIDE_COLORING, references[1].site.kind);
    EXPECT_EQ("dmj.ucl", references[1].filename);
    EXPECT_EQ("Smooth", references[1].entry);
    ASSERT_EQ(1U, references[1].parameters.size());
    EXPECT_EQ("p_bailout", references[1].parameters[0].key);

    EXPECT_EQ(ParameterReferenceKind::OUTSIDE_COLORING, references[2].site.kind);
    EXPECT_EQ("dmj.ucl", references[2].filename);
    EXPECT_EQ("Escape", references[2].entry);
    ASSERT_EQ(1U, references[2].parameters.size());
    EXPECT_EQ("f_fn1", references[2].parameters[0].key);

    EXPECT_EQ(ParameterReferenceKind::TRANSFORM, references[3].site.kind);
    EXPECT_EQ(0U, references[3].site.transform_index);
    EXPECT_EQ("one.uxf", references[3].filename);
    EXPECT_EQ("Transform One", references[3].entry);
    ASSERT_EQ(1U, references[3].parameters.size());
    EXPECT_EQ("p_amount", references[3].parameters[0].key);

    EXPECT_EQ(ParameterReferenceKind::TRANSFORM, references[4].site.kind);
    EXPECT_EQ(1U, references[4].site.transform_index);
    EXPECT_EQ("two.uxf", references[4].filename);
    EXPECT_EQ("Transform Two", references[4].entry);
}

TEST(TestParameterParser, collectsReferencesForEachLayer)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"one.ufm\" entry=\"A\"\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"two.ufm\" entry=\"B\"\n"
                                    "inside:\n"
                                    "outside:\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const std::vector<ParameterReference> references{collect_parameter_references(parameters)};

    ASSERT_EQ(6U, references.size());
    EXPECT_EQ(0U, references[0].site.layer_index);
    EXPECT_EQ("one.ufm", references[0].filename);
    EXPECT_EQ(0U, references[1].site.layer_index);
    EXPECT_EQ(0U, references[2].site.layer_index);
    EXPECT_EQ(1U, references[3].site.layer_index);
    EXPECT_EQ("two.ufm", references[3].filename);
    EXPECT_EQ(1U, references[4].site.layer_index);
    EXPECT_EQ(1U, references[5].site.layer_index);
}

TEST(TestParameterParser, resolvesParameterReferences)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "transform:\n"
                                    "filename=\"wobble.uxf\" entry=\"Wobble\"\n"
                                    "formula:\n"
                                    "filename=\"mmf.ufm\" entry=\"Mandelbrot\"\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\"\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\"\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const ParameterReferenceSet result{resolve_parameter_references(parameters,
        [](std::string_view filename, std::string_view entry_name) -> std::optional<FileEntry>
        {
            if (filename == "mmf.ufm" && entry_name == "Mandelbrot")
            {
                return formula_entry_with_body(filename, entry_name, "z=0:z=z+1,|z|<4");
            }
            if (filename == "dmj.ucl" && (entry_name == "Smooth" || entry_name == "Escape"))
            {
                return formula_entry_with_body(filename, entry_name, "0");
            }
            if (filename == "wobble.uxf" && entry_name == "Wobble")
            {
                return formula_entry_with_body(filename, entry_name, "pixel");
            }
            return std::nullopt;
        })};

    ASSERT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(4U, result.references.size());
    ASSERT_EQ(4U, result.resolved.size());
    EXPECT_TRUE(result.resolved[0].ast->iterate);
    EXPECT_TRUE(result.resolved[1].ast->final);
    EXPECT_TRUE(result.resolved[2].ast->final);
    EXPECT_TRUE(result.resolved[3].ast->transform);
    EXPECT_EQ("wobble.uxf", result.resolved[3].reference.filename);
}

TEST(TestParameterParser, resolveParameterReferencesReportsMissingSelectorsAndUnresolvedEntries)
{
    ExtendedParameterEntry parameters;
    ParameterLayer layer;
    layer.formula.assignments.push_back(Parameter{"entry", "Mandelbrot"});
    layer.inside.assignments.push_back(Parameter{"filename", "dmj.ucl"});
    layer.outside.assignments.push_back(Parameter{"filename", "missing.ucl"});
    layer.outside.assignments.push_back(Parameter{"entry", "Escape"});
    parameters.layers.push_back(std::move(layer));

    const ParameterReferenceSet result{resolve_parameter_references(
        parameters, [](std::string_view, std::string_view) -> std::optional<FileEntry> { return std::nullopt; })};

    ASSERT_EQ(3U, result.references.size());
    ASSERT_EQ(3U, result.diagnostics.size());
    EXPECT_EQ(ParameterReferenceErrorCode::MISSING_FILENAME, result.diagnostics[0].code);
    EXPECT_EQ(ParameterReferenceErrorCode::MISSING_ENTRY, result.diagnostics[1].code);
    EXPECT_EQ(ParameterReferenceErrorCode::UNRESOLVED_ENTRY, result.diagnostics[2].code);
    EXPECT_TRUE(result.resolved.empty());
}

TEST(TestParameterParser, resolveParameterReferencesReportsFormulaParseErrors)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"bad.ufm\" entry=\"Broken\"\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\"\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\"\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const ParameterReferenceSet result{resolve_parameter_references(parameters,
        [](std::string_view filename, std::string_view entry_name) -> std::optional<FileEntry>
        {
            if (filename == "bad.ufm" && entry_name == "Broken")
            {
                return formula_entry_with_body(filename, entry_name,
                    "default:\n"
                    "title=\"Broken\"\n"
                    "final:\n"
                    "1\n");
            }
            return formula_entry_with_body(filename, entry_name, "0");
        })};

    ASSERT_EQ(1U, result.diagnostics.size());
    EXPECT_EQ(ParameterReferenceErrorCode::PARSE_ERROR, result.diagnostics[0].code);
    EXPECT_EQ("bad.ufm", result.diagnostics[0].location.filename);
    EXPECT_EQ(2U, result.resolved.size());
}

TEST(TestParameterParser, resolveParameterReferencesValidatesSavedParameters)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"typed.ufm\" entry=\"Typed\" p_power=3.5 f_fn1=cos\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\"\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\"\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const ParameterReferenceSet result{resolve_parameter_references(parameters,
        [](std::string_view filename, std::string_view entry_name) -> std::optional<FileEntry>
        {
            if (filename == "typed.ufm" && entry_name == "Typed")
            {
                return formula_entry_with_body(filename, entry_name,
                    "default:\n"
                    "float param power\n"
                    "default=2.0\n"
                    "endparam\n"
                    "func fn1\n"
                    "default=cos()\n"
                    "endfunc\n");
            }
            return formula_entry_with_body(filename, entry_name, "0");
        })};

    EXPECT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(3U, result.resolved.size());
}

TEST(TestParameterParser, resolveParameterReferencesReportsParameterSemanticErrors)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"typed.ufm\" entry=\"Typed\" p_enabled=maybe p_extra=1 f_bad=sin\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\"\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\"\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const ParameterReferenceSet result{resolve_parameter_references(parameters,
        [](std::string_view filename, std::string_view entry_name) -> std::optional<FileEntry>
        {
            if (filename == "typed.ufm" && entry_name == "Typed")
            {
                return formula_entry_with_body(filename, entry_name,
                    "default:\n"
                    "float param power\n"
                    "endparam\n"
                    "bool param enabled\n"
                    "default=true\n"
                    "endparam\n"
                    "func fn1\n"
                    "default=cos()\n"
                    "endfunc\n");
            }
            return formula_entry_with_body(filename, entry_name, "0");
        })};

    ASSERT_EQ(4U, result.diagnostics.size());
    EXPECT_EQ(ParameterReferenceErrorCode::TYPE_MISMATCH, result.diagnostics[0].code);
    EXPECT_EQ("p_enabled", result.diagnostics[0].detail);
    EXPECT_EQ(ParameterReferenceErrorCode::UNKNOWN_PARAMETER, result.diagnostics[1].code);
    EXPECT_EQ("p_extra", result.diagnostics[1].detail);
    EXPECT_EQ(ParameterReferenceErrorCode::UNKNOWN_PARAMETER, result.diagnostics[2].code);
    EXPECT_EQ("f_bad", result.diagnostics[2].detail);
    EXPECT_EQ(ParameterReferenceErrorCode::MISSING_REQUIRED_PARAMETER, result.diagnostics[3].code);
    EXPECT_EQ("p_power", result.diagnostics[3].detail);
}

TEST(TestParameterParser, resolveParameterReferencesUsesParameterForwards)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"typed.ufm\" entry=\"Typed\" p_oldpower=3.5\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\"\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\"\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const ParameterReferenceSet result{resolve_parameter_references(parameters,
        [](std::string_view filename, std::string_view entry_name) -> std::optional<FileEntry>
        {
            if (filename == "typed.ufm" && entry_name == "Typed")
            {
                return formula_entry_with_body(filename, entry_name,
                    "default:\n"
                    "param oldpower = power\n"
                    "float param power\n"
                    "endparam\n");
            }
            return formula_entry_with_body(filename, entry_name, "0");
        })};

    EXPECT_TRUE(result.diagnostics.empty());
    ASSERT_EQ(3U, result.resolved.size());
}

TEST(TestParameterParser, resolveParameterReferencesReportsInvalidParameterForwards)
{
    FileEntry entry{entry_with_body("fractal:\n"
                                    "layer:\n"
                                    "mapping:\n"
                                    "formula:\n"
                                    "filename=\"typed.ufm\" entry=\"Typed\" p_oldpower=3.5 p_unknown=1\n"
                                    "inside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Smooth\"\n"
                                    "outside:\n"
                                    "filename=\"dmj.ucl\" entry=\"Escape\"\n"
                                    "gradient:\n")};
    const ExtendedParameterEntry parameters{parse_extended_parameters(entry)};
    ASSERT_TRUE(parameters.diagnostics.empty());

    const ParameterReferenceSet result{resolve_parameter_references(parameters,
        [](std::string_view filename, std::string_view entry_name) -> std::optional<FileEntry>
        {
            if (filename == "typed.ufm" && entry_name == "Typed")
            {
                return formula_entry_with_body(filename, entry_name,
                    "default:\n"
                    "param oldpower = missing\n"
                    "float param power\n"
                    "default=2.0\n"
                    "endparam\n");
            }
            return formula_entry_with_body(filename, entry_name, "0");
        })};

    ASSERT_EQ(2U, result.diagnostics.size());
    EXPECT_EQ(ParameterReferenceErrorCode::INVALID_PARAMETER_FORWARD, result.diagnostics[0].code);
    EXPECT_EQ("p_oldpower", result.diagnostics[0].detail);
    EXPECT_EQ(ParameterReferenceErrorCode::UNKNOWN_PARAMETER, result.diagnostics[1].code);
    EXPECT_EQ("p_unknown", result.diagnostics[1].detail);
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
    ASSERT_EQ(1U, result.layers.size());
    ASSERT_TRUE(result.layers[0].opacity.has_value());
    EXPECT_EQ("alpha", result.layers[0].opacity->name);
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

TEST(TestParameterParser, extendedParametersDropUnexpectedSections)
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
    ASSERT_EQ(1U, result.layers.size());
    EXPECT_EQ("gradient", result.layers[0].gradient.name);
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
