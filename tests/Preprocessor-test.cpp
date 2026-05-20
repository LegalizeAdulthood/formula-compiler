#include <formula/Preprocessor.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>

using namespace formula::preprocessor;

namespace
{

bool has_macro(const Preprocessor &preprocessor, std::string_view macro)
{
    const MacroList &macros = preprocessor.macros();
    const std::string name{macro};
    return std::find(macros.begin(), macros.end(), name) != macros.end();
}

} // namespace

TEST(TestPreprocessor, predefinedMacroControlsIfdef)
{
    Preprocessor preprocessor{MacroList{"answer"}};

    const std::string result = preprocessor.process("$ifdef answer\n"
                                                    "z=1\n"
                                                    "$endif\n");

    EXPECT_EQ("z=1\n", result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, defineCreatesSymbol)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$define test\n"
                                                    "$ifdef test\n"
                                                    "z=1\n"
                                                    "$endif\n");

    EXPECT_EQ("z=1\n", result);
    EXPECT_TRUE(preprocessor.errors().empty());
    EXPECT_TRUE(has_macro(preprocessor, "test"));
}

TEST(TestPreprocessor, macrosAreSorted)
{
    Preprocessor preprocessor{MacroList{"zeta", "alpha"}};

    const std::string result = preprocessor.process("$define middle\n");

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(MacroList({"alpha", "middle", "zeta"}), preprocessor.macros());
}

TEST(TestPreprocessor, ultraFractal6MacrosDefineVersionAndPreviousVersions)
{
    Preprocessor preprocessor{UltraFractalMacros::ULTRAFRACTAL6};

    const std::string result = preprocessor.process("$ifdef ULTRAFRACTAL\n"
                                                    "u=1\n"
                                                    "$endif\n"
                                                    "$ifdef VER20\n"
                                                    "v=1\n"
                                                    "$endif\n"
                                                    "$ifdef VER30\n"
                                                    "a=1\n"
                                                    "$endif\n"
                                                    "$ifdef VER40\n"
                                                    "b=1\n"
                                                    "$endif\n"
                                                    "$ifdef VER50\n"
                                                    "c=1\n"
                                                    "$endif\n"
                                                    "$ifdef VER60\n"
                                                    "z=1\n"
                                                    "$endif\n");

    EXPECT_EQ("u=1\n"
              "v=1\n"
              "a=1\n"
              "b=1\n"
              "c=1\n"
              "z=1\n",
        result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, ultraFractalVersionsDefineCumulativeSymbols)
{
    const MacroList none = predefined_macros(UltraFractalMacros::NONE);
    const MacroList ultra_fractal3 = predefined_macros(UltraFractalMacros::ULTRAFRACTAL3);
    const MacroList ultra_fractal4 = predefined_macros(UltraFractalMacros::ULTRAFRACTAL4);
    const MacroList ultra_fractal5 = predefined_macros(UltraFractalMacros::ULTRAFRACTAL5);
    const MacroList ultra_fractal6 = predefined_macros(UltraFractalMacros::ULTRAFRACTAL6);

    EXPECT_TRUE(none.empty());
    EXPECT_EQ(MacroList({"ULTRAFRACTAL", "VER20", "VER30"}), ultra_fractal3);
    EXPECT_EQ(MacroList({"ULTRAFRACTAL", "VER20", "VER30", "VER40"}), ultra_fractal4);
    EXPECT_EQ(MacroList({"ULTRAFRACTAL", "VER20", "VER30", "VER40", "VER50"}), ultra_fractal5);
    EXPECT_EQ(MacroList({"ULTRAFRACTAL", "VER20", "VER30", "VER40", "VER50", "VER60"}), ultra_fractal6);
}

TEST(TestPreprocessor, predefinedMacroListCanBeCombinedWithCustomMacros)
{
    MacroList macros = predefined_macros(UltraFractalMacros::ULTRAFRACTAL6);
    macros.push_back("answer");
    Preprocessor preprocessor{macros};

    const std::string result = preprocessor.process("$ifdef VER60\n"
                                                    "z=1\n"
                                                    "$endif\n"
                                                    "$ifdef answer\n"
                                                    "a=1\n"
                                                    "$endif\n");

    EXPECT_EQ("z=1\n"
              "a=1\n",
        result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, unknownPredefinedMacroIsInactive)
{
    Preprocessor preprocessor{UltraFractalMacros::ULTRAFRACTAL6};

    const std::string result = preprocessor.process("$ifdef VER70\n"
                                                    "z=1\n"
                                                    "$endif\n");

    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, defineEnablesIfdef)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$define TEST\n"
                                                    "$ifdef TEST\n"
                                                    "z=1\n"
                                                    "$endif\n");

    EXPECT_EQ("z=1\n", result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, undefDisablesLaterIfdef)
{
    Preprocessor preprocessor{MacroList{"test"}};

    const std::string result = preprocessor.process("$undef TEST\n"
                                                    "$ifdef TEST\n"
                                                    "z=1\n"
                                                    "$endif\n");

    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, elseSelectsInactiveBranch)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$ifdef NO\n"
                                                    "z=1\n"
                                                    "$else\n"
                                                    "z=2\n"
                                                    "$endif\n");

    EXPECT_EQ("z=2\n", result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, nestedIfdefsRespectParentActivity)
{
    Preprocessor preprocessor{MacroList{"inner"}};

    const std::string result = preprocessor.process("$ifdef OUT\n"
                                                    "$ifdef INNER\n"
                                                    "z=1\n"
                                                    "$endif\n"
                                                    "$endif\n");

    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, directivesAndSymbolsAreCaseInsensitive)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$DeFiNe Test\n"
                                                    "$IFDEF tESt\n"
                                                    "z=1\n"
                                                    "$ENDIF\n");

    EXPECT_EQ("z=1\n", result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, preservesNewlineStyle)
{
    Preprocessor preprocessor{MacroList{"test"}};

    const std::string result = preprocessor.process("$ifdef TEST\r\n"
                                                    "z=1\r\n"
                                                    "$endif");

    EXPECT_EQ("z=1\r\n", result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, leavesActiveSourceTextUnchanged)
{
    Preprocessor preprocessor{MacroList{"test"}};

    const std::string result = preprocessor.process("caption=\"test\" ; test\n"
                                                    "z=test\n");

    EXPECT_EQ("caption=\"test\" ; test\n"
              "z=test\n",
        result);
    EXPECT_TRUE(preprocessor.errors().empty());
}

TEST(TestPreprocessor, reportsUnclosedIfdef)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$ifdef TEST\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ(ErrorCode::EXPECTED_DIRECTIVE_ENDIF, preprocessor.errors()[0].code);
    EXPECT_EQ(2U, preprocessor.errors()[0].position.line);
}

TEST(TestPreprocessor, diagnosticsRetainSourceFilename)
{
    Preprocessor preprocessor{MacroList{}, "common.ulb"};

    const std::string result = preprocessor.process("$else\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ("common.ulb", preprocessor.errors()[0].position.filename);
}

TEST(TestPreprocessor, reportsUnexpectedElse)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$else\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ(ErrorCode::UNEXPECTED_DIRECTIVE_ELSE, preprocessor.errors()[0].code);
    EXPECT_EQ(1U, preprocessor.errors()[0].position.line);
}

TEST(TestPreprocessor, reportsUnexpectedEndif)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$endif\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ(ErrorCode::UNEXPECTED_DIRECTIVE_ENDIF, preprocessor.errors()[0].code);
}

TEST(TestPreprocessor, reportsDuplicateElse)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$ifdef TEST\n"
                                                    "$else\n"
                                                    "$else\n"
                                                    "$endif\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ(ErrorCode::DUPLICATE_DIRECTIVE_ELSE, preprocessor.errors()[0].code);
    EXPECT_EQ(3U, preprocessor.errors()[0].position.line);
}

TEST(TestPreprocessor, reportsMissingSymbol)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$define\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ(ErrorCode::EXPECTED_DIRECTIVE_SYMBOL, preprocessor.errors()[0].code);
}

TEST(TestPreprocessor, reportsInvalidDirective)
{
    Preprocessor preprocessor;

    const std::string result = preprocessor.process("$foo TEST\n");

    EXPECT_TRUE(result.empty());
    ASSERT_EQ(1U, preprocessor.errors().size());
    EXPECT_EQ(ErrorCode::INVALID_COMPILER_DIRECTIVE, preprocessor.errors()[0].code);
}
