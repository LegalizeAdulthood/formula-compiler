// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/SemanticAnalyzer.h>

#include <formula/Formula.h>
#include <formula/ParseOptions.h>

#include "NodeFormatter.h"
#include "trim_ws.h"

#include <gtest/gtest.h>

using namespace formula::semantic;

namespace formula::test
{

TEST(TestSemanticAnalyzer, emptyFormulaReturnsNoDiagnostics)
{
    const ast::FormulaSections formula;
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(formula, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, emptyParameterSetReturnsNoDiagnostics)
{
    const parameter::ExtendedParameterEntry parameters;
    const parameter::ParameterReferenceSet references;
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisDoesNotMutateParsedFormula)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("z=1", options)};
    ASSERT_TRUE(loaded.ast);
    ASSERT_TRUE(loaded.ast->bailout);
    const std::string before{trim_ws(to_string(loaded.ast->bailout))};
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
    EXPECT_EQ(before, trim_ws(to_string(loaded.ast->bailout)));
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisDoesNotMutateParsedParameters)
{
    parameter::ExtendedParameterEntry parameters;
    parameters.fractal.name = "fractal";
    parameters.fractal.assignments.push_back({"formula", "Example.ufm#Mandelbrot"});
    parameter::ParameterReferenceSet references;
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
    ASSERT_EQ(1U, parameters.fractal.assignments.size());
    EXPECT_EQ("formula", parameters.fractal.assignments.front().key);
    EXPECT_EQ("Example.ufm#Mandelbrot", parameters.fractal.assignments.front().value);
}

TEST(TestSemanticAnalyzer, defaultRegistryFindsScalarTypes)
{
    const BuiltinRegistry &registry{default_builtin_registry()};

    ASSERT_TRUE(registry.find_type("void"));
    EXPECT_EQ(SemanticTypeKind::VOID, registry.find_type("void")->kind);
    ASSERT_TRUE(registry.find_type("bool"));
    EXPECT_EQ(SemanticTypeKind::BOOL, registry.find_type("bool")->kind);
    ASSERT_TRUE(registry.find_type("int"));
    EXPECT_EQ(SemanticTypeKind::INT, registry.find_type("int")->kind);
    ASSERT_TRUE(registry.find_type("float"));
    EXPECT_EQ(SemanticTypeKind::FLOAT, registry.find_type("float")->kind);
    ASSERT_TRUE(registry.find_type("complex"));
    EXPECT_EQ(SemanticTypeKind::COMPLEX, registry.find_type("complex")->kind);
    ASSERT_TRUE(registry.find_type("color"));
    EXPECT_EQ(SemanticTypeKind::COLOR, registry.find_type("color")->kind);
    ASSERT_TRUE(registry.find_type("string"));
    EXPECT_EQ(SemanticTypeKind::STRING, registry.find_type("string")->kind);
}

TEST(TestSemanticAnalyzer, defaultRegistryFindsBuiltinFunctionDescriptors)
{
    const BuiltinRegistry &registry{default_builtin_registry()};

    const SemanticFunctionDescriptor *function{registry.find_function("sin")};

    ASSERT_TRUE(function);
    EXPECT_EQ("sin", function->name);
    EXPECT_EQ(SemanticTypeKind::COMPLEX, function->return_type.kind);
    ASSERT_EQ(1U, function->argument_types.size());
    EXPECT_EQ(SemanticTypeKind::COMPLEX, function->argument_types.front().kind);
}

TEST(TestSemanticAnalyzer, defaultRegistryFindsImageAsBuiltinClass)
{
    const BuiltinRegistry &registry{default_builtin_registry()};

    const SemanticClassDescriptor *image{registry.find_class("Image")};

    ASSERT_TRUE(image);
    EXPECT_EQ("Image", image->name);
    EXPECT_TRUE(image->builtin);
    EXPECT_EQ(SemanticTypeKind::BUILTIN_OBJECT, image->type.kind);
    EXPECT_TRUE(registry.find_type("Image"));
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsDuplicateLocalDeclarations)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int value\nint value", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::DUPLICATE_SYMBOL, diagnostics.front().code);
    EXPECT_EQ("duplicate symbol: value", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsDuplicateFunctions)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper()\n"
                                            "endfunc\n"
                                            "func helper()\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::DUPLICATE_SYMBOL, diagnostics.front().code);
    EXPECT_EQ("duplicate symbol: helper", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsDuplicateFunctionArguments)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(int value, int value)\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::DUPLICATE_SYMBOL, diagnostics.front().code);
    EXPECT_EQ("duplicate symbol: value", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownDeclarationType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Bogus value", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_TYPE, diagnostics.front().code);
    EXPECT_EQ("unknown type: Bogus", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownFunctionArgumentType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(Bogus value)\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_TYPE, diagnostics.front().code);
    EXPECT_EQ("unknown type: Bogus", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsBuiltinImageType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image source", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownVariable)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int value\nvalue=missing", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_SYMBOL, diagnostics.front().code);
    EXPECT_EQ("unknown symbol: missing", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsDeclaredVariableAndBuiltinVariable)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("complex value\nvalue=z", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownFunction)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("missing()", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_SYMBOL, diagnostics.front().code);
    EXPECT_EQ("unknown symbol: missing", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsForwardUserFunctionCall)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("helper()\n"
                                            "func helper()\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownParameterAndConstantRefs)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("@param + #constant", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(2U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_SYMBOL, diagnostics[0].code);
    EXPECT_EQ("unknown symbol: param", diagnostics[0].message);
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_SYMBOL, diagnostics[1].code);
    EXPECT_EQ("unknown symbol: constant", diagnostics[1].message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUserFunctionCallArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(int value)\n"
                                            "endfunc\n"
                                            "helper()",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_CALL_ARITY, diagnostics.front().code);
    EXPECT_EQ("invalid call arity: helper expects 1 argument, got 0", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsUserFunctionCallArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(int value)\n"
                                            "endfunc\n"
                                            "helper(1)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsBuiltinFunctionCallArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("custom(1, 2)", options)};
    ASSERT_TRUE(loaded.ast);
    BuiltinRegistry registry;
    registry.types = default_builtin_registry().types;
    const SemanticType *complex_type{registry.find_type("complex")};
    ASSERT_TRUE(complex_type);
    SemanticFunctionDescriptor function;
    function.name = "custom";
    function.return_type = *complex_type;
    function.argument_types.push_back(*complex_type);
    registry.functions.push_back(function);
    FormulaSemanticContext context;
    context.builtins = &registry;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_CALL_ARITY, diagnostics.front().code);
    EXPECT_EQ("invalid call arity: custom expects 1 argument, got 2", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidAssignmentConversion)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int value\n"
                                            "value=\"text\"",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_TYPE_CONVERSION, diagnostics.front().code);
    EXPECT_EQ("invalid conversion: string to int", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAllowsNumericAssignmentPromotion)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("complex value\n"
                                            "value=1",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidDeclarationInitializerConversion)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("color value=\"text\"", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_TYPE_CONVERSION, diagnostics.front().code);
    EXPECT_EQ("invalid conversion: string to color", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidIfConditionType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("if \"text\"\n"
                                            "endif",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid condition type: string", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAllowsNumericIfConditionType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("if 1\n"
                                            "endif",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidArrayIndexType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int value[\"text\"]", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARRAY_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid array index type: string", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidReturnConversion)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int func helper()\n"
                                            "return \"text\"\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_RETURN, diagnostics.front().code);
    EXPECT_EQ("invalid return: string to int", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsReturnOutsideFunction)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("return 1", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_RETURN, diagnostics.front().code);
    EXPECT_EQ("invalid return outside function", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUserFunctionArgumentConversion)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(int value)\n"
                                            "endfunc\n"
                                            "helper(\"text\")",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid argument type: helper got string, expected int", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidByRefArgument)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(int &value)\n"
                                            "endfunc\n"
                                            "helper(1)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid by-ref argument: helper", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsConstArgumentAssignment)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper(const int value)\n"
                                            "value=1\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ASSIGNMENT_TARGET, diagnostics.front().code);
    EXPECT_EQ("invalid assignment target: value is const", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsRetainedClassTypes)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture\n"
                                            "texture=new Texture()",
        options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsScalarNewType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int value\n"
                                            "value=new int()",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_BUILTIN_USAGE, diagnostics.front().code);
    EXPECT_EQ("invalid new type: int", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsDynamicArrayBuiltins)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int values[]\n"
                                            "setLength(values, 3)\n"
                                            "int size=length(values)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsSetLengthStaticArrayArgument)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int values[3]\n"
                                            "setLength(values, 3)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid dynamic array argument: setLength", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsLengthScalarArgument)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int value\n"
                                            "int size=length(value)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid dynamic array argument: length", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsSetLengthSizeType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int values[]\n"
                                            "setLength(values, \"text\")",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid argument type: setLength got string, expected int", diagnostics.front().message);
}

} // namespace formula::test
