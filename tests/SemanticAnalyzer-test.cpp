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

} // namespace formula::test
