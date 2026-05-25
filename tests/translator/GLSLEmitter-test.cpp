// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/parser/ParseOptions.h>
#include <formula/parser/Parser.h>
#include <formula/translator/GLSLEmitter.h>

#include <gtest/gtest.h>

#include <string>

namespace formula::test
{

namespace
{

TEST(TestGLSLEmitter, emitsExperimentalShaderString)
{
    const parser::ParserPtr parser{parser::create_parser("init:\n"
                                                         "  z = pixel\n"
                                                         "loop:\n"
                                                         "  z = sqr(z) + p1\n"
                                                         "bailout:\n"
                                                         "  |z| < 4\n",
        parser::Options{})};
    const ast::FormulaSectionsPtr formula{parser->parse()};
    ASSERT_TRUE(parser->get_errors().empty());
    ASSERT_TRUE(formula);

    const std::string shader{codegen::emit_shader(*formula)};

    EXPECT_NE(shader.find("#version 450"), std::string::npos);
    EXPECT_NE(shader.find("layout(rgba32f, binding = 0) uniform image2D output_image;"), std::string::npos);
    EXPECT_NE(shader.find("void main()"), std::string::npos);
    EXPECT_NE(shader.find("imageStore(output_image"), std::string::npos);
}

} // namespace

} // namespace formula::test
