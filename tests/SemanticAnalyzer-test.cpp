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

#include <array>

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

TEST(TestSemanticAnalyzer, collectFormulaParametersReportsDefaultsAndPlugins)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "float param power\n"
                                            "default=2.0\n"
                                            "endparam\n"
                                            "Plugin param bailout\n"
                                            "endparam\n"
                                            "Image param source\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<FormulaParameterInfo> parameters{collect_formula_parameters(*loaded.ast, context)};

    ASSERT_EQ(3U, parameters.size());
    EXPECT_EQ("float", parameters[0].type);
    EXPECT_EQ("power", parameters[0].name);
    EXPECT_TRUE(parameters[0].has_default);
    EXPECT_FALSE(parameters[0].is_plugin);
    EXPECT_EQ("Plugin", parameters[1].type);
    EXPECT_EQ("bailout", parameters[1].name);
    EXPECT_FALSE(parameters[1].has_default);
    EXPECT_TRUE(parameters[1].is_plugin);
    EXPECT_EQ("Image", parameters[2].type);
    EXPECT_EQ("source", parameters[2].name);
    EXPECT_FALSE(parameters[2].is_plugin);
}

TEST(TestSemanticAnalyzer, collectFormulaParametersReportsRetainedClassPlugins)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Trap param trap\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass trap;
    trap.reference.class_name = "Trap";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&trap);

    const std::vector<FormulaParameterInfo> parameters{collect_formula_parameters(*loaded.ast, context)};

    ASSERT_EQ(1U, parameters.size());
    EXPECT_EQ("Trap", parameters[0].type);
    EXPECT_EQ("trap", parameters[0].name);
    EXPECT_TRUE(parameters[0].is_plugin);
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

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsResolvedReferences)
{
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.references.push_back(reference);
    references.resolved.push_back(
        {reference, {}, std::make_shared<ast::FormulaSections>(), parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsFractalKindMismatch)
{
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Smooth";
    references.resolved.push_back(
        {reference, {}, std::make_shared<ast::FormulaSections>(), parser::EntryKind::COLORING});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_FORMULA_KIND, diagnostics.front().code);
    EXPECT_EQ("Smooth", diagnostics.front().entry_name);
    EXPECT_EQ("invalid formula kind: Example.ufm#Smooth is coloring, expected fractal", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsColoringKindMismatch)
{
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::INSIDE_COLORING;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back(
        {reference, {}, std::make_shared<ast::FormulaSections>(), parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_FORMULA_KIND, diagnostics.front().code);
    EXPECT_EQ("Mandelbrot", diagnostics.front().entry_name);
    EXPECT_EQ(
        "invalid formula kind: Example.ufm#Mandelbrot is fractal, expected coloring", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsTransformKindMismatch)
{
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::TRANSFORM;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back(
        {reference, {}, std::make_shared<ast::FormulaSections>(), parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_FORMULA_KIND, diagnostics.front().code);
    EXPECT_EQ("Mandelbrot", diagnostics.front().entry_name);
    EXPECT_EQ("invalid formula kind: Example.ufm#Mandelbrot is fractal, expected transformation",
        diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsMissingReferencedEntry)
{
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    references.diagnostics.push_back(
        {parameter::ParameterReferenceErrorCode::UNRESOLVED_ENTRY, {}, "Example.ufm:Missing"});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INCOMPLETE_REFERENCE_GRAPH, diagnostics.front().code);
    EXPECT_EQ("Example.ufm:Missing", diagnostics.front().entry_name);
    EXPECT_EQ("incomplete reference graph: missing referenced entry Example.ufm:Missing", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsMissingRetainedClass)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INCOMPLETE_REFERENCE_GRAPH, diagnostics.front().code);
    EXPECT_EQ("Mandelbrot", diagnostics.front().entry_name);
    EXPECT_EQ("incomplete reference graph: missing retained class Texture", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsRetainedClassReference)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsRetainedClassWithoutAst)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INCOMPLETE_REFERENCE_GRAPH, diagnostics.front().code);
    EXPECT_EQ("Mandelbrot", diagnostics.front().entry_name);
    EXPECT_EQ("incomplete reference graph: missing retained class Texture", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsNestedMissingRetainedClass)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    const LoadedFormula loaded_class{load_formula("Filter filter", options)};
    ASSERT_TRUE(loaded_class.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INCOMPLETE_REFERENCE_GRAPH, diagnostics.front().code);
    EXPECT_EQ("Texture", diagnostics.front().entry_name);
    EXPECT_EQ("incomplete reference graph: missing retained class Filter", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsUnknownRetainedBaseClass)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.base_class = "Filter";
    retained.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_TYPE, diagnostics.front().code);
    EXPECT_EQ("Texture", diagnostics.front().entry_name);
    EXPECT_EQ("unknown base class: Filter", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsRetainedClassInheritanceCycle)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass texture;
    texture.reference.class_name = "Texture";
    texture.base_class = "Filter";
    texture.ast = std::make_shared<ast::FormulaSections>();
    RetainedFormulaClass filter;
    filter.reference.class_name = "Filter";
    filter.base_class = "Texture";
    filter.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&texture);
    context.retained_classes.push_back(&filter);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INHERITANCE_CYCLE, diagnostics.front().code);
    EXPECT_EQ("Texture", diagnostics.front().entry_name);
    EXPECT_EQ("inheritance cycle: Texture", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsUnknownSavedParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_missing", "2"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("Mandelbrot", diagnostics.front().entry_name);
    EXPECT_EQ("invalid parameter binding: p_missing", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsUnknownSavedFunction)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "complex func fn1\n"
                                            "endfunc\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"f_missing", "sin"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("Mandelbrot", diagnostics.front().entry_name);
    EXPECT_EQ("invalid parameter binding: f_missing", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsSavedParameterAndFunction)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n"
                                            "complex func fn1\n"
                                            "endfunc\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_power", "2"});
    reference.parameters.push_back({"f_fn1", "sin"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsInvalidSavedFunctionTarget)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "complex func fn1\n"
                                            "endfunc\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"f_fn1", "missing"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: f_fn1 invalid function target", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsSavedFunctionShapeMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "complex func fn1\n"
                                            "endfunc\n"
                                            "color func merge\n"
                                            "endfunc\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"f_fn1", "mergenormal"});
    reference.parameters.push_back({"f_merge", "sin"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(2U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics[0].code);
    EXPECT_EQ("invalid parameter binding: f_fn1 invalid function target", diagnostics[0].message);
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics[1].code);
    EXPECT_EQ("invalid parameter binding: f_merge invalid function target", diagnostics[1].message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsColorFunctionTarget)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "color func merge\n"
                                            "endfunc\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"f_merge", "mergenormal"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsSavedParameterTypeMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_power", "not-a-number"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_power type mismatch", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsSavedEnumMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "int param mode\n"
                                            "enum=\"Golden\" \"Silver\"\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_mode", "Bronze"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_mode type mismatch", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsSavedEnumIndexMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "int param mode\n"
                                            "enum=\"Golden\" \"Silver\"\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_mode", "2"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_mode type mismatch", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsSavedParameterTypes)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "bool param enabled\n"
                                            "endparam\n"
                                            "int param count\n"
                                            "endparam\n"
                                            "float param power\n"
                                            "endparam\n"
                                            "complex param seed\n"
                                            "endparam\n"
                                            "color param tint\n"
                                            "endparam\n"
                                            "int param mode\n"
                                            "enum=\"Golden\" \"Silver\"\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_enabled", "yes"});
    reference.parameters.push_back({"p_count", "3"});
    reference.parameters.push_back({"p_power", "2.5"});
    reference.parameters.push_back({"p_seed", "1.0/2.0"});
    reference.parameters.push_back({"p_tint", "4294967295"});
    reference.parameters.push_back({"p_mode", "1"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsOmittedScalarParameterDefaults)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "bool param enabled\n"
                                            "endparam\n"
                                            "int param count\n"
                                            "endparam\n"
                                            "float param power\n"
                                            "endparam\n"
                                            "complex param seed\n"
                                            "endparam\n"
                                            "color param tint\n"
                                            "endparam\n"
                                            "param origin\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsDefaultedParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "float param power\n"
                                            "default=2.0\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsDefaultedImageParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Image param source\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::OUTSIDE_COLORING;
    reference.filename = "Example.ucl";
    reference.entry = "Image";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::COLORING});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsOmittedFunctionParameterDefault)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "complex func fn1\n"
                                            "endfunc\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsForwardedRequiredParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "param oldpower = power\n"
                                            "float param power\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_oldpower", "2.0"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsInvalidParameterForward)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "param oldpower = missing\n"
                                            "float param power\n"
                                            "default=2.0\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_oldpower", "2.0"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_oldpower invalid forward", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsNestedPluginParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula plugin{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n",
        class_options)};
    ASSERT_TRUE(plugin.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass", "plugin.ulb:Plugin"});
    reference.parameters.push_back({"p_formulaClass.p_power", "2.0"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.filename = "plugin.ulb";
    retained.reference.class_name = "Plugin";
    retained.ast = plugin.ast;
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsNestedPluginParameterMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula plugin{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n",
        class_options)};
    ASSERT_TRUE(plugin.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass", "plugin.ulb:Plugin"});
    reference.parameters.push_back({"p_formulaClass.p_power", "bad"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.filename = "plugin.ulb";
    retained.reference.class_name = "Plugin";
    retained.ast = plugin.ast;
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass.p_power type mismatch", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsOmittedPluginDefault)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula plugin{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n",
        class_options)};
    ASSERT_TRUE(plugin.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.class_name = "Plugin";
    retained.ast = plugin.ast;
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisAcceptsExplicitPluginDefault)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Base param formulaClass\n"
                                            "default=Derived\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    base.ast = std::make_shared<ast::FormulaSections>();
    RetainedFormulaClass derived;
    derived.reference.class_name = "Derived";
    derived.base_class = "Base";
    derived.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&derived);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsMissingExplicitPluginDefault)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Base param formulaClass\n"
                                            "default=Missing\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    base.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&base);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(2U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INCOMPLETE_REFERENCE_GRAPH, diagnostics[0].code);
    EXPECT_EQ("incomplete reference graph: missing retained class Missing", diagnostics[0].message);
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics[1].code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass missing plug-in class", diagnostics[1].message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsIncompatibleExplicitPluginDefault)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Base param formulaClass\n"
                                            "default=Other\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    base.ast = std::make_shared<ast::FormulaSections>();
    RetainedFormulaClass other;
    other.reference.class_name = "Other";
    other.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&other);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass invalid plug-in target", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisUsesPluginDefaultForNestedParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula plugin{load_formula("default:\n"
                                            "float param power\n"
                                            "endparam\n",
        class_options)};
    ASSERT_TRUE(plugin.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass.p_power", "2.0"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.class_name = "Plugin";
    retained.ast = plugin.ast;
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsIncompatiblePluginSelector)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Base param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass", "plugin.ulb:Other"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    base.ast = std::make_shared<ast::FormulaSections>();
    RetainedFormulaClass other;
    other.reference.filename = "plugin.ulb";
    other.reference.class_name = "Other";
    other.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&other);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass invalid plug-in target", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsMissingPluginDefaultClassForNestedParameter)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass.p_power", "2.0"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    const ParameterSetSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(2U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INCOMPLETE_REFERENCE_GRAPH, diagnostics[0].code);
    EXPECT_EQ("incomplete reference graph: missing retained class Plugin", diagnostics[0].message);
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics[1].code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass missing plug-in class", diagnostics[1].message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsInvalidPluginSelector)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass", "missing"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.filename = "other.ulb";
    retained.reference.class_name = "Plugin";
    retained.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass invalid plug-in target", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, parameterSetAnalysisReportsMissingPluginSelectorClass)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("default:\n"
                                            "Plugin param formulaClass\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const parameter::ExtendedParameterEntry parameters;
    parameter::ParameterReferenceSet references;
    parameter::ParameterReference reference;
    reference.site.kind = parameter::ParameterReferenceKind::FRACTAL_FORMULA;
    reference.filename = "Example.ufm";
    reference.entry = "Mandelbrot";
    reference.parameters.push_back({"p_formulaClass", "plugin.ulb:Plugin"});
    references.resolved.push_back({reference, {}, loaded.ast, parser::EntryKind::FRACTAL});
    RetainedFormulaClass retained;
    retained.reference.filename = "other.ulb";
    retained.reference.class_name = "Plugin";
    retained.ast = std::make_shared<ast::FormulaSections>();
    ParameterSetSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_parameter_set(parameters, references, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_PARAMETER_BINDING, diagnostics.front().code);
    EXPECT_EQ("invalid parameter binding: p_formulaClass missing plug-in class", diagnostics.front().message);
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
    ASSERT_EQ(1U, image->constructors.size());
    EXPECT_EQ("Image", image->constructors[0].name);
    EXPECT_TRUE(image->constructors[0].argument_types.empty());
    ASSERT_EQ(8U, image->methods.size());
    EXPECT_EQ("assign", image->methods[0].name);
    EXPECT_EQ("getColor", image->methods[1].name);
    EXPECT_EQ("getEmpty", image->methods[2].name);
    EXPECT_EQ("getHeight", image->methods[3].name);
    EXPECT_EQ("getPixel", image->methods[4].name);
    EXPECT_EQ("getWidth", image->methods[5].name);
    EXPECT_EQ("resize", image->methods[6].name);
    EXPECT_EQ("setPixel", image->methods[7].name);
}

TEST(TestSemanticAnalyzer, defaultRegistryFindsPredefinedSymbols)
{
    const BuiltinRegistry &registry{default_builtin_registry()};
    static constexpr std::array<std::string_view, 26> names{
        "angle",
        "calculationPurpose",
        "center",
        "color",
        "dpixel",
        "dz",
        "e",
        "height",
        "index",
        "magn",
        "maxiter",
        "numiter",
        "pi",
        "pixel",
        "random",
        "randomrange",
        "screenmax",
        "screenpixel",
        "skew",
        "solid",
        "stretch",
        "whitesq",
        "width",
        "x",
        "y",
        "z",
    };

    for (std::string_view name : names)
    {
        EXPECT_TRUE(registry.find_predefined_symbol(name)) << name;
    }
    ASSERT_TRUE(registry.find_predefined_symbol("pixel"));
    EXPECT_EQ(SemanticTypeKind::COMPLEX, registry.find_predefined_symbol("pixel")->type.kind);
    ASSERT_TRUE(registry.find_predefined_symbol("width"));
    EXPECT_EQ(SemanticTypeKind::INT, registry.find_predefined_symbol("width")->type.kind);
    ASSERT_TRUE(registry.find_predefined_symbol("calculationPurpose"));
    EXPECT_TRUE(registry.find_predefined_symbol("calculationPurpose")->constant_expression);
    EXPECT_FALSE(registry.find_predefined_symbol("foo"));
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsPredefinedSymbolRef)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("init:\n"
                                            "complex value\n"
                                            "value=#pixel",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsPredefinedSymbolKindMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("color value\n"
                                            "value=#color",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_BUILTIN_USAGE, diagnostics.front().code);
    EXPECT_EQ("bailout", diagnostics.front().section_name);
    EXPECT_EQ("invalid predefined symbol: #color in bailout", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsPredefinedSymbolSectionMismatch)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    options.entry_kind = parser::EntryKind::TRANSFORMATION;
    const LoadedFormula loaded{load_formula("transform:\n"
                                            "#z=1",
        options)};
    ASSERT_TRUE(loaded.ast);
    FormulaSemanticContext context;
    context.entry_kind = parser::EntryKind::TRANSFORMATION;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_BUILTIN_USAGE, diagnostics.front().code);
    EXPECT_EQ("transform", diagnostics.front().section_name);
    EXPECT_EQ("invalid predefined symbol: #z in transform", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsConstantPredefinedSymbolInArrayDimension)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("init:\n"
                                            "int values[#width]",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsNonConstantPredefinedSymbolInArrayDimension)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("init:\n"
                                            "int values[#x]",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_BUILTIN_USAGE, diagnostics.front().code);
    EXPECT_EQ("init", diagnostics.front().section_name);
    EXPECT_EQ("invalid predefined symbol: #x in constant expression", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsNonConstantPredefinedSymbolInDefaultSetting)
{
    ast::FormulaSections formula;
    formula.defaults =
        std::make_shared<ast::SettingNode>("precision", std::make_shared<ast::ConstantRefNode>("random"));
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(formula, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_BUILTIN_USAGE, diagnostics.front().code);
    EXPECT_EQ("default", diagnostics.front().section_name);
    EXPECT_EQ("invalid predefined symbol: #random in constant expression", diagnostics.front().message);
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

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownParameterRef)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("@param", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_SYMBOL, diagnostics.front().code);
    EXPECT_EQ("unknown symbol: param", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsDeclaredParameterRef)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("init:\n"
                                            "@power\n"
                                            "default:\n"
                                            "float param power\n"
                                            "endparam\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
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

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsReturnOutsideFunction)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("return 1", options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsMissingFunctionReturn)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int func helper()\n"
                                            "int value\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_RETURN, diagnostics.front().code);
    EXPECT_EQ("missing return: helper", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsVoidFunctionWithoutReturn)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("func helper()\n"
                                            "int value\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsIfElseReturnPaths)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("int func helper(bool value)\n"
                                            "if value\n"
                                            "return 1\n"
                                            "else\n"
                                            "return 2\n"
                                            "endif\n"
                                            "endfunc",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
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

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownRetainedBaseClass)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.base_class = "Filter";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::UNKNOWN_TYPE, diagnostics.front().code);
    EXPECT_EQ("Texture", diagnostics.front().entry_name);
    EXPECT_EQ("unknown base class: Filter", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsRetainedClassInheritanceCycle)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass texture;
    texture.reference.class_name = "Texture";
    texture.base_class = "Filter";
    RetainedFormulaClass filter;
    filter.reference.class_name = "Filter";
    filter.base_class = "Texture";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&texture);
    context.retained_classes.push_back(&filter);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INHERITANCE_CYCLE, diagnostics.front().code);
    EXPECT_EQ("Texture", diagnostics.front().entry_name);
    EXPECT_EQ("inheritance cycle: Texture", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsPublicRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "int width",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture\n"
                                            "texture.width",
        options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsClassNameRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "int width",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsClassNameRetainedClassStaticMethod)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "static int func width()\n"
                                                  "return 1\n"
                                                  "endfunc\n",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width()", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsClassNameRetainedClassStaticMethodArity)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "static int func width(int value)\n"
                                                  "return value\n"
                                                  "endfunc\n",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width()", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_CALL_ARITY, diagnostics.front().code);
    EXPECT_EQ("invalid call arity: width expects 1 argument, got 0", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsClassNameRetainedClassStaticMethodArgumentType)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "static int func width(int value)\n"
                                                  "return value\n"
                                                  "endfunc\n",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width(\"wide\")", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid argument type: width got string, expected int", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsClassNameRetainedClassInstanceMethod)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "int func width()\n"
                                                  "return 1\n"
                                                  "endfunc\n",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width()", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsInheritedClassNameRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula base_class{load_formula("public:\n"
                                                "int width",
        class_options)};
    ASSERT_TRUE(base_class.ast);
    const LoadedFormula derived_class{load_formula("public:\n"
                                                   "int height",
        class_options)};
    ASSERT_TRUE(derived_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    base.ast = base_class.ast;
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.base_class = "Base";
    retained.ast = derived_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownClassNameRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("public:\n"
                                                  "int width",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.height", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_MEMBER_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid member access: Texture.height", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsProtectedClassNameRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("protected:\n"
                                                  "int width",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_MEMBER_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid member access: Texture.width is not public", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsPrivateRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("private:\n"
                                                  "int width",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture texture\n"
                                            "texture.width",
        options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_MEMBER_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid member access: Texture.width is not public", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsPrivateClassNameRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula loaded_class{load_formula("private:\n"
                                                  "int width",
        class_options)};
    ASSERT_TRUE(loaded_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.ast = loaded_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_MEMBER_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid member access: Texture.width is not public", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInheritedProtectedClassNameRetainedClassMember)
{
    parser::Options class_options;
    class_options.dialect = Dialect::EXTENDED;
    class_options.entry_kind = parser::EntryKind::CLASS;
    const LoadedFormula base_class{load_formula("protected:\n"
                                                "int width",
        class_options)};
    ASSERT_TRUE(base_class.ast);
    const LoadedFormula derived_class{load_formula("public:\n"
                                                   "int height",
        class_options)};
    ASSERT_TRUE(derived_class.ast);
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture.width", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    base.ast = base_class.ast;
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    retained.base_class = "Base";
    retained.ast = derived_class.ast;
    FormulaSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_MEMBER_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid member access: Texture.width is not public", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsClassIdentifierAsRetainedClassReference)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Texture", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass retained;
    retained.reference.class_name = "Texture";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&retained);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsRetainedClassCast)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Base base\n"
                                            "Derived derived=Derived(base)",
        options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    RetainedFormulaClass derived;
    derived.reference.class_name = "Derived";
    derived.base_class = "Base";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&derived);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsRetainedClassCastArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Base base\n"
                                            "Derived derived=Derived()",
        options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass base;
    base.reference.class_name = "Base";
    RetainedFormulaClass derived;
    derived.reference.class_name = "Derived";
    derived.base_class = "Base";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&base);
    context.retained_classes.push_back(&derived);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_CALL_ARITY, diagnostics.front().code);
    EXPECT_EQ("invalid call arity: Derived expects 1 argument, got 0", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsRetainedClassCastArgumentType)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Derived derived=Derived(1)", options)};
    ASSERT_TRUE(loaded.ast);
    RetainedFormulaClass derived;
    derived.reference.class_name = "Derived";
    FormulaSemanticContext context;
    context.retained_classes.push_back(&derived);

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_ARGUMENT_TYPE, diagnostics.front().code);
    EXPECT_EQ("invalid cast argument: Derived got int", diagnostics.front().message);
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

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsImageConstructorArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image target\n"
                                            "target=new Image()",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsImageConstructorArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image target\n"
                                            "target=new Image(1)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_CALL_ARITY, diagnostics.front().code);
    EXPECT_EQ("invalid call arity: Image expects 0 arguments, got 1", diagnostics.front().message);
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

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsImageMethodMember)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image source\n"
                                            "source.getWidth",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsImageMethodCall)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image source\n"
                                            "int width=source.getWidth()\n"
                                            "color shade=source.getPixel(1, 2)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsImageMethodCallArity)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image source\n"
                                            "source.getPixel(1)",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_CALL_ARITY, diagnostics.front().code);
    EXPECT_EQ("invalid call arity: getPixel expects 2 arguments, got 1", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsUnknownImageMember)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("Image source\n"
                                            "source.missing",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_MEMBER_ACCESS, diagnostics.front().code);
    EXPECT_EQ("invalid member access: Image.missing", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsNumericBailoutSectionResult)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("init:\n"
                                            "z = 0\n"
                                            "bailout:\n"
                                            "1 < 2",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidBailoutSectionResult)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("init:\n"
                                            "z = 0\n"
                                            "bailout:\n"
                                            "\"text\"",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_SECTION_RESULT, diagnostics.front().code);
    EXPECT_EQ("bailout", diagnostics.front().section_name);
    EXPECT_EQ("invalid section result: bailout got string", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsAllowedColorArithmetic)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    options.entry_kind = parser::EntryKind::COLORING;
    const LoadedFormula loaded{load_formula("final:\n"
                                            "color add = rgba(0.1,0.2,0.3,0.4) + rgba(0.5,0.6,0.7,0.8)\n"
                                            "color sub = rgba(0.8,0.7,0.6,0.5) - rgba(0.1,0.2,0.3,0.4)\n"
                                            "color mul = rgba(0.1,0.2,0.3,0.4) * 2\n"
                                            "color div = rgba(0.2,0.4,0.6,0.8) / 2\n"
                                            "add + sub + mul + div",
        options)};
    ASSERT_TRUE(loaded.ast);
    FormulaSemanticContext context;
    context.entry_kind = parser::EntryKind::COLORING;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisAcceptsColorFinalSectionResult)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    options.entry_kind = parser::EntryKind::COLORING;
    const LoadedFormula loaded{load_formula("global:\n"
                                            "color shade\n"
                                            "final:\n"
                                            "shade",
        options)};
    ASSERT_TRUE(loaded.ast);
    FormulaSemanticContext context;
    context.entry_kind = parser::EntryKind::COLORING;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    EXPECT_TRUE(diagnostics.empty());
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidFinalSectionResult)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    options.entry_kind = parser::EntryKind::COLORING;
    const LoadedFormula loaded{load_formula("final:\n"
                                            "\"text\"",
        options)};
    ASSERT_TRUE(loaded.ast);
    FormulaSemanticContext context;
    context.entry_kind = parser::EntryKind::COLORING;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_SECTION_RESULT, diagnostics.front().code);
    EXPECT_EQ("final", diagnostics.front().section_name);
    EXPECT_EQ("invalid section result: final got string", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidSwitchPredefinedSymbol)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    const LoadedFormula loaded{load_formula("switch:\n"
                                            "seed=#z\n",
        options)};
    ASSERT_TRUE(loaded.ast);
    const FormulaSemanticContext context;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_BUILTIN_USAGE, diagnostics.front().code);
    EXPECT_EQ("switch", diagnostics.front().section_name);
    EXPECT_EQ("invalid switch parameter: seed=#z", diagnostics.front().message);
}

TEST(TestSemanticAnalyzer, formulaAnalysisReportsInvalidSectionForEntryKind)
{
    parser::Options options;
    options.dialect = Dialect::EXTENDED;
    options.entry_kind = parser::EntryKind::COLORING;
    const LoadedFormula loaded{load_formula("final:\n"
                                            "1",
        options)};
    ASSERT_TRUE(loaded.ast);
    FormulaSemanticContext context;
    context.entry_kind = parser::EntryKind::FRACTAL;

    const std::vector<SemanticDiagnostic> diagnostics{analyze_formula(*loaded.ast, context)};

    ASSERT_EQ(1U, diagnostics.size());
    EXPECT_EQ(SemanticDiagnosticCode::INVALID_SECTION, diagnostics.front().code);
    EXPECT_EQ("final", diagnostics.front().section_name);
    EXPECT_EQ("invalid section: final for fractal", diagnostics.front().message);
}

} // namespace formula::test
