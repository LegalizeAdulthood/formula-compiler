// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedInterpreter.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace formula::test
{

namespace
{

FileEntry formula_entry(std::string body)
{
    FileEntry entry;
    entry.name = "Formula";
    entry.body = std::move(body);
    return entry;
}

ExtendedInterpreterOptions options()
{
    ExtendedInterpreterOptions result;
    result.parser.dialect = Dialect::EXTENDED;
    return result;
}

ExtendedInterpreterOptions options(parser::EntryKind entry_kind)
{
    ExtendedInterpreterOptions result{options()};
    result.parser.entry_kind = entry_kind;
    return result;
}

Value interpret_init(std::string expression)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n" + std::move(expression)), options()};
    EXPECT_TRUE(interpreter.ok());
    return interpreter.interpret(Section::INITIALIZE);
}

} // namespace

TEST(TestExtendedInterpreter, parseFailureBlocksExecution)
{
    ExtendedInterpreter interpreter{formula_entry("if 1\n"), options()};

    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(1U, interpreter.diagnostics().size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::PARSE, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, missingReferenceBlocksExecution)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "global:\n"
            "Missing missing\n"
            "}\n"},
    };
    ExtendedInterpreterOptions interpreter_options{options()};
    interpreter_options.parser.source_filename = "main.ufm";
    interpreter_options.parser.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };

    ExtendedInterpreter interpreter{formula_entry("global:\nMissing missing"), interpreter_options};

    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(1U, interpreter.diagnostics().size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::REFERENCE, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::PER_IMAGE), std::runtime_error);
}

TEST(TestExtendedInterpreter, semanticFailureBlocksExecution)
{
    ExtendedInterpreter interpreter{formula_entry("int value\nvalue=\"text\""), options()};

    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(1U, interpreter.diagnostics().size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::SEMANTIC, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, emptyValidSectionRuns)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"), options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{}, interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, missingValidSectionRunsAsNoOp)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"), options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{}, interpreter.interpret(Section::BAILOUT));
}

TEST(TestExtendedInterpreter, coloringFinalSectionDispatches)
{
    ExtendedInterpreter interpreter{formula_entry("final:\n"), options(parser::EntryKind::COLORING)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{}, interpreter.interpret(Section::FINAL));
}

TEST(TestExtendedInterpreter, transformationTransformSectionDispatches)
{
    ExtendedInterpreter interpreter{formula_entry("transform:\n"), options(parser::EntryKind::TRANSFORMATION)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{}, interpreter.interpret(Section::TRANSFORM));
}

TEST(TestExtendedInterpreter, invalidSectionForEntryKindThrows)
{
    ExtendedInterpreter interpreter{formula_entry("final:\n"), options(parser::EntryKind::COLORING)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::BAILOUT), std::runtime_error);
}

TEST(TestExtendedInterpreter, classEntriesDoNotDispatchRuntimeSections)
{
    ExtendedInterpreter interpreter{formula_entry("public:\nint value\n"), options(parser::EntryKind::CLASS)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::PER_IMAGE), std::runtime_error);
}

TEST(TestExtendedInterpreter, nonEmptyValidSectionReportsUnsupportedExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\nsin(1)"), options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, setAndGetRuntimeValues)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"), options()};
    ASSERT_TRUE(interpreter.ok());

    interpreter.set_value("pixel", Value{Complex{1.0, 2.0}});

    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpreter.value("pixel"));
}

TEST(TestExtendedInterpreter, interpretsLiteralValues)
{
    EXPECT_EQ(Value{7}, interpret_init("7"));
    EXPECT_EQ(Value{1.5}, interpret_init("1.5"));
    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpret_init("(1,2)"));
    EXPECT_EQ(Value{true}, interpret_init("true"));
    EXPECT_EQ(Value{std::string{"text"}}, interpret_init("\"text\""));
}

TEST(TestExtendedInterpreter, interpretsPredefinedSymbolReferences)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n#pixel"), options()};
    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("#pixel", Value{Complex{1.0, 2.0}});

    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, interpretsParameterReferences)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@power\n"
                                                  "default:\n"
                                                  "float param power\n"
                                                  "endparam\n"),
        options()};
    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@power", Value{2.5});

    EXPECT_EQ(Value{2.5}, interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, interpretsArithmeticAndComparisonExpressions)
{
    EXPECT_EQ(Value{7}, interpret_init("1+2*3"));
    EXPECT_EQ(Value{1}, interpret_init("7%3"));
    EXPECT_EQ(Value{8.0}, interpret_init("2^3"));
    EXPECT_EQ(Value{true}, interpret_init("2<3"));
    EXPECT_EQ(Value{false}, interpret_init("2==3"));
    EXPECT_EQ((Value{Complex{4.0, 6.0}}), interpret_init("(1,2)+(3,4)"));
}

TEST(TestExtendedInterpreter, interpretsUnaryExpressions)
{
    EXPECT_EQ(Value{-3}, interpret_init("-3"));
    EXPECT_EQ(Value{3}, interpret_init("+3"));
    EXPECT_EQ(Value{false}, interpret_init("!true"));
    EXPECT_EQ(Value{25.0}, interpret_init("|(3,4)|"));
}

TEST(TestExtendedInterpreter, interpretsLogicalExpressions)
{
    EXPECT_EQ(Value{false}, interpret_init("true && false"));
    EXPECT_EQ(Value{true}, interpret_init("true || false"));
    // No explicit short-circuit side-effect test is possible until user functions can mutate state.
}

TEST(TestExtendedInterpreter, interpretsAssignmentToWritablePredefinedSymbol)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n#pixel=(1,2)"), options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpreter.value("#pixel"));
}

TEST(TestExtendedInterpreter, interpretsScalarDeclarations)
{
    EXPECT_EQ(Value{0}, interpret_init("int count"));
    EXPECT_EQ(Value{2.0}, interpret_init("float power=2"));
    EXPECT_EQ((Value{Complex{3.0, 0.0}}), interpret_init("complex seed=3"));
    EXPECT_EQ(Value{std::string{}}, interpret_init("string label"));
}

TEST(TestExtendedInterpreter, declaredScalarsPersistInFormulaScope)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int count=1\n"
                                                  "loop:\n"
                                                  "count=count+1\n"
                                                  "count"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{1}, interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ(Value{2}, interpreter.interpret(Section::ITERATE));
    EXPECT_EQ(Value{2}, interpreter.value("count"));
}

TEST(TestExtendedInterpreter, declarationInitializerConversionBackstopRejectsInvalidRuntimeValue)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "float power=@power\n"
                                                  "default:\n"
                                                  "float param power\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@power", Value{std::string{"bad"}});

    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, interpretsIfElseIfElseStatements)
{
    EXPECT_EQ(Value{2},
        interpret_init("int value=0\n"
                       "if false\n"
                       "value=1\n"
                       "elseif true\n"
                       "value=2\n"
                       "else\n"
                       "value=3\n"
                       "endif\n"
                       "value"));
}

TEST(TestExtendedInterpreter, interpretsWhileStatements)
{
    EXPECT_EQ(Value{3},
        interpret_init("int count=0\n"
                       "while count<3\n"
                       "count=count+1\n"
                       "endwhile\n"
                       "count"));
}

TEST(TestExtendedInterpreter, interpretsRepeatUntilStatements)
{
    EXPECT_EQ(Value{1},
        interpret_init("int count=0\n"
                       "repeat\n"
                       "count=count+1\n"
                       "until true\n"
                       "count"));
}

TEST(TestExtendedInterpreter, topLevelReturnStopsSectionExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int value=1\n"
                                                  "return 3\n"
                                                  "value=4"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{3}, interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ(Value{1}, interpreter.value("value"));
}

TEST(TestExtendedInterpreter, nestedStatementBlocksHaveLocalScope)
{
    EXPECT_EQ(Value{1},
        interpret_init("int value=1\n"
                       "if true\n"
                       "int value=2\n"
                       "endif\n"
                       "value"));
}

TEST(TestExtendedInterpreter, loopGuardRejectsRunawayLoops)
{
    ExtendedInterpreterOptions interpreter_options{options()};
    interpreter_options.max_loop_iterations = 2;
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "while true\n"
                                                  "endwhile"),
        interpreter_options};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

} // namespace formula::test
