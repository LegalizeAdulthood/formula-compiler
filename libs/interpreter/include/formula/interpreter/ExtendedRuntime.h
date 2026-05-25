// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/core/Value.h>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace formula
{

class RuntimeLValue
{
public:
    using Getter = std::function<Value()>;
    using Setter = std::function<void(Value)>;

    RuntimeLValue() = default;
    RuntimeLValue(Getter getter, Setter setter, bool writable);

    static RuntimeLValue variable(std::shared_ptr<Value> value, bool writable = true);
    static RuntimeLValue array_element(Value::ArrayPtr array, std::size_t index, bool writable = true);

    bool valid() const;
    bool writable() const;
    RuntimeLValue read_only() const;
    Value get() const;
    void set(Value value) const;

private:
    Getter m_getter;
    Setter m_setter;
    bool m_writable{};
};

struct RuntimeBinding
{
    std::string source_name;
    RuntimeLValue value;
};

class ExtendedRuntimeState
{
public:
    void set_formula_value(std::string_view name, Value value);
    void declare_local_value(std::string_view name, Value value, bool writable = true);
    void bind_local_reference(std::string_view name, RuntimeLValue value);

    Value value(std::string_view name) const;
    RuntimeLValue lvalue(std::string_view name);
    std::string source_name(std::string_view name) const;

    void push_local_scope();
    void pop_local_scope();
    void push_function_frame();
    void pop_function_frame();

    void set_parameter_value(std::string_view name, Value value, bool writable = false);
    bool has_parameter_value(std::string_view name) const;
    Value parameter_value(std::string_view name) const;
    RuntimeLValue parameter_lvalue(std::string_view name);

    void set_predefined_value(std::string_view name, Value value, bool writable);
    bool has_predefined_value(std::string_view name) const;
    Value predefined_value(std::string_view name) const;
    RuntimeLValue predefined_lvalue(std::string_view name);

    void add_message(std::string message);
    const std::vector<std::string> &messages() const;

private:
    using RuntimeScope = std::unordered_map<std::string, RuntimeBinding>;

    static RuntimeBinding make_binding(std::string_view name, Value value, bool writable);
    static std::string parameter_key(std::string_view name);
    static std::string predefined_key(std::string_view name);

    const RuntimeBinding *find_binding(std::string_view name) const;
    RuntimeBinding *find_binding(std::string_view name);

    RuntimeScope m_formula_scope;
    std::vector<RuntimeScope> m_local_scopes;
    std::vector<std::size_t> m_function_frame_markers;
    RuntimeScope m_parameters;
    RuntimeScope m_predefined;
    std::vector<std::string> m_messages;
};

} // namespace formula
