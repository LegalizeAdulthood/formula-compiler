// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/facade/Formula.h>

#include <formula/parser/ParseOptions.h>

#include <formula/test/ExpressionParam.h>
#include <formula/test/function-call.h>

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

using namespace formula::parser;
using namespace testing;

namespace formula::test
{

using CompileParam = ExpressionParam;
using FormulaSetup = void (*)(const FormulaPtr &formula);

class CompiledFormulaRunSuite : public TestWithParam<CompileParam>
{
};

TEST_P(CompiledFormulaRunSuite, run)
{
    const CompileParam &param{GetParam()};
    const FormulaPtr formula{create_formula(param.text, Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(param.section));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(param.section)};

    EXPECT_NEAR(param.expected_re, result.re, 1e-6);
    EXPECT_NEAR(param.expected_im, result.im, 1e-6);
}

INSTANTIATE_TEST_SUITE_P(TestCompiledFormula, CompiledFormulaRunSuite, ValuesIn(g_expression_params));

void no_setup(const FormulaPtr &)
{
}

void setup_runtime_inputs(const FormulaPtr &formula)
{
    formula->set_value("p1", {1.0, 2.0});
    formula->set_value("p2", {3.0, 4.0});
    formula->set_value("pixel", {-1.0, -2.0});
    formula->set_value("maxit", {256.0, 0.0});
    formula->set_value("scrnmax", {320.0, 200.0});
    formula->set_value("scrnpix", {12.0, 34.0});
}

void setup_fn1_sqr(const FormulaPtr &formula)
{
    ASSERT_TRUE(formula->set_function("fn1", "sqr"));
}

void setup_random_seed(const FormulaPtr &formula)
{
    formula->set_random_seed(5678);
}

struct CompilerParityParam
{
    std::string_view name;
    std::string_view text;
    Section section;
    FormulaSetup setup{no_setup};
    Section before{Section::NONE};
};

inline void PrintTo(const CompilerParityParam &param, std::ostream *os)
{
    *os << param.name;
}

class CompilerParity : public TestWithParam<CompilerParityParam>
{
};

TEST_P(CompilerParity, matchesInterpreter)
{
    const CompilerParityParam &param{GetParam()};
    const FormulaPtr interpreted{create_formula(param.text, Options{})};
    ASSERT_TRUE(interpreted) << "Formula should have parsed";
    const FormulaPtr compiled{create_formula(param.text, Options{})};
    ASSERT_TRUE(compiled) << "Formula should have parsed";
    ASSERT_TRUE(interpreted->get_section(param.section));
    ASSERT_TRUE(compiled->get_section(param.section));
    param.setup(interpreted);
    param.setup(compiled);
    ASSERT_TRUE(compiled->compile());
    if (param.before != Section::NONE)
    {
        interpreted->interpret(param.before);
        compiled->run(param.before);
    }

    const Complex expected{interpreted->interpret(param.section)};
    const Complex actual{compiled->run(param.section)};

    EXPECT_NEAR(expected.re, actual.re, 1e-8);
    EXPECT_NEAR(expected.im, actual.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestCompiledFormulaRun, CompilerParity,
    Values(CompilerParityParam{"unknown_variables", "missing+1", Section::BAILOUT},
        CompilerParityParam{"assignment", "z=4+flip(2)", Section::BAILOUT},
        CompilerParityParam{"chained_assignment",
            "z1=z2=3\n"
            "z1+z2",
            Section::BAILOUT},
        CompilerParityParam{"truthiness_uses_real_part",
            "if(0+flip(1))\n"
            "3\n"
            "else\n"
            "4\n"
            "endif\n",
            Section::BAILOUT},
        CompilerParityParam{"ordering_uses_real_part", "(1+flip(9)) < (2-flip(9))", Section::BAILOUT},
        CompilerParityParam{"equality_uses_both_parts", "(1+flip(2))==(1+flip(3))", Section::BAILOUT},
        CompilerParityParam{"modulus_squared", "|3+flip(4)|", Section::BAILOUT},
        CompilerParityParam{"complex_pair_function_arg", "sqr(3,4)", Section::BAILOUT},
        CompilerParityParam{"cosxx_complex", "cosxx(1+flip(2))", Section::BAILOUT},
        CompilerParityParam{"runtime_defaults", "ismand+lastsqr+rand", Section::BAILOUT},
        CompilerParityParam{
            "runtime_inputs", "p1+p2+pixel+maxit+scrnmax+scrnpix", Section::BAILOUT, setup_runtime_inputs},
        CompilerParityParam{"function_defaults", "fn1(pi/2)+fn2(2)+fn3(1)+fn4(1)", Section::BAILOUT},
        CompilerParityParam{"function_selector", "fn1(2)", Section::BAILOUT, setup_fn1_sqr},
        CompilerParityParam{"client_seeded_rand",
            "loop:\n"
            "rand\n",
            Section::ITERATE, setup_random_seed},
        CompilerParityParam{"formula_seeded_rand",
            "init:\n"
            "srand(1234)\n"
            "loop:\n"
            "rand\n",
            Section::ITERATE, no_setup, Section::INITIALIZE}),
    [](const TestParamInfo<CompilerParityParam> &info) { return std::string{info.param.name}; });

TEST(TestCompiledFormulaRun, identifierComplex)
{
    const FormulaPtr formula{create_formula("z", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile());
    formula->set_value("z", {2.0, 4.0});

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(4.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(2.0, z.re);
    EXPECT_EQ(4.0, z.im);
}

TEST(TestCompiledFormulaRun, addComplex)
{
    const FormulaPtr formula{create_formula("z+q", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {2.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(6.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(2.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, subtractComplex)
{
    const FormulaPtr formula{create_formula("z-q", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {2.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(-2.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(2.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

// (a + bi)(c + di) = (ac - bd) + (ad + bc)i
TEST(TestCompiledFormulaRun, multiplyComplex)
{
    const FormulaPtr formula{create_formula("z*q", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    formula->set_value("q", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    const std::complex<double> expected{std::complex<double>{1.0, 2.0} * std::complex<double>{3.0, 4.0}};
    EXPECT_EQ(expected.real(), result.re);
    EXPECT_EQ(expected.imag(), result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(4.0, q.im);
}

TEST(TestCompiledFormulaRun, divideComplex)
{
    const FormulaPtr formula{create_formula("w/z", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("w", {1.0, 2.0});
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    const std::complex<double> expected{std::complex<double>{1.0, 2.0} / std::complex<double>{3.0, 4.0}};
    EXPECT_EQ(expected.real(), result.re);
    EXPECT_EQ(expected.imag(), result.im);
    const Complex w{formula->get_value("w")};
    EXPECT_EQ(1.0, w.re);
    EXPECT_EQ(2.0, w.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(4.0, z.im);
}

TEST(TestCompiledFormulaRun, unaryNegateComplex)
{
    const FormulaPtr formula{create_formula("-z", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(-1.0, result.re);
    EXPECT_EQ(-2.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(1.0, z.re);
    EXPECT_EQ(2.0, z.im);
}

TEST(TestCompiledFormulaRun, assignment)
{
    const FormulaPtr formula{create_formula("z=4+2", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(6.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(6.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, complexAssignmentStoresBothComponents)
{
    const FormulaPtr formula{create_formula("z=3+flip(4)", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{3.0, 4.0}), result);
    EXPECT_EQ((Complex{3.0, 4.0}), formula->get_value("z"));
}

TEST(TestCompiledFormulaRun, complexAssignmentPersistsAcrossSections)
{
    const FormulaPtr formula{create_formula("loop:\n"
                                            "z=3+flip(4)\n"
                                            "bailout:\n"
                                            "z\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    EXPECT_EQ((Complex{3.0, 4.0}), formula->run(Section::ITERATE));
    EXPECT_EQ((Complex{3.0, 4.0}), formula->run(Section::BAILOUT));
}

TEST(TestCompiledFormulaRun, chainedAssignment)
{
    const FormulaPtr formula{create_formula("z1=z2=3", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z1{formula->get_value("z1")};
    EXPECT_EQ(3.0, z1.re);
    EXPECT_EQ(0.0, z1.im);
    const Complex z2{formula->get_value("z2")};
    EXPECT_EQ(3.0, z2.re);
    EXPECT_EQ(0.0, z2.im);
}

TEST(TestCompiledFormulaRun, modulus)
{
    const FormulaPtr formula{create_formula("|z|", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {-3.0, -2.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(13.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, sqrUpdatesLastsqr)
{
    const FormulaPtr formula{create_formula("bailout:\n"
                                            "ignored=sqr(3+flip(4))\n"
                                            "lastsqr\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{25.0, 0.0}), result);
    EXPECT_EQ((Complex{25.0, 0.0}), formula->get_value("lastsqr"));
}

TEST(TestCompiledFormulaRun, sqrReplacesLastsqrOnEachCall)
{
    const FormulaPtr formula{create_formula("bailout:\n"
                                            "ignored=sqr(3+flip(4))\n"
                                            "ignored=sqr(1+flip(2))\n"
                                            "lastsqr\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{5.0, 0.0}), result);
    EXPECT_EQ((Complex{5.0, 0.0}), formula->get_value("lastsqr"));
}

TEST(TestCompiledFormulaRun, modulusDoesNotUpdateLastsqr)
{
    const FormulaPtr formula{create_formula("bailout:\n"
                                            "ignored=|3+flip(4)|\n"
                                            "lastsqr\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("lastsqr", {7.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{7.0, 0.0}), result);
    EXPECT_EQ((Complex{7.0, 0.0}), formula->get_value("lastsqr"));
}

TEST(TestCompiledFormulaRun, conjugate)
{
    const FormulaPtr formula{create_formula("conj(z)", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(-4.0, result.im);
}

TEST(TestCompiledFormulaRun, identity)
{
    const FormulaPtr formula{create_formula("ident(z)", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    ASSERT_EQ(3.0, result.re);
    ASSERT_EQ(4.0, result.im);
}

TEST(TestCompiledFormulaRun, functionSelectorsUseIdDefaults)
{
    const FormulaPtr formula{create_formula("fn1(pi/2) + fn2(2) + fn3(1) + fn4(1)", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ("sin", formula->get_function("fn1"));
    EXPECT_EQ("sqr", formula->get_function("fn2"));
    EXPECT_EQ("sinh", formula->get_function("fn3"));
    EXPECT_EQ("cosh", formula->get_function("fn4"));
    EXPECT_NEAR(1.0 + 4.0 + std::sinh(1.0) + std::cosh(1.0), result.re, 1e-8);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, functionSelectorUsesBoundBuiltin)
{
    const FormulaPtr formula{create_formula("fn1(pi/2)", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->set_function("fn1", "sin"));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ("sin", formula->get_function("fn1"));
    EXPECT_NEAR(1.0, result.re, 1e-8);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, selectedSqrUpdatesLastsqr)
{
    const FormulaPtr formula{create_formula("bailout:\n"
                                            "ignored=fn1(3+flip(4))\n"
                                            "lastsqr\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->set_function("fn1", "sqr"));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{25.0, 0.0}), result);
    EXPECT_EQ((Complex{25.0, 0.0}), formula->get_value("lastsqr"));
}

TEST(TestCompiledFormulaRun, basicUnknownVariableCompilesAsZero)
{
    const FormulaPtr formula{create_formula("missing + 1", Options{})};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, basicCompileSectionDispatchIsUnchanged)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z+1,|z|<4", Options{})};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::INITIALIZE));
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("pixel", {2.0, 0.0});
    ASSERT_TRUE(formula->compile());

    EXPECT_EQ(2.0, formula->run(Section::INITIALIZE).re);
    EXPECT_EQ(3.0, formula->run(Section::ITERATE).re);
    EXPECT_EQ(0.0, formula->run(Section::BAILOUT).re);
}

TEST(TestCompiledFormulaRun, documentedRuntimeDefaultsCompile)
{
    const FormulaPtr formula{create_formula("ismand + lastsqr + rand", Options{})};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
}

TEST(TestCompiledFormulaRun, randDefaultsToZeroBeforeIteration)
{
    const FormulaPtr formula{create_formula("rand", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{0.0, 0.0}), result);
}

TEST(TestCompiledFormulaRun, randChangesEachIteration)
{
    const FormulaPtr formula{create_formula("loop:\n"
                                            "z=rand\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    formula->set_random_seed(1234);
    ASSERT_TRUE(formula->compile());

    const Complex first{formula->run(Section::ITERATE)};
    const Complex second{formula->run(Section::ITERATE)};

    EXPECT_GE(first.re, 0.0);
    EXPECT_LT(first.re, 1.0);
    EXPECT_GE(first.im, 0.0);
    EXPECT_LT(first.im, 1.0);
    EXPECT_NE(first, second);
}

TEST(TestCompiledFormulaRun, clientSeedRepeatsRandomSequence)
{
    const auto first_iteration = []
    {
        const FormulaPtr formula{create_formula("loop:\n"
                                                "rand\n",
            Options{})};
        formula->set_random_seed(5678);
        formula->compile();
        return formula->run(Section::ITERATE);
    };

    EXPECT_EQ(first_iteration(), first_iteration());
}

TEST(TestCompiledFormulaRun, srandRepeatsRandomSequence)
{
    const auto first_iteration = []
    {
        const FormulaPtr formula{create_formula("init:\n"
                                                "srand(1234)\n"
                                                "loop:\n"
                                                "rand\n",
            Options{})};
        formula->compile();
        formula->run(Section::INITIALIZE);
        return formula->run(Section::ITERATE);
    };

    EXPECT_EQ(first_iteration(), first_iteration());
}

TEST(TestCompiledFormulaRun, srandOverridesClientSeed)
{
    const auto first_iteration = [](const std::uint32_t client_seed)
    {
        const FormulaPtr formula{create_formula("init:\n"
                                                "srand(1234)\n"
                                                "loop:\n"
                                                "rand\n",
            Options{})};
        formula->set_random_seed(client_seed);
        formula->compile();
        formula->run(Section::INITIALIZE);
        return formula->run(Section::ITERATE);
    };

    EXPECT_EQ(first_iteration(5678), first_iteration(8765));
}

TEST(TestCompiledFormulaRun, srandReturnsZeroAndResetsRand)
{
    const FormulaPtr formula{create_formula("init:\n"
                                            "srand(1234)\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::INITIALIZE));
    formula->set_value("rand", {1.0, 2.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::INITIALIZE)};

    EXPECT_EQ((Complex{0.0, 0.0}), result);
    EXPECT_EQ((Complex{0.0, 0.0}), formula->get_value("rand"));
}

TEST(TestCompiledFormulaRun, settingClientSeedResetsRandUntilIteration)
{
    const FormulaPtr formula{create_formula("loop:\n"
                                            "rand\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    ASSERT_TRUE(formula->compile());
    formula->run(Section::ITERATE);

    formula->set_random_seed(5678);

    EXPECT_EQ((Complex{0.0, 0.0}), formula->get_value("rand"));
}

struct CompiledRuntimeInputParam
{
    std::string_view name;
    Complex value;
};

inline void PrintTo(const CompiledRuntimeInputParam &param, std::ostream *os)
{
    *os << param.name;
}

class CompiledRuntimeInputs : public TestWithParam<CompiledRuntimeInputParam>
{
};

TEST_P(CompiledRuntimeInputs, clientSuppliedPredefinedValueIsReadAfterCompile)
{
    const CompiledRuntimeInputParam &param{GetParam()};
    const FormulaPtr formula{create_formula(param.name, Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());
    formula->set_value(param.name, param.value);

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(param.value, result);
}

INSTANTIATE_TEST_SUITE_P(TestCompiledFormulaRun, CompiledRuntimeInputs,
    Values(CompiledRuntimeInputParam{"p1", {1.0, 2.0}}, CompiledRuntimeInputParam{"p2", {3.0, 4.0}},
        CompiledRuntimeInputParam{"p3", {5.0, 6.0}}, CompiledRuntimeInputParam{"p4", {7.0, 8.0}},
        CompiledRuntimeInputParam{"p5", {9.0, 10.0}}, CompiledRuntimeInputParam{"pixel", {-1.0, -2.0}},
        CompiledRuntimeInputParam{"maxit", {256.0, 0.0}}, CompiledRuntimeInputParam{"scrnmax", {320.0, 200.0}},
        CompiledRuntimeInputParam{"scrnpix", {12.0, 34.0}}, CompiledRuntimeInputParam{"whitesq", {1.0, 0.0}},
        CompiledRuntimeInputParam{"center", {-0.75, 0.1}}, CompiledRuntimeInputParam{"magxmag", {2.0, 3.0}},
        CompiledRuntimeInputParam{"rotskew", {45.0, 0.25}}, CompiledRuntimeInputParam{"ismand", {0.0, 0.0}}),
    [](const TestParamInfo<CompiledRuntimeInputParam> &info) { return std::string{info.param.name}; });

TEST(TestCompiledFormulaRun, clientSuppliedRuntimeInputsCanBeCombinedAfterCompile)
{
    const FormulaPtr formula{
        create_formula("p1+p2+p3+p4+p5+pixel+maxit+scrnmax+scrnpix+whitesq+center+magxmag+rotskew", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());
    formula->set_value("p1", {1.0, 2.0});
    formula->set_value("p2", {3.0, 4.0});
    formula->set_value("p3", {5.0, 6.0});
    formula->set_value("p4", {7.0, 8.0});
    formula->set_value("p5", {9.0, 10.0});
    formula->set_value("pixel", {-1.0, -2.0});
    formula->set_value("maxit", {256.0, 0.0});
    formula->set_value("scrnmax", {320.0, 200.0});
    formula->set_value("scrnpix", {12.0, 34.0});
    formula->set_value("whitesq", {1.0, 0.0});
    formula->set_value("center", {-0.75, 0.1});
    formula->set_value("magxmag", {2.0, 3.0});
    formula->set_value("rotskew", {45.0, 0.25});

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ((Complex{659.25, 265.35}), result);
}

TEST(TestCompiledFormulaRun, extendedCompilerStillRejectsUnsupportedNodes)
{
    Options options;
    options.dialect = Dialect::EXTENDED;
    const FormulaPtr formula{create_formula("init:\n"
                                            "int value",
        options)};
    ASSERT_TRUE(formula);
    ASSERT_TRUE(formula->get_section(Section::INITIALIZE));

    EXPECT_FALSE(formula->compile());
}

TEST(TestCompiledFormulaRun, recompileUsesUpdatedFunctionSelector)
{
    const FormulaPtr formula{create_formula("fn1(2)", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->set_function("fn1", "sin"));
    ASSERT_TRUE(formula->compile());
    EXPECT_NEAR(std::sin(2.0), formula->run(Section::BAILOUT).re, 1e-8);

    ASSERT_TRUE(formula->set_function("fn1", "sqr"));
    ASSERT_TRUE(formula->compile());

    EXPECT_EQ((Complex{4.0, 0.0}), formula->run(Section::BAILOUT));
}

TEST(TestCompiledFormulaRun, recompilePreservesLiveSymbolStorage)
{
    const FormulaPtr formula{create_formula("z+1", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {1.0, 2.0});
    ASSERT_TRUE(formula->compile());
    EXPECT_EQ((Complex{2.0, 2.0}), formula->run(Section::BAILOUT));

    formula->set_value("z", {3.0, 4.0});
    ASSERT_TRUE(formula->compile());

    EXPECT_EQ((Complex{4.0, 4.0}), formula->run(Section::BAILOUT));
}

TEST(TestCompiledFormulaRun, recompileUsesResetRandomSeed)
{
    const auto first_iteration = [](const std::uint32_t seed)
    {
        const FormulaPtr formula{create_formula("loop:\n"
                                                "rand\n",
            Options{})};
        formula->set_random_seed(1);
        formula->compile();
        formula->run(Section::ITERATE);
        formula->set_random_seed(seed);
        formula->compile();
        return formula->run(Section::ITERATE);
    };

    EXPECT_EQ(first_iteration(5678), first_iteration(5678));
}

TEST(TestCompiledFormulaRun, assignmentStatementsIterate)
{
    const FormulaPtr formula{create_formula("q=3\n"
                                            "z=4\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::ITERATE)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(3.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, assignmentStatementsBailout)
{
    const FormulaPtr formula{create_formula("q=3\n"
                                            "z=4\n",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

// Non-short-circuit compiled logical evaluation is covered by implementation
// and expression result tests only. Parsed formulas cannot express a logical
// RHS side effect because assignment is statement syntax, not expression syntax.
// Extended user functions can cover this once they can mutate globals in the
// compiler.

TEST(TestCompiledFormulaRun, formulaInitialize)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::INITIALIZE));
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {100.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::INITIALIZE)};

    EXPECT_EQ(4.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex pixel{formula->get_value("pixel")};
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaIterate)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::ITERATE));
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {2.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::ITERATE)};

    EXPECT_EQ(8.4, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex pixel{formula->get_value("pixel")};
    EXPECT_EQ(4.4, pixel.re);
    EXPECT_EQ(0.0, pixel.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(8.4, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaBailoutFalse)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {2.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(2.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, formulaBailoutTrue)
{
    const FormulaPtr formula{create_formula("z=pixel:z=z*z+pixel,|z|>4", Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("pixel", {4.4, 0.0});
    formula->set_value("z", {8.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(1.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(8.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

namespace
{

class RunFunctionCall : public TestWithParam<FunctionCallParam>
{
};

} // namespace

TEST_P(RunFunctionCall, run)
{
    const FunctionCallParam &param{GetParam()};
    const FormulaPtr formula{create_formula(param.expr, Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_NEAR(param.result.re, result.re, 1e-8);
    EXPECT_NEAR(param.result.im, result.im, 1e-8);
}

INSTANTIATE_TEST_SUITE_P(TestFormula, RunFunctionCall, ValuesIn(g_calls));

TEST(TestCompiledFormulaRun, ifStatementBodyTrue)
{
    const FormulaPtr formula{create_formula("if(5)\n"
                                            "z=3\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(3.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(5<4)\n"
                                            "z=3\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {5.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(0.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(5.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifThenElseComplexBodyConditionFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "x=1\n"
                                            "y=2\n"
                                            "else\n"
                                            "z=3\n"
                                            "q=4\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("x", {0.0, 0.0});
    formula->set_value("y", {0.0, 0.0});
    formula->set_value("z", {0.0, 0.0});
    formula->set_value("q", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex x{formula->get_value("x")};
    EXPECT_EQ(0.0, x.re);
    EXPECT_EQ(0.0, x.im);
    const Complex y{formula->get_value("y")};
    EXPECT_EQ(0.0, y.re);
    EXPECT_EQ(0.0, y.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(3.0, z.re);
    EXPECT_EQ(0.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(4.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, ifThenElseComplexBodyConditionTrue)
{
    const FormulaPtr formula{create_formula("if(1)\n"
                                            "x=1\n"
                                            "y=2\n"
                                            "else\n"
                                            "z=3\n"
                                            "q=4\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("x", {0.0, 0.0});
    formula->set_value("y", {0.0, 0.0});
    formula->set_value("z", {0.0, 0.0});
    formula->set_value("q", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(2.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex x{formula->get_value("x")};
    EXPECT_EQ(1.0, x.re);
    EXPECT_EQ(0.0, x.im);
    const Complex y{formula->get_value("y")};
    EXPECT_EQ(2.0, y.re);
    EXPECT_EQ(0.0, y.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(0.0, z.re);
    EXPECT_EQ(0.0, z.im);
    const Complex q{formula->get_value("q")};
    EXPECT_EQ(0.0, q.re);
    EXPECT_EQ(0.0, q.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementBodyTrue)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(1)\n"
                                            "z=4\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifElseIfStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(0)\n"
                                            "z=3\n"
                                            "else\n"
                                            "z=4\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

TEST(TestCompiledFormulaRun, ifMultipleElseIfStatementBodyFalse)
{
    const FormulaPtr formula{create_formula("if(0)\n"
                                            "z=1\n"
                                            "elseif(0)\n"
                                            "z=3\n"
                                            "elseif(1)\n"
                                            "z=4\n"
                                            "else\n"
                                            "z=5\n"
                                            "endif",
        Options{})};
    ASSERT_TRUE(formula) << "Formula should have parsed";
    ASSERT_TRUE(formula->get_section(Section::BAILOUT));
    formula->set_value("z", {0.0, 0.0});
    ASSERT_TRUE(formula->compile());

    const Complex result{formula->run(Section::BAILOUT)};

    EXPECT_EQ(4.0, result.re);
    EXPECT_EQ(0.0, result.im);
    const Complex z{formula->get_value("z")};
    EXPECT_EQ(4.0, z.re);
    EXPECT_EQ(0.0, z.im);
}

} // namespace formula::test
