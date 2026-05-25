// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/facade/Formula.h>
#include <formula/parser/ParseOptions.h>
#include <formula/parser/Parser.h>
#include <formula/translator/GLSLEmitter.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

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

void expect_complex_near(const Complex &expected, const Complex &actual)
{
    EXPECT_NEAR(expected.re, actual.re, 1.0e-12);
    EXPECT_NEAR(expected.im, actual.im, 1.0e-12);
}

void validate_glsl_compute_shader(std::string_view name, std::string_view shader)
{
#ifdef FORMULA_GLSLANG_VALIDATOR
    const std::filesystem::path path{std::filesystem::temp_directory_path() / (std::string{name} + ".comp")};
    {
        std::ofstream output{path};
        ASSERT_TRUE(output);
        output << shader;
    }

    const std::string command{"\"" FORMULA_GLSLANG_VALIDATOR "\" -S comp \"" + path.string() + "\""};
    const int result{std::system(command.c_str())};
    std::filesystem::remove(path);
    EXPECT_EQ(0, result);
#else
    (void) name;
    (void) shader;
    GTEST_SKIP() << "glslangValidator not found";
#endif
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
        "struct FormulaResult {\n"
        "    vec2 z;\n"
        "    vec2 bailout;\n"
        "    uint iter;\n"
        "    uint escaped;\n"
        "};\n"
        "\n"
        "layout(std430, binding = 2) buffer FormulaResults {\n"
        "    FormulaResult formula_results[];\n"
        "};\n");
    expect_contains(shader,
        "layout(std140, binding = 1) uniform FractalParams {\n"
        "    vec2 p1;          // Parameter 1\n"
        "    vec2 p2;          // Parameter 2\n"
        "    vec2 p3;          // Parameter 3\n"
        "    vec2 p4;          // Parameter 4\n"
        "    vec2 p5;          // Parameter 5\n");
    expect_contains(shader,
        "    vec2 center;      // View center\n"
        "    vec2 magxmag;     // Magnification and x magnitude\n"
        "    vec2 rotskew;     // Rotation and skew\n"
        "    vec2 ismand;      // Mandelbrot/Julia toggle\n");
    expect_contains(shader,
        "    uint fn1_selector; // Default sin\n"
        "    uint fn2_selector; // Default sqr\n"
        "    uint fn3_selector; // Default sinh\n"
        "    uint fn4_selector; // Default cosh\n");
    expect_contains(shader, "    uint random_seed; // Client random seed\n");
    expect_contains(shader,
        "const uint FUNCTION_SIN = 0u;\n"
        "const uint FUNCTION_COS = 1u;\n"
        "const uint FUNCTION_SINH = 2u;\n"
        "const uint FUNCTION_COSH = 3u;\n");
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
        "vec2 c_fn1(vec2 z, inout float lastsqr_value, inout uint random_state, inout vec2 rand) {\n"
        "    return c_dispatch_function(fn1_selector, z, lastsqr_value, random_state, rand);\n"
        "}\n"
        "vec2 c_fn2(vec2 z, inout float lastsqr_value, inout uint random_state, inout vec2 rand) {\n"
        "    return c_dispatch_function(fn2_selector, z, lastsqr_value, random_state, rand);\n"
        "}\n");
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
    expect_contains(shader, "    uint pixel_index = uint(pixel_coords.y) * resolution.x + uint(pixel_coords.x);\n");
    expect_contains(shader,
        "    // Screen-derived predefined variables\n"
        "    vec2 scrnmax = vec2(resolution);\n"
        "    vec2 scrnpix = vec2(pixel_coords);\n"
        "    vec2 whitesq = vec2(float((pixel_coords.x + pixel_coords.y) & 1), 0.0);\n");
    expect_contains(shader,
        "    // Variable initialization\n"
        "    vec2 z = pixel;       // Default initialization\n"
        "    vec2 bailout_value = vec2(1.0, 0.0);\n"
        "    float lastsqr_value = 0.0;\n"
        "    uint iter = 0u;\n");
    expect_contains(shader, "    uint escaped = 0u;\n");
    expect_contains(shader,
        "    // Main iteration loop\n"
        "    while (iter < maxit) {\n");
    expect_contains(shader,
        "    // Debug output color based on iteration count\n"
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

TEST(TestGLSLEmitter, emitsIntegerLiteralAsComplexValue)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = 0\n")};

    expect_contains(shader, "    z = vec2(0.0, 0.0);\n");
}

TEST(TestGLSLEmitter, emitsFloatLiteralAsComplexValue)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = z + 1.5\n")};

    expect_contains(shader, "    z = c_add(z, vec2(1.5, 0.0));\n");
}

TEST(TestGLSLEmitter, emitsComplexLiteralAsComplexValue)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = z + (1, 2)\n")};

    expect_contains(shader, "    z = c_add(z, vec2(1.0, 2.0));\n");
}

TEST(TestGLSLEmitter, emitsArithmeticThroughComplexHelpers)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = (z + p1) - p2\n"
                                               "  z = z * p3\n"
                                               "  z = z / p4\n"
                                               "  z = z ^ p5\n")};

    expect_contains(shader, "    z = c_sub(c_add(z, p1), p2);\n");
    expect_contains(shader, "    z = c_mul(z, p3);\n");
    expect_contains(shader, "    z = c_div(z, p4);\n");
    expect_contains(shader, "    z = c_pow(z, p5);\n");
}

TEST(TestGLSLEmitter, emitsUnaryArithmeticThroughComplexHelpers)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = -z\n"
                                               "  z = +z\n")};

    expect_contains(shader, "vec2 c_neg(vec2 z) { return vec2(-z.x, -z.y); }\n");
    expect_contains(shader, "    z = c_neg(z);\n");
    expect_contains(shader, "    z = z;\n");
}

TEST(TestGLSLEmitter, emitsModulusAsModulusSquared)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = |z|\n")};

    expect_contains(shader,
        "vec2 c_mod_sqr(vec2 z) {\n"
        "    return vec2(z.x * z.x + z.y * z.y, 0.0);\n"
        "}\n");
    expect_contains(shader, "    z = c_mod_sqr(z);\n");
}

TEST(TestGLSLEmitter, emitsAbsoluteBuiltinsWithBasicSemantics)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = abs((3, -4))\n"
                                               "  z = cabs((3, 4))\n")};

    expect_contains(shader,
        "vec2 c_abs(vec2 z) {\n"
        "    return vec2(abs(z.x), abs(z.y));\n"
        "}\n");
    expect_contains(shader,
        "vec2 c_cabs(vec2 z) {\n"
        "    return c_mag(z);\n"
        "}\n");
    expect_contains(shader, "    z = c_abs(vec2(3.0, -4.0));\n");
    expect_contains(shader, "    z = c_cabs(vec2(3.0, 4.0));\n");
}

TEST(TestGLSLEmitter, emitsBasicBuiltinFunctionMapping)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = sin(p1)\n"
                                               "  z = cos(p1)\n"
                                               "  z = sinh(p1)\n"
                                               "  z = cosh(p1)\n"
                                               "  z = cosxx(p1)\n"
                                               "  z = tan(p1)\n"
                                               "  z = cotan(p1)\n"
                                               "  z = tanh(p1)\n"
                                               "  z = cotanh(p1)\n"
                                               "  z = sqr(p1)\n"
                                               "  z = log(p1)\n"
                                               "  z = exp(p1)\n"
                                               "  z = abs(p1)\n"
                                               "  z = conj(p1)\n"
                                               "  z = real(p1)\n"
                                               "  z = imag(p1)\n"
                                               "  z = flip(p1)\n"
                                               "  z = fn1(p1)\n"
                                               "  z = fn2(p1)\n"
                                               "  z = fn3(p1)\n"
                                               "  z = fn4(p1)\n"
                                               "  z = srand(p1)\n"
                                               "  z = asin(p1)\n"
                                               "  z = asinh(p1)\n"
                                               "  z = acos(p1)\n"
                                               "  z = acosh(p1)\n"
                                               "  z = atan(p1)\n"
                                               "  z = atanh(p1)\n"
                                               "  z = sqrt(p1)\n"
                                               "  z = cabs(p1)\n"
                                               "  z = floor(p1)\n"
                                               "  z = ceil(p1)\n"
                                               "  z = trunc(p1)\n"
                                               "  z = round(p1)\n"
                                               "  z = ident(p1)\n"
                                               "  z = one(p1)\n"
                                               "  z = zero(p1)\n")};

    // clang-format off
    constexpr std::string_view functions[]{
        "sin", "cos", "sinh", "cosh", "cosxx", "tan", "cotan", "tanh", "cotanh",
        "log", "exp", "abs", "conj", "real", "imag", "flip",
        "asin", "asinh", "acos", "acosh", "atan", "atanh", "sqrt", "cabs",
        "floor", "ceil", "trunc", "round", "ident", "one", "zero",
    };
    // clang-format on
    for (const std::string_view function : functions)
    {
        expect_contains(shader, "    z = c_" + std::string{function} + "(p1);\n");
    }
    expect_contains(shader, "    z = c_sqr(p1, lastsqr_value);\n");
    expect_contains(shader, "    z = c_srand(p1, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn1(p1, lastsqr_value, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn2(p1, lastsqr_value, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn3(p1, lastsqr_value, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn4(p1, lastsqr_value, random_state, rand);\n");
}

TEST(TestGLSLEmitter, emitsCorrectedBuiltinHelperSemantics)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = flip(p1)\n"
                                               "  z = cosxx(p1)\n"
                                               "  z = real(p1)\n"
                                               "  z = imag(p1)\n")};

    expect_contains(shader, "vec2 c_real(vec2 z) { return vec2(z.x, 0.0); }\n");
    expect_contains(shader, "vec2 c_imag(vec2 z) { return vec2(z.y, 0.0); }\n");
    expect_contains(shader, "vec2 c_recip(vec2 z) { return c_div(vec2(1.0, 0.0), z); }\n");
    expect_contains(shader, "vec2 c_srand(vec2 z, inout uint random_state, inout vec2 rand) {\n");
    expect_contains(shader,
        "vec2 c_flip(vec2 z) {\n"
        "    return vec2(z.y, z.x);\n"
        "}\n");
    expect_contains(shader,
        "vec2 c_cosxx(vec2 z) {\n"
        "    return vec2(cos(z.x) * cosh(z.y), sin(z.x) * sinh(z.y));\n"
        "}\n");
}

TEST(TestGLSLEmitter, emitsSqrWithLastsqrUpdate)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = sqr((3, 4))\n"
                                               "  z = lastsqr\n")};

    expect_contains(shader,
        "vec2 c_sqr_value(vec2 z) {\n"
        "    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);\n"
        "}\n");
    expect_contains(shader,
        "vec2 c_sqr(vec2 z, inout float lastsqr_value) {\n"
        "    lastsqr_value = z.x * z.x + z.y * z.y;\n"
        "    return c_sqr_value(z);\n"
        "}\n");
    expect_contains(shader, "    float lastsqr_value = 0.0;\n");
    expect_contains(shader, "    z = c_sqr(vec2(3.0, 4.0), lastsqr_value);\n");
    expect_contains(shader, "    z = vec2(lastsqr_value, 0.0);\n");
}

TEST(TestGLSLEmitter, emitsFunctionSelectorDispatch)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = fn1(p1)\n"
                                               "  z = fn2(p1)\n"
                                               "  z = fn3(p1)\n"
                                               "  z = fn4(p1)\n")};

    expect_contains(shader,
        "vec2 c_dispatch_function(uint selector, vec2 z, inout float lastsqr_value,\n"
        "    inout uint random_state, inout vec2 rand) {\n");
    expect_contains(shader, "    case FUNCTION_SIN: return c_sin(z);\n");
    expect_contains(shader, "    case FUNCTION_COS: return c_cos(z);\n");
    expect_contains(shader, "    case FUNCTION_SINH: return c_sinh(z);\n");
    expect_contains(shader, "    case FUNCTION_COSH: return c_cosh(z);\n");
    expect_contains(shader, "    case FUNCTION_SQR: return c_sqr(z, lastsqr_value);\n");
    expect_contains(shader, "    case FUNCTION_SRAND: return c_srand(z, random_state, rand);\n");
    expect_contains(shader, "    return c_ident(z);\n");
    expect_contains(shader, "    z = c_fn1(p1, lastsqr_value, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn2(p1, lastsqr_value, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn3(p1, lastsqr_value, random_state, rand);\n");
    expect_contains(shader, "    z = c_fn4(p1, lastsqr_value, random_state, rand);\n");
}

TEST(TestGLSLEmitter, emitsRandomStateAndSrandReset)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = srand(1234)\n"
                                               "loop:\n"
                                               "  z = rand\n")};

    expect_contains(shader,
        "float c_random_float(inout uint random_state) {\n"
        "    random_state = random_state * 1664525u + 1013904223u;\n"
        "    return float(random_state & 0x00ffffffu) / 16777216.0;\n"
        "}\n");
    expect_contains(shader,
        "vec2 c_next_rand(inout uint random_state) {\n"
        "    return vec2(c_random_float(random_state), c_random_float(random_state));\n"
        "}\n");
    expect_contains(shader,
        "vec2 c_srand(vec2 z, inout uint random_state, inout vec2 rand) {\n"
        "    random_state = uint(z.x);\n"
        "    rand = vec2(0.0, 0.0);\n"
        "    return vec2(0.0, 0.0);\n"
        "}\n");
    expect_contains(
        shader, "    uint random_state = random_seed ^ uint(pixel_coords.x) ^ (uint(pixel_coords.y) << 16u);\n");
    expect_contains(shader, "    vec2 rand = vec2(0.0, 0.0);\n");
    expect_contains(shader, "    z = c_srand(vec2(1234.0, 0.0), random_state, rand);\n");
    expect_contains(shader,
        "    while (iter < maxit) {\n"
        "        rand = c_next_rand(random_state);\n");
    expect_contains(shader, "        z = rand;\n");
}

TEST(TestGLSLEmitter, emitsPredefinedVariablesAsComplexValues)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pi + e + maxit\n"
                                               "  z = scrnmax + scrnpix + whitesq\n"
                                               "  z = ismand + center + magxmag + rotskew\n")};

    expect_contains(shader, "    z = c_add(c_add(vec2(pi, 0.0), vec2(e, 0.0)), vec2(float(maxit), 0.0));\n");
    expect_contains(shader, "    z = c_add(c_add(scrnmax, scrnpix), whitesq);\n");
    expect_contains(shader, "    z = c_add(c_add(c_add(ismand, center), magxmag), rotskew);\n");
}

TEST(TestGLSLEmitter, emitsComparisonOperatorsThroughHelpers)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = z < p1\n"
                                               "  z = z <= p1\n"
                                               "  z = z > p1\n"
                                               "  z = z >= p1\n"
                                               "  z = z == p1\n"
                                               "  z = z != p1\n")};

    expect_contains(shader, "vec2 c_lt(vec2 a, vec2 b) { return c_bool(a.x < b.x); }\n");
    expect_contains(shader, "vec2 c_eq(vec2 a, vec2 b) { return c_bool(a.x == b.x && a.y == b.y); }\n");
    expect_contains(shader, "    z = c_lt(z, p1);\n");
    expect_contains(shader, "    z = c_le(z, p1);\n");
    expect_contains(shader, "    z = c_gt(z, p1);\n");
    expect_contains(shader, "    z = c_ge(z, p1);\n");
    expect_contains(shader, "    z = c_eq(z, p1);\n");
    expect_contains(shader, "    z = c_ne(z, p1);\n");
}

TEST(TestGLSLEmitter, emitsLogicalOperatorsThroughHelpers)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = (z < p1) && (z != p2)\n"
                                               "  z = (z < p1) || (z != p2)\n")};

    expect_contains(shader, "vec2 c_and(vec2 a, vec2 b) { return c_bool(a.x != 0.0 && b.x != 0.0); }\n");
    expect_contains(shader, "vec2 c_or(vec2 a, vec2 b) { return c_bool(a.x != 0.0 || b.x != 0.0); }\n");
    expect_contains(shader, "    z = c_and(c_lt(z, p1), c_ne(z, p2));\n");
    expect_contains(shader, "    z = c_or(c_lt(z, p1), c_ne(z, p2));\n");
}

TEST(TestGLSLEmitter, emitsConditionalTruthiness)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  if z < p1\n"
                                               "    z = p1\n"
                                               "  elseif z > p2\n"
                                               "    z = p2\n"
                                               "  else\n"
                                               "    z = p3\n"
                                               "  endif\n")};

    expect_contains(shader, "bool c_truth(vec2 value) { return value.x != 0.0; }\n");
    expect_contains(shader, "    if (c_truth(c_lt(z, p1))) {\n");
    expect_contains(shader,
        "    } else {\n"
        "        if (c_truth(c_gt(z, p2))) {\n");
}

TEST(TestGLSLEmitter, emitsNestedConditionalTruthiness)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  if z < p1\n"
                                               "    if z > p2\n"
                                               "      z = p3\n"
                                               "    endif\n"
                                               "  endif\n")};

    expect_contains(shader,
        "    if (c_truth(c_lt(z, p1))) {\n"
        "        if (c_truth(c_gt(z, p2))) {\n"
        "            z = p3;\n");
}

TEST(TestGLSLEmitter, emitsBailoutTruthiness)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pixel\n"
                                               "bailout:\n"
                                               "  |z| < 4\n")};

    expect_contains(shader, "        bailout_value = c_lt(c_mod_sqr(z), vec2(4.0, 0.0));\n");
    expect_contains(shader,
        "        if (!c_truth(bailout_value)) {\n"
        "            escaped = 1u;\n"
        "            break;\n"
        "        }\n");
}

TEST(TestGLSLEmitter, emitsDefaultBailoutResult)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pixel\n")};

    expect_contains(shader, "        bailout_value = c_bool(c_mod_sqr(z).x <= bailout * bailout);\n");
    expect_contains(shader,
        "        if (!c_truth(bailout_value)) {\n"
        "            escaped = 1u;\n"
        "            break;\n"
        "        }\n");
}

TEST(TestGLSLEmitter, emitsFormulaResultOutput)
{
    const std::string shader{emit_basic_shader("init:\n"
                                               "  z = pixel\n"
                                               "bailout:\n"
                                               "  |z| < 4\n")};

    expect_contains(shader,
        "    // Formula result output\n"
        "    formula_results[pixel_index].z = z;\n"
        "    formula_results[pixel_index].bailout = bailout_value;\n"
        "    formula_results[pixel_index].iter = iter;\n"
        "    formula_results[pixel_index].escaped = escaped;\n");
}

TEST(TestGLSLEmitter, loweredFixturesMatchKnownInterpreterResults)
{
    struct EquivalenceFixture
    {
        std::string body;
        Section section;
        std::map<std::string, Complex> values;
        Complex expected;
        std::vector<std::string_view> snippets;
    };
    const std::vector<EquivalenceFixture> fixtures{
        {
            "bailout:\n"
            "  abs((-3, -4))\n",
            Section::BAILOUT,
            {},
            {3.0, 4.0},
            {"bailout_value = c_abs(vec2(-3.0, -4.0));\n"},
        },
        {
            "bailout:\n"
            "  |(3, 4)|\n",
            Section::BAILOUT,
            {},
            {25.0, 0.0},
            {"bailout_value = c_mod_sqr(vec2(3.0, 4.0));\n"},
        },
        {
            "init:\n"
            "  z = sqr((3, 4))\n"
            "bailout:\n"
            "  lastsqr\n",
            Section::INITIALIZE,
            {},
            {-7.0, 24.0},
            {
                "z = c_sqr(vec2(3.0, 4.0), lastsqr_value);\n",
                "bailout_value = vec2(lastsqr_value, 0.0);\n",
            },
        },
        {
            "bailout:\n"
            "  (1, 2) == (1, 2)\n",
            Section::BAILOUT,
            {},
            {1.0, 0.0},
            {"bailout_value = c_eq(vec2(1.0, 2.0), vec2(1.0, 2.0));\n"},
        },
        {
            "bailout:\n"
            "  (0, 1) || (0, 0)\n",
            Section::BAILOUT,
            {},
            {0.0, 0.0},
            {"bailout_value = c_or(vec2(0.0, 1.0), vec2(0.0, 0.0));\n"},
        },
        {
            "bailout:\n"
            "  cosxx(1 + flip(2))\n",
            Section::BAILOUT,
            {},
            {2.0327230070196656, 3.0518977991518},
            {
                "bailout_value = "
                "c_cosxx(c_add(vec2(1.0, 0.0), c_flip(vec2(2.0, 0.0))));\n",
            },
        },
        {
            "bailout:\n"
            "  fn2((3, 4))\n",
            Section::BAILOUT,
            {},
            {-7.0, 24.0},
            {
                "bailout_value = "
                "c_fn2(vec2(3.0, 4.0), lastsqr_value, random_state, rand);\n",
            },
        },
        {
            "init:\n"
            "  z = pixel + p1\n",
            Section::INITIALIZE,
            {{"pixel", {1.0, 2.0}}, {"p1", {3.0, 4.0}}},
            {4.0, 6.0},
            {"z = c_add(pixel, p1);\n"},
        },
    };

    for (const EquivalenceFixture &fixture : fixtures)
    {
        const FormulaPtr formula{create_formula(fixture.body, parser::Options{})};
        ASSERT_TRUE(formula) << fixture.body;
        for (const auto &value : fixture.values)
        {
            formula->set_value(value.first, value.second);
        }
        expect_complex_near(fixture.expected, formula->interpret(fixture.section));

        const std::string shader{emit_basic_shader(fixture.body)};
        for (const std::string_view snippet : fixture.snippets)
        {
            expect_contains(shader, snippet);
        }
    }
}

TEST(TestGLSLEmitter, validatesRepresentativeShadersWithGlslang)
{
    struct ShaderFixture
    {
        std::string name;
        std::string body;
    };
    const std::vector<ShaderFixture> fixtures{
        {"basic",
            "init:\n"
            "  c = pixel\n"
            "loop:\n"
            "  z = sqr(z) + c\n"
            "bailout:\n"
            "  |z| < 4\n"},
        {"conditional",
            "init:\n"
            "  if pixel < p1\n"
            "    z = fn1(pixel)\n"
            "  else\n"
            "    z = abs(pixel) + cabs(p2)\n"
            "  endif\n"
            "loop:\n"
            "  z = z * p3 + rand\n"
            "bailout:\n"
            "  z != p4\n"},
    };

    for (const ShaderFixture &fixture : fixtures)
    {
        validate_glsl_compute_shader(fixture.name, emit_basic_shader(fixture.body));
    }
}

} // namespace

} // namespace formula::test
