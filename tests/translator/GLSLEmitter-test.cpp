// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/parser/ParseOptions.h>
#include <formula/parser/Parser.h>
#include <formula/translator/GLSLEmitter.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>

namespace formula::test
{

namespace
{

std::string normalize_line_endings(std::string text)
{
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

std::string emit_basic_shader(std::string_view body)
{
    const parser::ParserPtr parser{parser::create_parser(body, parser::Options{})};
    const ast::FormulaSectionsPtr formula{parser->parse()};
    EXPECT_TRUE(parser->get_errors().empty());
    EXPECT_TRUE(formula);
    if (!formula)
    {
        return {};
    }
    return normalize_line_endings(codegen::emit_shader(*formula));
}

void expect_contains(std::string_view text, std::string_view snippet)
{
    EXPECT_NE(text.find(snippet), std::string_view::npos) << snippet;
}

TEST(TestGLSLEmitter, emitsHeaderAndUniformSnapshot)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pixel\n"
                                               "loop:\n"
                                               "  z = sqr(z) + p1\n"
                                               "bailout:\n"
                                               "  |z| < 4\n")};

    expect_contains(shader,
        "#version 450\n"
        "\n"
        "// Auto-generated fractal compute shader\n"
        "\n"
        "layout(local_size_x = 8, local_size_y = 8) in;\n"
        "\n"
        "layout(rgba32f, binding = 0) uniform image2D output_image;\n");
    expect_contains(shader,
        "layout(std140, binding = 1) uniform FractalParams {\n"
        "    vec2 p1;          // Parameter 1\n"
        "    vec2 p2;          // Parameter 2\n"
        "    vec2 p3;          // Parameter 3\n"
        "    vec2 p4;          // Parameter 4\n"
        "    vec2 p5;          // Parameter 5\n");
}

TEST(TestGLSLEmitter, emitsHelperDeclarationSnapshot)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pixel\n")};

    expect_contains(shader,
        "vec2 c_mul(vec2 a, vec2 b) {\n"
        "    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);\n"
        "}\n");
    expect_contains(shader,
        "vec2 c_fn1(vec2 z) { return c_ident(z); }\n"
        "vec2 c_fn2(vec2 z) { return c_ident(z); }\n"
        "vec2 c_fn3(vec2 z) { return c_ident(z); }\n"
        "vec2 c_fn4(vec2 z) { return c_ident(z); }\n");
}

TEST(TestGLSLEmitter, emitsSectionStructureSnapshot)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pixel\n"
                                               "loop:\n"
                                               "  z = sqr(z) + p1\n"
                                               "bailout:\n"
                                               "  |z| < 4\n")};

    expect_contains(shader,
        "void main() {\n"
        "    // Get pixel coordinates\n"
        "    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);\n");
    expect_contains(shader,
        "    // Variable initialization\n"
        "    vec2 z = pixel;       // Default initialization\n"
        "    float lastsqr = 0.0;\n"
        "    uint iter = 0u;\n");
    expect_contains(shader,
        "    // Main iteration loop\n"
        "    while (iter < maxit) {\n");
    expect_contains(shader,
        "    // Output color based on iteration count\n"
        "    float t = float(iter) / float(maxit);\n"
        "    vec4 color = vec4(t, t * t, sqrt(t), 1.0);\n"
        "    imageStore(output_image, pixel_coords, color);\n");
}

TEST(TestGLSLEmitter, declaresAssignedUserVariables)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  c = pixel\n"
                                               "loop:\n"
                                               "  z = sqr(z) + c\n")};

    expect_contains(shader,
        "    // Formula variables\n"
        "    vec2 c = vec2(0.0, 0.0);\n");
    expect_contains(shader, "    c = pixel;\n");
}

TEST(TestGLSLEmitter, declaresUnknownUserVariableReads)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = foo + pixel\n")};

    expect_contains(shader,
        "    // Formula variables\n"
        "    vec2 foo = vec2(0.0, 0.0);\n");
    expect_contains(shader, "    z = c_add(foo, pixel);\n");
}

} // namespace

} // namespace formula::test
