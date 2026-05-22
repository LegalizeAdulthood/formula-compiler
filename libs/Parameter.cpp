// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Parameter.h>

#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace formula::parameter
{

#define PARSE_ERROR_CASE(name_) \
    case ParseErrorCode::name_: \
        return #name_

std::string to_string(ParseErrorCode code)
{
    switch (code)
    {
        PARSE_ERROR_CASE(NONE);
        PARSE_ERROR_CASE(EXPECTED_SECTION_LABEL);
        PARSE_ERROR_CASE(EXPECTED_ASSIGNMENT);
        PARSE_ERROR_CASE(EXPECTED_VALUE);
        PARSE_ERROR_CASE(UNTERMINATED_QUOTED_STRING);
        PARSE_ERROR_CASE(INVALID_COMPRESSED_PARAMETER_SET);
        PARSE_ERROR_CASE(EXPECTED_FRACTAL_SECTION);
        PARSE_ERROR_CASE(EXPECTED_LAYER_SECTION);
        PARSE_ERROR_CASE(EXPECTED_PARAMETER_SECTION);
        PARSE_ERROR_CASE(UNEXPECTED_PARAMETER_SECTION);
    }
    return "ParseErrorCode(" + std::to_string(static_cast<int>(code)) + ")";
}

namespace
{

struct ProcessedLine
{
    std::string text;
    std::size_t line{};
};

struct CommentStrippedLine
{
    std::string text;
    bool in_quote{};
};

struct ParsedValue
{
    std::string text;
    std::size_t end{};
};

enum class LayerParseState
{
    EXPECT_LAYER,
    EXPECT_MAPPING,
    EXPECT_FORMULA,
    EXPECT_INSIDE,
    EXPECT_OUTSIDE,
    EXPECT_GRADIENT,
    EXPECT_OPACITY_OR_LAYER,
    EXPECT_LAYER_AFTER_OPACITY,
};

bool is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

std::size_t skip_space(std::string_view text, std::size_t pos)
{
    while (pos < text.length() && is_space(text[pos]))
    {
        ++pos;
    }
    return pos;
}

std::string_view trim(std::string_view text)
{
    while (!text.empty() && is_space(text.front()))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_space(text.back()))
    {
        text.remove_suffix(1);
    }
    return text;
}

bool ends_with_continuation(std::string_view text)
{
    text = trim(text);
    return !text.empty() && text.back() == '\\';
}

bool starts_with(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

CommentStrippedLine strip_comment(std::string_view line, bool in_quote)
{
    std::string result;
    bool escaped{};
    for (char ch : line)
    {
        if (escaped)
        {
            result.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\' && in_quote)
        {
            result.push_back(ch);
            escaped = true;
            continue;
        }
        if (ch == '"')
        {
            in_quote = !in_quote;
            result.push_back(ch);
            continue;
        }
        if (ch == ';' && !in_quote)
        {
            break;
        }
        result.push_back(ch);
    }
    return {std::move(result), in_quote};
}

std::vector<std::string> split_physical_lines(std::string_view input);

std::string encode_uf_base64(std::string_view bytes)
{
    // UF parameter compression uses the normal base64 alphabet, but
    // little-endian bit packing inside each 4-character group.
    static constexpr std::string_view alphabet{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

    std::string result;
    result.reserve(((bytes.size() + 2) / 3) * 4);
    for (std::size_t pos = 0; pos < bytes.size(); pos += 3)
    {
        const std::size_t count{std::min<std::size_t>(3, bytes.size() - pos)};
        const auto b0 = static_cast<unsigned char>(bytes[pos]);
        const auto b1 = count > 1 ? static_cast<unsigned char>(bytes[pos + 1]) : 0;
        const auto b2 = count > 2 ? static_cast<unsigned char>(bytes[pos + 2]) : 0;

        const unsigned q0{b0 & 0x3fU};
        const unsigned q1{((b0 >> 6U) | ((b1 & 0x0fU) << 2U)) & 0x3fU};
        const unsigned q2{((b1 >> 4U) | ((b2 & 0x03U) << 4U)) & 0x3fU};
        const unsigned q3{(b2 >> 2U) & 0x3fU};

        result.push_back(alphabet[q0]);
        result.push_back(alphabet[q1]);
        result.push_back(count > 1 ? alphabet[q2] : '=');
        result.push_back(count > 2 ? alphabet[q3] : '=');
    }
    return result;
}

int decode_uf_base64_char(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z')
    {
        return ch - 'a' + 26;
    }
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0' + 52;
    }
    if (ch == '+')
    {
        return 62;
    }
    if (ch == '/')
    {
        return 63;
    }
    return -1;
}

std::string decode_uf_base64(std::string_view text)
{
    // This intentionally does not use a standard base64 decoder; UF
    // reverses the bit packing order within each encoded group.
    if (text.size() % 4 != 0)
    {
        throw std::runtime_error{"invalid compressed parameter set"};
    }

    std::string result;
    result.reserve((text.size() / 4) * 3);
    for (std::size_t pos = 0; pos < text.size(); pos += 4)
    {
        const bool pad2{text[pos + 2] == '='};
        const bool pad3{text[pos + 3] == '='};
        if (text[pos] == '=' || text[pos + 1] == '=' || (pad2 && !pad3))
        {
            throw std::runtime_error{"invalid compressed parameter set"};
        }

        const int q0{decode_uf_base64_char(text[pos])};
        const int q1{decode_uf_base64_char(text[pos + 1])};
        const int q2{pad2 ? 0 : decode_uf_base64_char(text[pos + 2])};
        const int q3{pad3 ? 0 : decode_uf_base64_char(text[pos + 3])};
        if (q0 < 0 || q1 < 0 || q2 < 0 || q3 < 0)
        {
            throw std::runtime_error{"invalid compressed parameter set"};
        }

        result.push_back(static_cast<char>(q0 | ((q1 & 0x03) << 6)));
        if (!pad2)
        {
            result.push_back(static_cast<char>(((q1 & 0x3c) >> 2) | ((q2 & 0x0f) << 4)));
        }
        if (!pad3)
        {
            result.push_back(static_cast<char>(((q2 & 0x30) >> 4) | (q3 << 2)));
        }
    }
    return result;
}

std::string wrap_payload(std::string_view text)
{
    std::string result{"::"};
    for (std::size_t pos = 0; pos < text.size(); pos += 76)
    {
        if (pos != 0)
        {
            result.push_back('\n');
            result.append("  ");
        }
        result.append(text.substr(pos, std::min<std::size_t>(76, text.size() - pos)));
    }
    result.push_back('\n');
    return result;
}

std::string compressed_payload(std::string_view body)
{
    std::string payload;
    const std::vector<std::string> lines{split_physical_lines(body)};
    bool found_marker{};
    for (std::string_view line : lines)
    {
        const CommentStrippedLine stripped{strip_comment(line, false)};
        std::string_view text{trim(stripped.text)};
        if (!found_marker)
        {
            if (text.empty())
            {
                continue;
            }
            if (!starts_with(text, "::"))
            {
                throw std::runtime_error{"invalid compressed parameter set"};
            }
            text.remove_prefix(2);
            found_marker = true;
        }
        if (text.empty())
        {
            continue;
        }
        if (text.size() % 4 != 0)
        {
            throw std::runtime_error{"invalid compressed parameter set"};
        }
        payload.append(text);
    }
    if (!found_marker || payload.empty())
    {
        throw std::runtime_error{"invalid compressed parameter set"};
    }
    return payload;
}

bool is_compressed_parameter_set(std::string_view body)
{
    const std::vector<std::string> lines{split_physical_lines(body)};
    for (std::string_view line : lines)
    {
        const CommentStrippedLine stripped{strip_comment(line, false)};
        const std::string_view text{trim(stripped.text)};
        if (text.empty())
        {
            continue;
        }
        return starts_with(text, "::");
    }
    return false;
}

std::string zlib_deflate(std::string_view body)
{
    uLongf output_size{compressBound(static_cast<uLong>(body.size()))};
    std::string output(output_size, '\0');
    const int status{compress2(reinterpret_cast<Bytef *>(output.data()), &output_size,
        reinterpret_cast<const Bytef *>(body.data()), static_cast<uLong>(body.size()), Z_BEST_COMPRESSION)};
    if (status != Z_OK)
    {
        throw std::runtime_error{"parameter compression failed"};
    }
    output.resize(output_size);
    return output;
}

std::string zlib_inflate(std::string_view body)
{
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(body.data()));
    stream.avail_in = static_cast<uInt>(body.size());
    if (inflateInit(&stream) != Z_OK)
    {
        throw std::runtime_error{"invalid compressed parameter set"};
    }

    std::string output;
    std::array<char, 4096> buffer{};
    int status{Z_OK};
    while (status == Z_OK)
    {
        stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
        stream.avail_out = static_cast<uInt>(buffer.size());
        status = inflate(&stream, Z_NO_FLUSH);
        output.append(buffer.data(), buffer.size() - stream.avail_out);
    }

    inflateEnd(&stream);
    if (status != Z_STREAM_END)
    {
        throw std::runtime_error{"invalid compressed parameter set"};
    }
    return output;
}

std::vector<std::string> split_physical_lines(std::string_view input)
{
    std::vector<std::string> result;
    std::string line;
    for (std::size_t i = 0; i < input.length();)
    {
        const char ch{input[i]};
        if (ch == '\r' || ch == '\n')
        {
            result.push_back(std::move(line));
            line.clear();
            ++i;
            if (ch == '\r' && i < input.length() && input[i] == '\n')
            {
                ++i;
            }
            continue;
        }
        line.push_back(ch);
        ++i;
    }
    if (!line.empty() || input.empty())
    {
        result.push_back(std::move(line));
    }
    return result;
}

std::vector<ProcessedLine> preprocess_lines(std::string_view input)
{
    std::vector<ProcessedLine> result;
    std::string current;
    std::size_t current_line{};
    bool continuing{};
    bool quote_continues{};
    const std::vector<std::string> physical_lines{split_physical_lines(input)};

    for (std::size_t i = 0; i < physical_lines.size(); ++i)
    {
        if (!continuing)
        {
            current_line = i + 1;
        }

        CommentStrippedLine stripped{strip_comment(physical_lines[i], quote_continues)};
        std::string piece{std::move(stripped.text)};
        if (continuing)
        {
            piece.erase(0, piece.find_first_not_of(" \t"));
        }
        current.append(piece);

        if (ends_with_continuation(current))
        {
            current.erase(current.find_last_not_of(" \t") + 1);
            current.pop_back();
            continuing = true;
            quote_continues = stripped.in_quote;
            continue;
        }

        result.push_back({std::move(current), current_line});
        current.clear();
        continuing = false;
        quote_continues = false;
    }

    if (!current.empty())
    {
        result.push_back({std::move(current), current_line});
    }

    return result;
}

SourceLocation location_at(SourceLocation start, std::size_t line, std::size_t column)
{
    SourceLocation result{std::move(start)};
    if (line > 1)
    {
        result.line += line - 1;
        result.column = column;
    }
    else
    {
        result.column += column - 1;
    }
    return result;
}

bool is_section_label(std::string_view line)
{
    line = trim(line);
    if (line.empty() || line.back() != ':')
    {
        return false;
    }
    line.remove_suffix(1);
    return !line.empty() && line.find_first_of(" \t=") == std::string_view::npos;
}

std::string section_name(std::string_view line)
{
    line = trim(line);
    line.remove_suffix(1);
    return std::string{line};
}

bool is_known_layer_section(std::string_view name)
{
    return name == "layer" || name == "mapping" || name == "transform" || name == "formula" || name == "inside" ||
        name == "outside" || name == "gradient" || name == "opacity" || name == "alpha";
}

bool is_complete_layer_state(LayerParseState state)
{
    return state == LayerParseState::EXPECT_OPACITY_OR_LAYER || state == LayerParseState::EXPECT_LAYER_AFTER_OPACITY;
}

std::optional<ParsedValue> parse_quoted_value(std::string_view line, std::size_t value_start)
{
    std::string value;
    bool escaped{};
    for (std::size_t pos = value_start + 1; pos < line.length(); ++pos)
    {
        const char ch{line[pos]};
        if (escaped)
        {
            if (ch == '"' || ch == '\\')
            {
                value.push_back(ch);
            }
            else
            {
                value.push_back('\\');
                value.push_back(ch);
            }
            escaped = false;
            continue;
        }
        if (ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (ch == '"')
        {
            return ParsedValue{std::move(value), pos + 1};
        }
        value.push_back(ch);
    }
    return std::nullopt;
}

class BodyParser
{
public:
    BodyParser(std::string_view input, SourceLocation source_location) :
        m_lines(preprocess_lines(input)),
        m_source_location(std::move(source_location))
    {
    }

    BasicParameterEntry parse_basic()
    {
        BasicParameterEntry result;
        for (const ProcessedLine &line : m_lines)
        {
            parse_basic_line(result, line);
        }
        result.diagnostics = std::move(m_diagnostics);
        return result;
    }

    ExtendedParameterEntry parse_extended()
    {
        ExtendedParameterEntry result;
        for (const ProcessedLine &line : m_lines)
        {
            parse_extended_line(result, line);
        }
        if (!m_seen_fractal)
        {
            error(ParseErrorCode::EXPECTED_FRACTAL_SECTION, 1, 1);
        }
        else if (!m_seen_layer)
        {
            error(ParseErrorCode::EXPECTED_LAYER_SECTION, 1, 1);
        }
        else if (!is_complete_layer_state(m_layer_state))
        {
            error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, 1, 1);
        }
        finish_layer(result);
        result.diagnostics = std::move(m_diagnostics);
        return result;
    }

private:
    void parse_basic_line(BasicParameterEntry &entry, const ProcessedLine &processed)
    {
        const std::string_view line{processed.text};
        const std::string_view stripped{trim(line)};
        if (stripped.empty())
        {
            return;
        }

        const std::size_t first_non_space{line.find_first_not_of(" \t")};
        if (is_section_label(stripped))
        {
            error(ParseErrorCode::EXPECTED_ASSIGNMENT, processed.line, first_non_space + 1);
            return;
        }

        std::vector<Parameter> assignments{parse_assignments(line, processed.line)};
        entry.assignments.insert(entry.assignments.end(), std::make_move_iterator(assignments.begin()),
            std::make_move_iterator(assignments.end()));
    }

    void parse_extended_line(ExtendedParameterEntry &entry, const ProcessedLine &processed)
    {
        const std::string_view line{processed.text};
        const std::string_view stripped{trim(line)};
        if (stripped.empty())
        {
            return;
        }

        const std::size_t first_non_space{line.find_first_not_of(" \t")};
        if (is_section_label(stripped))
        {
            start_extended_section(entry, section_name(stripped), processed.line, first_non_space + 1);
            return;
        }

        if (m_current_section == nullptr)
        {
            if (!m_drop_current_section)
            {
                error(ParseErrorCode::EXPECTED_SECTION_LABEL, processed.line, first_non_space + 1);
            }
            return;
        }

        std::vector<Parameter> assignments{parse_assignments(line, processed.line)};
        m_current_section->assignments.insert(m_current_section->assignments.end(),
            std::make_move_iterator(assignments.begin()), std::make_move_iterator(assignments.end()));
    }

    void start_extended_section(
        ExtendedParameterEntry &entry, std::string name, std::size_t line_number, std::size_t column)
    {
        if (!m_seen_fractal)
        {
            if (name != "fractal")
            {
                error(ParseErrorCode::EXPECTED_FRACTAL_SECTION, line_number, column);
                drop_section();
                return;
            }
            entry.fractal.name = std::move(name);
            m_current_section = &entry.fractal;
            m_drop_current_section = false;
            m_seen_fractal = true;
            m_layer_state = LayerParseState::EXPECT_LAYER;
            return;
        }

        if (name == "fractal")
        {
            error(ParseErrorCode::UNEXPECTED_PARAMETER_SECTION, line_number, column);
            drop_section();
            return;
        }
        if (!is_known_layer_section(name))
        {
            error(ParseErrorCode::UNEXPECTED_PARAMETER_SECTION, line_number, column);
            drop_section();
            return;
        }
        if (name == "layer")
        {
            if (m_seen_layer && !is_complete_layer_state(m_layer_state))
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
            }
            finish_layer(entry);
            m_current_layer.emplace();
            m_current_layer->layer.name = std::move(name);
            m_current_section = &m_current_layer->layer;
            m_drop_current_section = false;
            m_seen_layer = true;
            m_layer_state = LayerParseState::EXPECT_MAPPING;
            return;
        }
        if (!m_seen_layer)
        {
            error(ParseErrorCode::EXPECTED_LAYER_SECTION, line_number, column);
            drop_section();
            return;
        }
        start_layer_section(std::move(name), line_number, column);
    }

    void start_layer_section(std::string name, std::size_t line_number, std::size_t column)
    {
        if (!m_current_layer)
        {
            error(ParseErrorCode::EXPECTED_LAYER_SECTION, line_number, column);
            drop_section();
            return;
        }

        switch (m_layer_state)
        {
        case LayerParseState::EXPECT_LAYER:
            error(ParseErrorCode::EXPECTED_LAYER_SECTION, line_number, column);
            drop_section();
            return;
        case LayerParseState::EXPECT_MAPPING:
            if (name != "mapping")
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
                drop_section();
                return;
            }
            m_current_layer->mapping.name = std::move(name);
            m_current_section = &m_current_layer->mapping;
            m_drop_current_section = false;
            m_layer_state = LayerParseState::EXPECT_FORMULA;
            return;
        case LayerParseState::EXPECT_FORMULA:
            if (name == "transform")
            {
                m_current_section = &m_current_layer->transforms.emplace_back();
                m_current_section->name = std::move(name);
                m_drop_current_section = false;
                return;
            }
            if (name != "formula")
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
                drop_section();
                return;
            }
            m_current_layer->formula.name = std::move(name);
            m_current_section = &m_current_layer->formula;
            m_drop_current_section = false;
            m_layer_state = LayerParseState::EXPECT_INSIDE;
            return;
        case LayerParseState::EXPECT_INSIDE:
            if (name != "inside")
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
                drop_section();
                return;
            }
            m_current_layer->inside.name = std::move(name);
            m_current_section = &m_current_layer->inside;
            m_drop_current_section = false;
            m_layer_state = LayerParseState::EXPECT_OUTSIDE;
            return;
        case LayerParseState::EXPECT_OUTSIDE:
            if (name != "outside")
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
                drop_section();
                return;
            }
            m_current_layer->outside.name = std::move(name);
            m_current_section = &m_current_layer->outside;
            m_drop_current_section = false;
            m_layer_state = LayerParseState::EXPECT_GRADIENT;
            return;
        case LayerParseState::EXPECT_GRADIENT:
            if (name != "gradient")
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
                drop_section();
                return;
            }
            m_current_layer->gradient.name = std::move(name);
            m_current_section = &m_current_layer->gradient;
            m_drop_current_section = false;
            m_layer_state = LayerParseState::EXPECT_OPACITY_OR_LAYER;
            return;
        case LayerParseState::EXPECT_OPACITY_OR_LAYER:
            if (name != "opacity" && name != "alpha")
            {
                error(ParseErrorCode::EXPECTED_PARAMETER_SECTION, line_number, column);
                drop_section();
                return;
            }
            m_current_layer->opacity.emplace();
            m_current_layer->opacity->name = std::move(name);
            m_current_section = &*m_current_layer->opacity;
            m_drop_current_section = false;
            m_layer_state = LayerParseState::EXPECT_LAYER_AFTER_OPACITY;
            return;
        case LayerParseState::EXPECT_LAYER_AFTER_OPACITY:
            error(ParseErrorCode::EXPECTED_LAYER_SECTION, line_number, column);
            drop_section();
            return;
        }
    }

    void finish_layer(ExtendedParameterEntry &entry)
    {
        if (m_current_layer && is_complete_layer_state(m_layer_state))
        {
            entry.layers.push_back(std::move(*m_current_layer));
        }
        m_current_layer.reset();
    }

    void drop_section()
    {
        m_current_section = nullptr;
        m_drop_current_section = true;
    }

    std::vector<Parameter> parse_assignments(std::string_view line, std::size_t line_number)
    {
        std::vector<Parameter> result;
        std::size_t pos{};
        while (pos < line.length())
        {
            pos = skip_space(line, pos);
            if (pos >= line.length())
            {
                break;
            }

            const std::size_t assignment_start{pos};
            while (pos < line.length() && line[pos] != '=' && !is_space(line[pos]))
            {
                ++pos;
            }
            const std::size_t key_end{pos};
            if (assignment_start == key_end)
            {
                error(ParseErrorCode::EXPECTED_ASSIGNMENT, line_number, assignment_start + 1);
                break;
            }

            if (pos >= line.length() || line[pos] != '=')
            {
                error(ParseErrorCode::EXPECTED_ASSIGNMENT, line_number, assignment_start + 1);
                break;
            }
            ++pos;
            const std::size_t value_start{pos};
            if (value_start >= line.length() || is_space(line[value_start]))
            {
                error(ParseErrorCode::EXPECTED_VALUE, line_number, value_start + 1);
                break;
            }

            Parameter assignment;
            assignment.key = std::string{line.substr(assignment_start, key_end - assignment_start)};

            if (line[value_start] == '"')
            {
                std::optional<ParsedValue> value{parse_quoted_value(line, value_start)};
                if (!value)
                {
                    error(ParseErrorCode::UNTERMINATED_QUOTED_STRING, line_number, value_start + 1);
                    break;
                }
                assignment.value = std::move(value->text);
                pos = value->end;
            }
            else
            {
                while (pos < line.length() && !is_space(line[pos]))
                {
                    ++pos;
                }
                assignment.value = std::string{line.substr(value_start, pos - value_start)};
            }

            result.push_back(std::move(assignment));
        }
        return result;
    }

    void error(ParseErrorCode code, std::size_t line, std::size_t column)
    {
        m_diagnostics.push_back(ParseDiagnostic{code, location_at(m_source_location, line, column)});
    }

    std::vector<ProcessedLine> m_lines;
    SourceLocation m_source_location;
    std::optional<ParameterLayer> m_current_layer;
    ParameterSection *m_current_section{};
    bool m_drop_current_section{};
    bool m_seen_fractal{};
    bool m_seen_layer{};
    LayerParseState m_layer_state{LayerParseState::EXPECT_LAYER};
    std::vector<ParseDiagnostic> m_diagnostics;
};

} // namespace

std::string compress_parameter_set(std::string_view body)
{
    const std::string compressed{zlib_deflate(body)};
    const uLong crc{crc32(0, reinterpret_cast<const Bytef *>(compressed.data()), static_cast<uInt>(compressed.size()))};

    std::string payload;
    payload.reserve(compressed.size() + 4);
    payload.push_back(static_cast<char>(crc & 0xffU));
    payload.push_back(static_cast<char>((crc >> 8U) & 0xffU));
    payload.push_back(static_cast<char>((crc >> 16U) & 0xffU));
    payload.push_back(static_cast<char>((crc >> 24U) & 0xffU));
    payload.append(compressed);
    return wrap_payload(encode_uf_base64(payload));
}

std::string decompress_parameter_set(std::string_view body)
{
    const std::string encoded{compressed_payload(body)};
    const std::string decoded{decode_uf_base64(encoded)};
    if (decoded.size() < 4)
    {
        throw std::runtime_error{"invalid compressed parameter set"};
    }

    const auto expected_crc = static_cast<uLong>(static_cast<unsigned char>(decoded[0])) |
        (static_cast<uLong>(static_cast<unsigned char>(decoded[1])) << 8U) |
        (static_cast<uLong>(static_cast<unsigned char>(decoded[2])) << 16U) |
        (static_cast<uLong>(static_cast<unsigned char>(decoded[3])) << 24U);
    const std::string_view compressed{decoded.data() + 4, decoded.size() - 4};
    const uLong actual_crc{
        crc32(0, reinterpret_cast<const Bytef *>(compressed.data()), static_cast<uInt>(compressed.size()))};
    if (actual_crc != expected_crc)
    {
        throw std::runtime_error{"invalid compressed parameter set"};
    }

    return zlib_inflate(compressed);
}

BasicParameterEntry parse_basic_parameters(FileEntry file_entry)
{
    return BodyParser{file_entry.body, file_entry.body_range.begin}.parse_basic();
}

ExtendedParameterEntry parse_extended_parameters(FileEntry file_entry)
{
    if (is_compressed_parameter_set(file_entry.body))
    {
        try
        {
            file_entry.body = decompress_parameter_set(file_entry.body);
        }
        catch (const std::exception &)
        {
            ExtendedParameterEntry result;
            result.diagnostics.push_back(
                ParseDiagnostic{ParseErrorCode::INVALID_COMPRESSED_PARAMETER_SET, file_entry.body_range.begin});
            return result;
        }
    }
    return BodyParser{file_entry.body, file_entry.body_range.begin}.parse_extended();
}

} // namespace formula::parameter
