// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedInterpreter.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <unordered_map>

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

TEST(TestExtendedInterpreter, nonEmptyValidSectionReportsUnsupportedExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\nz=1"), options()};

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

} // namespace formula::test
