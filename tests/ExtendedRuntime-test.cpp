// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedRuntime.h>

#include <gtest/gtest.h>

#include <stdexcept>

namespace formula::test
{

namespace
{

Value plugin_value(std::string class_name, int field_value)
{
    PluginValue plugin;
    plugin.filename = "common.ulb";
    plugin.class_name = std::move(class_name);
    plugin.base_class = "Base";
    plugin.object_fields = {{"field", Value{field_value}}};
    plugin.object_initialized = true;
    return make_plugin_value(std::move(plugin));
}

} // namespace

TEST(TestExtendedRuntime, setGetAndMissingValues)
{
    ExtendedRuntimeState state;
    state.set_formula_value("SourceName", Value{3});

    EXPECT_EQ(Value{3}, state.value("SourceName"));
    EXPECT_EQ(Value{}, state.value("sourcename"));
    EXPECT_EQ("SourceName", state.source_name("SourceName"));
    EXPECT_EQ("", state.source_name("sourcename"));
}

TEST(TestExtendedRuntime, storesPluginValuesInFormulaParameterAndPredefinedScopes)
{
    ExtendedRuntimeState state;
    const Value formula{plugin_value("FormulaPlugin", 1)};
    const Value parameter{plugin_value("ParameterPlugin", 2)};
    const Value predefined{plugin_value("PredefinedPlugin", 3)};

    state.set_formula_value("plugin", formula);
    state.set_parameter_value("@plugin", parameter);
    state.set_predefined_value("#plugin", predefined, true);

    EXPECT_EQ(formula, state.value("plugin"));
    EXPECT_EQ(parameter, state.parameter_value("plugin"));
    EXPECT_EQ(parameter, state.parameter_value("@plugin"));
    EXPECT_EQ(predefined, state.predefined_value("plugin"));
    EXPECT_EQ(predefined, state.predefined_value("#plugin"));
}

TEST(TestExtendedRuntime, localScopesShadowFormulaValues)
{
    ExtendedRuntimeState state;
    state.set_formula_value("x", Value{1});

    state.push_local_scope();
    state.declare_local_value("x", Value{2});

    EXPECT_EQ(Value{2}, state.value("x"));

    state.pop_local_scope();

    EXPECT_EQ(Value{1}, state.value("x"));
}

TEST(TestExtendedRuntime, lvalueAssignsFormulaVariables)
{
    ExtendedRuntimeState state;
    RuntimeLValue target{state.lvalue("z")};

    ASSERT_TRUE(target.valid());
    EXPECT_TRUE(target.writable());

    target.set(Value{4});

    EXPECT_EQ(Value{4}, state.value("z"));
}

TEST(TestExtendedRuntime, formulaAndParameterLvaluesReplacePluginValues)
{
    ExtendedRuntimeState state;
    const Value formula{plugin_value("FormulaPlugin", 1)};
    const Value parameter{plugin_value("ParameterPlugin", 2)};
    const Value replacement{plugin_value("ReplacementPlugin", 3)};
    state.set_formula_value("plugin", formula);
    state.set_parameter_value("@plugin", parameter, true);
    state.set_predefined_value("#plugin", plugin_value("PredefinedPlugin", 4), true);

    state.lvalue("plugin").set(replacement);
    state.parameter_lvalue("@plugin").set(formula);

    EXPECT_EQ(replacement, state.value("plugin"));
    EXPECT_EQ(formula, state.parameter_value("plugin"));
    EXPECT_EQ(plugin_value("PredefinedPlugin", 4), state.predefined_value("plugin"));
}

TEST(TestExtendedRuntime, byReferenceBindingMutatesTarget)
{
    ExtendedRuntimeState state;
    state.set_formula_value("target", Value{1});
    state.push_function_frame();
    state.bind_local_reference("arg", state.lvalue("target"));

    state.lvalue("arg").set(Value{9});

    EXPECT_EQ(Value{9}, state.value("target"));

    state.pop_function_frame();
    EXPECT_EQ(Value{}, state.value("arg"));
}

TEST(TestExtendedRuntime, arrayElementLvalueAssignsElement)
{
    auto array = std::make_shared<ArrayValue>();
    array->element_kind = ValueKind::INT;
    array->elements = {Value{1}, Value{2}};

    RuntimeLValue element{RuntimeLValue::array_element(array, 1)};
    element.set(Value{7});

    EXPECT_EQ(Value{7}, array->elements[1]);
}

TEST(TestExtendedRuntime, arrayElementLvalueChecksBounds)
{
    auto array = std::make_shared<ArrayValue>();
    array->element_kind = ValueKind::INT;
    array->elements = {Value{1}};

    RuntimeLValue element{RuntimeLValue::array_element(array, 1)};

    EXPECT_THROW(element.get(), std::runtime_error);
    EXPECT_THROW(element.set(Value{7}), std::runtime_error);
}

TEST(TestExtendedRuntime, parameterAndPredefinedNamesUseSigilFreeKeys)
{
    ExtendedRuntimeState state;
    state.set_parameter_value("@p", Value{5});
    state.set_predefined_value("#pixel", Value{Complex{1.0, 2.0}}, true);

    EXPECT_EQ(Value{5}, state.parameter_value("p"));
    EXPECT_EQ(Value{5}, state.parameter_value("@p"));
    EXPECT_EQ((Value{Complex{1.0, 2.0}}), state.predefined_value("pixel"));
    EXPECT_EQ((Value{Complex{1.0, 2.0}}), state.predefined_value("#pixel"));
}

TEST(TestExtendedRuntime, readOnlyLvalueRejectsAssignment)
{
    ExtendedRuntimeState state;
    state.set_parameter_value("p", Value{5});
    state.set_predefined_value("pi", Value{3.14}, false);

    EXPECT_FALSE(state.parameter_lvalue("p").writable());
    EXPECT_FALSE(state.predefined_lvalue("#pi").writable());
    EXPECT_THROW(state.parameter_lvalue("p").set(Value{6}), std::runtime_error);
    EXPECT_THROW(state.predefined_lvalue("pi").set(Value{4.0}), std::runtime_error);
}

TEST(TestExtendedRuntime, readOnlyPluginLvaluesRejectAssignment)
{
    ExtendedRuntimeState state;
    const Value parameter{plugin_value("ParameterPlugin", 1)};
    const Value predefined{plugin_value("PredefinedPlugin", 2)};
    state.set_parameter_value("@plugin", parameter);
    state.set_predefined_value("#plugin", predefined, false);

    EXPECT_FALSE(state.parameter_lvalue("plugin").writable());
    EXPECT_FALSE(state.predefined_lvalue("#plugin").writable());
    EXPECT_THROW(state.parameter_lvalue("@plugin").set(plugin_value("Replacement", 3)), std::runtime_error);
    EXPECT_THROW(state.predefined_lvalue("plugin").set(plugin_value("Replacement", 4)), std::runtime_error);
    EXPECT_EQ(parameter, state.parameter_value("plugin"));
    EXPECT_EQ(predefined, state.predefined_value("plugin"));
}

TEST(TestExtendedRuntime, missingPluginLookupsReturnEmptyValues)
{
    ExtendedRuntimeState state;
    state.set_formula_value("plugin", plugin_value("FormulaPlugin", 1));
    state.set_parameter_value("@plugin", plugin_value("ParameterPlugin", 2));
    state.set_predefined_value("#plugin", plugin_value("PredefinedPlugin", 3), true);

    EXPECT_EQ(Value{}, state.value("missing"));
    EXPECT_FALSE(state.has_parameter_value("missing"));
    EXPECT_EQ(Value{}, state.parameter_value("missing"));
    EXPECT_FALSE(state.has_predefined_value("#missing"));
    EXPECT_EQ(Value{}, state.predefined_value("#missing"));
}

TEST(TestExtendedRuntime, rebindingPluginParameterIsIsolatedFromOtherScopes)
{
    ExtendedRuntimeState state;
    const Value formula{plugin_value("FormulaPlugin", 1)};
    const Value predefined{plugin_value("PredefinedPlugin", 2)};
    const Value original_parameter{plugin_value("ParameterPlugin", 3)};
    const Value replacement{plugin_value("ReplacementPlugin", 4)};
    state.set_formula_value("plugin", formula);
    state.set_parameter_value("@plugin", original_parameter);
    state.set_predefined_value("#plugin", predefined, true);

    state.set_parameter_value("plugin", replacement);

    EXPECT_EQ(formula, state.value("plugin"));
    EXPECT_EQ(replacement, state.parameter_value("@plugin"));
    EXPECT_EQ(predefined, state.predefined_value("plugin"));
}

TEST(TestExtendedRuntime, readOnlyAliasRejectsAssignment)
{
    ExtendedRuntimeState state;
    state.set_formula_value("target", Value{5});
    state.push_function_frame();
    state.bind_local_reference("arg", state.lvalue("target").read_only());

    EXPECT_EQ(Value{5}, state.value("arg"));
    EXPECT_THROW(state.lvalue("arg").set(Value{6}), std::runtime_error);
    EXPECT_EQ(Value{5}, state.value("target"));
}

TEST(TestExtendedRuntime, recordsRuntimeMessages)
{
    ExtendedRuntimeState state;
    state.add_message("first");
    state.add_message("second");

    ASSERT_EQ(2U, state.messages().size());
    EXPECT_EQ("first", state.messages()[0]);
    EXPECT_EQ("second", state.messages()[1]);
}

} // namespace formula::test
