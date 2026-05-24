// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedInterpreter.h>
#include <formula/Parser.h>

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

parameter::ParameterResolvedReference resolved_parameter_reference(std::string filename, std::string entry_name,
    std::string body, parser::EntryKind entry_kind, std::vector<parameter::Parameter> parameters)
{
    parser::Options parser_options;
    parser_options.dialect = Dialect::EXTENDED;
    parser_options.entry_kind = entry_kind;
    const parser::ParserPtr parser{parser::create_parser(body, parser_options)};
    ast::FormulaSectionsPtr ast{parser->parse()};
    EXPECT_TRUE(parser->get_errors().empty());
    EXPECT_TRUE(ast);

    parameter::ParameterReference reference;
    reference.filename = std::move(filename);
    reference.entry = std::move(entry_name);
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.parameters = std::move(parameters);

    FileEntry entry;
    entry.name = reference.entry;
    entry.body = std::move(body);
    return {std::move(reference), std::move(entry), std::move(ast), entry_kind};
}

void expect_unsupported(ExtendedInterpreter &interpreter, Section section, std::string_view node)
{
    ASSERT_TRUE(interpreter.ok());
    try
    {
        (void) interpreter.interpret(section);
        FAIL() << "expected unsupported runtime node";
    }
    catch (const std::runtime_error &err)
    {
        EXPECT_EQ(std::string{node} + " is not supported by the extended interpreter", err.what());
    }
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

TEST(TestExtendedInterpreter, exposesParameterMetadataAfterConstruction)
{
    ExtendedInterpreter interpreter{formula_entry("default:\n"
                                                  "float param power\n"
                                                  "default=2.0\n"
                                                  "endparam\n"
                                                  "Plugin param bailout\n"
                                                  "endparam\n"
                                                  "Image param source\n"
                                                  "endparam\n"),
        options()};

    ASSERT_FALSE(interpreter.ok());
    const std::vector<semantic::FormulaParameterInfo> &parameters{interpreter.parameters()};
    ASSERT_EQ(3U, parameters.size());
    EXPECT_EQ("float", parameters[0].type);
    EXPECT_EQ("power", parameters[0].name);
    EXPECT_TRUE(parameters[0].has_default);
    EXPECT_FALSE(parameters[0].is_plugin);
    EXPECT_EQ("Plugin", parameters[1].type);
    EXPECT_EQ("bailout", parameters[1].name);
    EXPECT_TRUE(parameters[1].is_plugin);
    EXPECT_EQ("Image", parameters[2].type);
    EXPECT_EQ("source", parameters[2].name);
    EXPECT_FALSE(parameters[2].is_plugin);
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

TEST(TestExtendedInterpreter, interpretsMathBuiltins)
{
    EXPECT_EQ((Value{Complex{4.0, 0.0}}), interpret_init("sqr(2)"));
    EXPECT_EQ((Value{Complex{3.0, 0.0}}), interpret_init("real((3,4))"));
    EXPECT_EQ(Value{std::atan2(4.0, 3.0)}, interpret_init("atan2((3,4))"));
    EXPECT_EQ(Value{true}, interpret_init("isInf(1.0/0.0)"));
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

TEST(TestExtendedInterpreter, initializesParametersFromDefaults)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@power\n"
                                                  "default:\n"
                                                  "float param power\n"
                                                  "default=2.5\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{2.5}, interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ(Value{2.5}, interpreter.value("@power"));
}

TEST(TestExtendedInterpreter, initializesOmittedParameterDefaults)
{
    ExtendedInterpreter interpreter{formula_entry("default:\n"
                                                  "complex param seed\n"
                                                  "endparam\n"
                                                  "bool param enabled\n"
                                                  "endparam\n"
                                                  "int param count\n"
                                                  "endparam\n"
                                                  "float param power\n"
                                                  "endparam\n"
                                                  "color param tint\n"
                                                  "endparam\n"
                                                  "string param label\n"
                                                  "endparam\n"
                                                  "Image param source\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_EQ((Value{Complex{0.0, 0.0}}), interpreter.value("@seed"));
    EXPECT_EQ(Value{false}, interpreter.value("@enabled"));
    EXPECT_EQ(Value{0}, interpreter.value("@count"));
    EXPECT_EQ(Value{0.0}, interpreter.value("@power"));
    EXPECT_EQ((Value{ColorValue{0.0, 0.0, 0.0, 0.0}}), interpreter.value("@tint"));
    EXPECT_EQ(Value{std::string{}}, interpreter.value("@label"));
    EXPECT_EQ(false, std::get<Value::ImagePtr>(interpreter.value("@source").storage()) == nullptr);
}

TEST(TestExtendedInterpreter, initializesFunctionParameterDefaults)
{
    ExtendedInterpreter interpreter{formula_entry("default:\n"
                                                  "complex func fn1\n"
                                                  "endfunc\n"
                                                  "color func merge\n"
                                                  "default=mergesoftlight()\n"
                                                  "endfunc\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_EQ(Value{std::string{"sin"}}, interpreter.value("@fn1"));
    EXPECT_EQ(Value{std::string{"mergesoftlight"}}, interpreter.value("@merge"));
}

TEST(TestExtendedInterpreter, dispatchesDefaultFunctionParameter)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "fn1((1,0))\n"
                                                  "default:\n"
                                                  "complex func fn1\n"
                                                  "endfunc\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ((Value{Complex{std::sin(1.0), 0.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, dispatchesHostFunctionParameter)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "fn1((2,0))\n"
                                                  "default:\n"
                                                  "complex func fn1\n"
                                                  "endfunc\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_function_parameter("fn1", "sqr");

    EXPECT_EQ((Value{Complex{4.0, 0.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, dispatchesUserFunctionParameter)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "fn1((2,0))\n"
                                                  "complex func twice(complex value)\n"
                                                  "return value+value\n"
                                                  "endfunc\n"
                                                  "default:\n"
                                                  "complex func fn1\n"
                                                  "endfunc\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_function_parameter("fn1", "twice");

    EXPECT_EQ((Value{Complex{4.0, 0.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, invalidFunctionParameterBindingBlocksExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "fn1((2,0))\n"
                                                  "default:\n"
                                                  "complex func fn1\n"
                                                  "endfunc\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_function_parameter("fn1", "missing");

    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(1U, interpreter.diagnostics().size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::BINDING, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, dispatchesNamedFunctionParameter)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@myfunc((2,0))\n"
                                                  "default:\n"
                                                  "complex func myfunc\n"
                                                  "default=sqr()\n"
                                                  "endfunc\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ((Value{Complex{4.0, 0.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, hostParameterOverridesDefaultValue)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@power\n"
                                                  "default:\n"
                                                  "float param power\n"
                                                  "default=2.5\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_parameter("power", Value{4.0});

    EXPECT_EQ(Value{4.0}, interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, initializesEnumParameterDefault)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@mode==\"Silver\"\n"
                                                  "default:\n"
                                                  "int param mode\n"
                                                  "enum=\"Golden\" \"Silver\"\n"
                                                  "default=1\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{true}, interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ(Value{1}, convert_value(interpreter.value("@mode"), ValueKind::INT));
}

TEST(TestExtendedInterpreter, bindsEnumParameterByLabelAndIndex)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@mode==\"Silver\"\n"
                                                  "default:\n"
                                                  "int param mode\n"
                                                  "enum=\"Golden\" \"Silver\"\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_parameter("mode", Value{std::string{"Silver"}});

    EXPECT_EQ(Value{true}, interpreter.interpret(Section::INITIALIZE));

    interpreter.set_parameter("mode", Value{0});

    EXPECT_EQ(Value{false}, interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, invalidEnumParameterBindingBlocksExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@mode\n"
                                                  "default:\n"
                                                  "int param mode\n"
                                                  "enum=\"Golden\" \"Silver\"\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_parameter("mode", Value{std::string{"Bronze"}});

    ASSERT_FALSE(interpreter.ok());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::BINDING, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, cleanParameterApisBindByRawName)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@source\n"
                                                  "default:\n"
                                                  "Image param source\n"
                                                  "endparam\n"
                                                  "complex func fn1\n"
                                                  "endfunc\n"),
        options()};
    ImageValue image;
    image.width = 1;
    image.height = 1;
    image.pixels.push_back(ColorValue{0.25, 0.5, 0.75, 1.0});

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_parameter("source", make_image_value(image));
    interpreter.set_function_parameter("fn1", "sin");
    interpreter.set_plugin_parameter("bailout", "Plugin.ulb:Default");
    interpreter.set_plugin_parameter_value("bailout", "power", Value{2.5});

    EXPECT_EQ(make_image_value(image), interpreter.value("@source"));
    EXPECT_EQ(Value{std::string{"sin"}}, interpreter.value("@fn1"));
    EXPECT_EQ(Value{std::string{"Plugin.ulb:Default"}}, interpreter.value("@bailout"));
    EXPECT_EQ(Value{2.5}, interpreter.value("@bailout.power"));
}

TEST(TestExtendedInterpreter, badParameterBindingBlocksExecutionUntilRebound)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@power\n"
                                                  "default:\n"
                                                  "float param power\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@power", Value{std::string{"bad"}});

    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(1U, interpreter.diagnostics().size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::BINDING, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);

    interpreter.set_value("@power", Value{3.5});

    ASSERT_TRUE(interpreter.ok());
    EXPECT_TRUE(interpreter.diagnostics().empty());
    EXPECT_EQ(Value{3.5}, interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, rebindingParameterLeavesUnrelatedBindingDiagnostics)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@power+@count\n"
                                                  "default:\n"
                                                  "float param power\n"
                                                  "endparam\n"
                                                  "int param count\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@power", Value{std::string{"bad"}});
    interpreter.set_value("@count", Value{std::string{"bad"}});
    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(2U, interpreter.diagnostics().size());

    interpreter.set_value("@power", Value{3.5});

    ASSERT_FALSE(interpreter.ok());
    ASSERT_EQ(1U, interpreter.diagnostics().size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::BINDING, interpreter.diagnostics()[0].kind);
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, preparesParameterSetInterpretersWithSavedValues)
{
    parameter::ParameterReferenceSet references;
    references.resolved.push_back(resolved_parameter_reference("formula.ufm", "Mandelbrot",
        "init:\n"
        "@power\n"
        "default:\n"
        "float param power\n"
        "endparam\n",
        parser::EntryKind::FRACTAL, {{"p_power", "2.5"}}));

    PreparedParameterSet prepared{prepare_parameter_interpreters(references, options())};

    ASSERT_TRUE(prepared.ok());
    ASSERT_EQ(1U, prepared.formulas.size());
    EXPECT_EQ("formula.ufm", prepared.formulas[0].filename);
    EXPECT_EQ("Mandelbrot", prepared.formulas[0].entry);
    EXPECT_EQ(Value{2.5}, prepared.formulas[0].interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, preparesLayerSpecificParameterSetInterpreters)
{
    parameter::ParameterReferenceSet references;
    parameter::ParameterResolvedReference first{resolved_parameter_reference("formula.ufm", "Mandelbrot",
        "init:\n"
        "@power\n"
        "default:\n"
        "float param power\n"
        "endparam\n",
        parser::EntryKind::FRACTAL, {{"p_power", "2.0"}})};
    first.reference.site.layer_index = 0;
    parameter::ParameterResolvedReference second{resolved_parameter_reference("formula.ufm", "Mandelbrot",
        "init:\n"
        "@power\n"
        "default:\n"
        "float param power\n"
        "endparam\n",
        parser::EntryKind::FRACTAL, {{"p_power", "3.0"}})};
    second.reference.site.layer_index = 1;
    references.resolved.push_back(std::move(first));
    references.resolved.push_back(std::move(second));

    PreparedParameterSet prepared{prepare_parameter_interpreters(references, options())};

    ASSERT_TRUE(prepared.ok());
    ASSERT_EQ(2U, prepared.formulas.size());
    EXPECT_EQ(0U, prepared.formulas[0].site.layer_index);
    EXPECT_EQ(Value{2.0}, prepared.formulas[0].interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ(1U, prepared.formulas[1].site.layer_index);
    EXPECT_EQ(Value{3.0}, prepared.formulas[1].interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, preparesImageFunctionAndPluginParameterBindings)
{
    parameter::ParameterReferenceSet references;
    references.resolved.push_back(resolved_parameter_reference("formula.ufm", "Mandelbrot",
        "init:\n"
        "1\n"
        "default:\n"
        "Image param source\n"
        "endparam\n"
        "complex func fn1\n"
        "endfunc\n",
        parser::EntryKind::FRACTAL, {{"p_source", "source.png"}, {"f_fn1", "sin"}, {"p_plugin.p_power", "2.0"}}));

    PreparedParameterSet prepared{prepare_parameter_interpreters(references, options())};

    ASSERT_TRUE(prepared.ok());
    ASSERT_EQ(1U, prepared.formulas.size());
    EXPECT_EQ(Value{std::string{"sin"}}, prepared.formulas[0].interpreter.value("@fn1"));
    EXPECT_EQ(Value{std::string{"2.0"}}, prepared.formulas[0].interpreter.value("@plugin.power"));
    EXPECT_EQ(false, std::get<Value::ImagePtr>(prepared.formulas[0].interpreter.value("@source").storage())->empty);
}

TEST(TestExtendedInterpreter, prepareParameterSetInterpretersBlocksDiagnostics)
{
    parameter::ParameterReferenceSet references;
    references.diagnostics.push_back(
        {parameter::ParameterReferenceErrorCode::UNRESOLVED_ENTRY, {}, "formula.ufm:Missing"});

    PreparedParameterSet prepared{prepare_parameter_interpreters(references, options())};

    EXPECT_FALSE(prepared.ok());
    EXPECT_TRUE(prepared.formulas.empty());
    ASSERT_EQ(1U, prepared.diagnostics.size());
    EXPECT_EQ(ExtendedInterpreterDiagnosticKind::REFERENCE, prepared.diagnostics[0].kind);
}

TEST(TestExtendedInterpreter, initializesPredefinedSymbols)
{
    EXPECT_EQ(Value{std::atan2(0.0, -1.0)}, interpret_init("#pi"));
    EXPECT_EQ(Value{std::exp(1.0)}, interpret_init("#e"));
    EXPECT_EQ(Value{2147483648.0}, interpret_init("#randomrange"));
    EXPECT_EQ((Value{Complex{0.0, 0.0}}), interpret_init("#z"));
}

TEST(TestExtendedInterpreter, hostPredefinedValueOverridesDefaultValue)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n#pixel"), options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("#pixel", Value{Complex{1.0, 2.0}});

    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpreter.interpret(Section::INITIALIZE));
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

TEST(TestExtendedInterpreter, readOnlyPredefinedSymbolAssignmentIsRejected)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n#pi=4"), options()};

    ASSERT_FALSE(interpreter.ok());
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, transformationWritesPixelAndSolid)
{
    ExtendedInterpreter interpreter{formula_entry("transform:\n"
                                                  "#pixel=(1,2)\n"
                                                  "#solid=true"),
        options(parser::EntryKind::TRANSFORMATION)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{}, interpreter.interpret(Section::TRANSFORM));
    EXPECT_EQ((Value{Complex{1.0, 2.0}}), interpreter.value("#pixel"));
    EXPECT_EQ(Value{true}, interpreter.value("#solid"));
}

TEST(TestExtendedInterpreter, coloringWritesIndexAndSolid)
{
    ExtendedInterpreter interpreter{formula_entry("final:\n"
                                                  "#index=0.5\n"
                                                  "#solid=true\n"
                                                  "#solid"),
        options(parser::EntryKind::COLORING)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{true}, interpreter.interpret(Section::FINAL));
    EXPECT_EQ(Value{0.5}, interpreter.value("#index"));
    EXPECT_EQ(Value{true}, interpreter.value("#solid"));
}

TEST(TestExtendedInterpreter, coloringWritesDirectColor)
{
    ExtendedInterpreter interpreter{formula_entry("final:\n"
                                                  "#color=rgb(1,0,0)\n"
                                                  "#color"),
        options(parser::EntryKind::COLORING)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ((Value{ColorValue{1.0, 0.0, 0.0, 1.0}}), interpreter.interpret(Section::FINAL));
    EXPECT_EQ((Value{ColorValue{1.0, 0.0, 0.0, 1.0}}), interpreter.value("#color"));
}

TEST(TestExtendedInterpreter, bailoutSectionReturnsTruthiness)
{
    ExtendedInterpreter false_interpreter{formula_entry("bailout:\n0"), options()};
    ExtendedInterpreter true_interpreter{formula_entry("bailout:\n(0,1)"), options()};

    ASSERT_TRUE(false_interpreter.ok());
    ASSERT_TRUE(true_interpreter.ok());

    EXPECT_EQ(Value{false}, false_interpreter.interpret(Section::BAILOUT));
    EXPECT_EQ(Value{true}, true_interpreter.interpret(Section::BAILOUT));
}

TEST(TestExtendedInterpreter, finalSectionReturnsNumericValue)
{
    ExtendedInterpreter interpreter{formula_entry("final:\n0.25"), options(parser::EntryKind::COLORING)};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{0.25}, interpreter.interpret(Section::FINAL));
}

TEST(TestExtendedInterpreter, finalSectionRejectsInvalidRuntimeResult)
{
    ExtendedInterpreter interpreter{formula_entry("final:\n"
                                                  "@value\n"
                                                  "default:\n"
                                                  "float param value\n"
                                                  "endparam\n"),
        options(parser::EntryKind::COLORING)};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@value", Value{std::string{"bad"}});

    EXPECT_THROW(interpreter.interpret(Section::FINAL), std::runtime_error);
}

TEST(TestExtendedInterpreter, bailoutSectionRejectsInvalidRuntimeResult)
{
    ExtendedInterpreter interpreter{formula_entry("bailout:\n"
                                                  "@value\n"
                                                  "default:\n"
                                                  "bool param value\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@value", Value{std::string{"bad"}});

    EXPECT_THROW(interpreter.interpret(Section::BAILOUT), std::runtime_error);
}

TEST(TestExtendedInterpreter, unsupportedObjectConstructionFailsClearly)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture texture\n"
            "init:\n"
            "new Texture()\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };
    ExtendedInterpreterOptions interpreter_options{options()};
    interpreter_options.parser.source_filename = "main.ufm";
    interpreter_options.parser.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };
    ExtendedInterpreter interpreter{formula_entry("import \"common.ulb\"\n"
                                                  "global:\n"
                                                  "Texture texture\n"
                                                  "init:\n"
                                                  "new Texture()\n"),
        interpreter_options};

    expect_unsupported(interpreter, Section::INITIALIZE, "NewNode");
}

TEST(TestExtendedInterpreter, unsupportedObjectMethodCallFailsClearly)
{
    std::unordered_map<std::string, std::string> files{
        {"main.ufm",
            "Formula {\n"
            "import \"common.ulb\"\n"
            "global:\n"
            "Texture texture\n"
            "init:\n"
            "texture.getValue()\n"
            "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int func getValue()\n"
            "return 1\n"
            "endfunc\n"
            "}\n"},
    };
    ExtendedInterpreterOptions interpreter_options{options()};
    interpreter_options.parser.source_filename = "main.ufm";
    interpreter_options.parser.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };
    ExtendedInterpreter interpreter{formula_entry("import \"common.ulb\"\n"
                                                  "global:\n"
                                                  "Texture texture\n"
                                                  "init:\n"
                                                  "texture.getValue()\n"),
        interpreter_options};

    expect_unsupported(interpreter, Section::INITIALIZE, "FunctionCallNode");
}

TEST(TestExtendedInterpreter, imageParametersDefaultToEmptyImages)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@image.getEmpty()\n"
                                                  "default:\n"
                                                  "Image param image\n"
                                                  "endparam\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_EQ(Value{true}, interpreter.interpret(Section::INITIALIZE));
    EXPECT_EQ(Value{0}, interpret_init("Image image\nimage.getWidth()"));
}

TEST(TestExtendedInterpreter, imageMethodsReadHostBoundImageData)
{
    ImageValue image;
    image.name = "source";
    image.empty = false;
    image.width = 2;
    image.height = 1;
    image.pixels = {ColorValue{1.0, 0.0, 0.0, 1.0}, ColorValue{0.0, 1.0, 0.0, 1.0}};

    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "Image image=new @source\n"
                                                  "image.getPixel(1,0)\n"
                                                  "default:\n"
                                                  "Image param source\n"
                                                  "endparam\n"),
        options()};
    ASSERT_TRUE(interpreter.ok());
    interpreter.set_value("@source", make_image_value(image));

    EXPECT_EQ((Value{ColorValue{0.0, 1.0, 0.0, 1.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, imageMethodsMutateStandaloneImageData)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "Image image=new Image()\n"
                                                  "image.resize(2,2)\n"
                                                  "image.setPixel(1,1,rgba(0.25,0.5,0.75,1))\n"
                                                  "image.getPixel(1,1)\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_EQ((Value{ColorValue{0.25, 0.5, 0.75, 1.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, imageAssignCopiesImageHandles)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "Image source=new Image()\n"
                                                  "source.resize(1,1)\n"
                                                  "source.setPixel(0,0,rgba(1,0,0,1))\n"
                                                  "Image target=new Image()\n"
                                                  "target.assign(source)\n"
                                                  "target.getPixel(0,0)\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_EQ((Value{ColorValue{1.0, 0.0, 0.0, 1.0}}), interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, imageMethodRuntimeBackstopsRejectBadCalls)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "Image image=new Image()\n"
                                                  "image.resize(-1,1)\n"),
        options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, unsupportedObjectFieldAccessFailsClearly)
{
    semantic::BuiltinRegistry registry{semantic::default_builtin_registry()};
    const semantic::SemanticType *bool_type{registry.find_type("bool")};
    ASSERT_TRUE(bool_type);
    for (semantic::SemanticClassDescriptor &klass : registry.classes)
    {
        if (klass.name == "Image")
        {
            klass.fields.push_back({semantic::SemanticSymbolKind::FIELD, "empty", *bool_type, {}});
        }
    }
    ExtendedInterpreterOptions interpreter_options{options()};
    interpreter_options.builtins = &registry;
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "@image.empty\n"
                                                  "default:\n"
                                                  "Image param image\n"
                                                  "endparam\n"),
        interpreter_options};

    expect_unsupported(interpreter, Section::INITIALIZE, "MemberAccessNode");
}

TEST(TestExtendedInterpreter, unsupportedClassCastFailsClearly)
{
    const std::string body{"import \"common.ulb\"\n"
                           "global:\n"
                           "Texture texture\n"
                           "init:\n"
                           "Texture(@image)\n"
                           "default:\n"
                           "Image param image\n"
                           "endparam\n"};
    std::unordered_map<std::string, std::string> files{
        {"main.ufm", "Formula {\n" + body + "}\n"},
        {"common.ulb",
            "class Texture {\n"
            "public:\n"
            "int value\n"
            "}\n"},
    };
    ExtendedInterpreterOptions interpreter_options{options()};
    interpreter_options.parser.source_filename = "main.ufm";
    interpreter_options.parser.file_importer = [&files](std::string_view filename)
    {
        return files.at(std::string{filename});
    };
    ExtendedInterpreter interpreter{formula_entry(body), interpreter_options};

    expect_unsupported(interpreter, Section::INITIALIZE, "FunctionCallNode");
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

TEST(TestExtendedInterpreter, interpretsForwardFunctionCalls)
{
    EXPECT_EQ(Value{3},
        interpret_init("add(1,2)\n"
                       "int func add(int left, int right)\n"
                       "return left+right\n"
                       "endfunc"));
}

TEST(TestExtendedInterpreter, interpretsRecursiveFunctionCalls)
{
    EXPECT_EQ(Value{24},
        interpret_init("factorial(4)\n"
                       "int func factorial(int value)\n"
                       "if value<=1\n"
                       "return 1\n"
                       "endif\n"
                       "return value*factorial(value-1)\n"
                       "endfunc"));
}

TEST(TestExtendedInterpreter, functionLocalsDoNotLeak)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int value=1\n"
                                                  "helper()\n"
                                                  "value\n"
                                                  "func helper()\n"
                                                  "int value=2\n"
                                                  "endfunc"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_EQ(Value{1}, interpreter.interpret(Section::INITIALIZE));
}

TEST(TestExtendedInterpreter, convertsFunctionReturnValue)
{
    EXPECT_EQ(Value{2.0},
        interpret_init("float func helper()\n"
                       "return 2\n"
                       "endfunc\n"
                       "helper()"));
}

TEST(TestExtendedInterpreter, functionByRefArgumentsMutateCaller)
{
    EXPECT_EQ(Value{2},
        interpret_init("int value=1\n"
                       "increment(value)\n"
                       "value\n"
                       "func increment(int &target)\n"
                       "target=target+1\n"
                       "endfunc"));
}

TEST(TestExtendedInterpreter, functionConstArgumentsAreReadOnly)
{
    EXPECT_EQ(Value{3},
        interpret_init("int func helper(const int value)\n"
                       "return value\n"
                       "endfunc\n"
                       "helper(3)"));
}

TEST(TestExtendedInterpreter, staticArrayElementsReadAndWrite)
{
    EXPECT_EQ(Value{7},
        interpret_init("int values[2]\n"
                       "values[1]=7\n"
                       "values[1]"));
}

TEST(TestExtendedInterpreter, staticArrayIndexesFlattenMultipleDimensions)
{
    EXPECT_EQ(Value{5},
        interpret_init("int values[2,3]\n"
                       "values[1,2]=5\n"
                       "values[1,2]"));
}

TEST(TestExtendedInterpreter, staticArrayBoundsAreChecked)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int values[1]\n"
                                                  "values[1]"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, staticArrayElementAssignmentConvertsValue)
{
    EXPECT_EQ((Value{Complex{2.0, 0.0}}),
        interpret_init("complex values[1]\n"
                       "values[0]=2\n"
                       "values[0]"));
}

TEST(TestExtendedInterpreter, wholeStaticArrayAssignmentCopiesMatchingArrays)
{
    EXPECT_EQ(Value{4},
        interpret_init("int source[2]\n"
                       "int target[2]\n"
                       "source[1]=4\n"
                       "target=source\n"
                       "target[1]"));
}

TEST(TestExtendedInterpreter, wholeStaticArrayAssignmentRejectsMismatchedArrays)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int source[2]\n"
                                                  "int target[3]\n"
                                                  "target=source"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, dynamicArraysStartEmpty)
{
    EXPECT_EQ(Value{0},
        interpret_init("int values[]\n"
                       "length(values)"));
}

TEST(TestExtendedInterpreter, interpretsColorConstructionAndExtraction)
{
    EXPECT_EQ((Value{ColorValue{0.1, 0.2, 0.3, 1.0}}), interpret_init("rgb(0.1,0.2,0.3)"));
    EXPECT_EQ((Value{ColorValue{0.1, 0.2, 0.3, 0.4}}), interpret_init("rgba(0.1,0.2,0.3,0.4)"));
    EXPECT_EQ(Value{0.2}, interpret_init("green(rgba(0.1,0.2,0.3,0.4))"));
    EXPECT_EQ(Value{0.4}, interpret_init("alpha(rgba(0.1,0.2,0.3,0.4))"));
}

TEST(TestExtendedInterpreter, interpretsHslColorConstructionAndExtraction)
{
    EXPECT_EQ((Value{ColorValue{1.0, 0.0, 0.0, 1.0}}), interpret_init("hsl(0,1,0.5)"));
    EXPECT_EQ((Value{ColorValue{0.0, 1.0, 0.0, 0.25}}), interpret_init("hsla(2,1,0.5,0.25)"));
    EXPECT_EQ(Value{2.0}, interpret_init("hue(hsl(2,1,0.5))"));
    EXPECT_EQ(Value{1.0}, interpret_init("sat(hsl(2,1,0.5))"));
    EXPECT_EQ(Value{0.5}, interpret_init("lum(hsl(2,1,0.5))"));
}

TEST(TestExtendedInterpreter, randomFunctionIsSeedDeterministic)
{
    const Value first{interpret_init("random(1234)")};

    EXPECT_EQ(first, interpret_init("random(1234)"));
    EXPECT_NE(first, interpret_init("random(random(1234))"));
}

TEST(TestExtendedInterpreter, printRecordsRuntimeMessages)
{
    ExtendedInterpreter interpreter{formula_entry("init:\nprint(\"value=\", 7, \" color=\", rgb(1,0,0))"), options()};

    ASSERT_TRUE(interpreter.ok());
    EXPECT_EQ(Value{}, interpreter.interpret(Section::INITIALIZE));
    ASSERT_EQ(1U, interpreter.messages().size());
    EXPECT_EQ("value=7 color=rgba(1,0,0,1)", interpreter.messages().front());
}

TEST(TestExtendedInterpreter, builtinArityErrorsBlockExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\nrgb(1,2)"), options()};

    ASSERT_FALSE(interpreter.ok());
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, builtinArgumentTypeErrorsBlockExecution)
{
    ExtendedInterpreter interpreter{formula_entry("init:\nred(1)"), options()};

    ASSERT_FALSE(interpreter.ok());
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, dynamicArraySetLengthResizesUpAndDown)
{
    EXPECT_EQ(Value{2},
        interpret_init("int values[]\n"
                       "setLength(values, 3)\n"
                       "values[2]=7\n"
                       "setLength(values, 2)\n"
                       "length(values)"));
}

TEST(TestExtendedInterpreter, dynamicArrayNewElementsUseDefaultValues)
{
    EXPECT_EQ(Value{0},
        interpret_init("int values[]\n"
                       "setLength(values, 2)\n"
                       "values[1]"));
}

TEST(TestExtendedInterpreter, dynamicArrayBuiltinsRejectScalarArgument)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int value=0\n"
                                                  "setLength(value, 1)"),
        options()};

    ASSERT_FALSE(interpreter.ok());
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, dynamicArrayBuiltinsRejectStaticArrayArgument)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int values[1]\n"
                                                  "length(values)"),
        options()};

    ASSERT_FALSE(interpreter.ok());
    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

TEST(TestExtendedInterpreter, wholeDynamicArrayAssignmentIsRejected)
{
    ExtendedInterpreter interpreter{formula_entry("init:\n"
                                                  "int source[]\n"
                                                  "int target[]\n"
                                                  "setLength(source, 1)\n"
                                                  "setLength(target, 1)\n"
                                                  "target=source"),
        options()};

    ASSERT_TRUE(interpreter.ok());

    EXPECT_THROW(interpreter.interpret(Section::INITIALIZE), std::runtime_error);
}

} // namespace formula::test
