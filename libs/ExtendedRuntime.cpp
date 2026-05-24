// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ExtendedRuntime.h>

#include <stdexcept>
#include <utility>

namespace formula
{

namespace
{

std::string remove_prefix(std::string_view name, char prefix)
{
    if (!name.empty() && name.front() == prefix)
    {
        name.remove_prefix(1);
    }
    return std::string{name};
}

void check_array_index(const Value::ArrayPtr &array, std::size_t index)
{
    if (!array)
    {
        throw std::runtime_error("invalid array lvalue");
    }
    if (index >= array->elements.size())
    {
        throw std::runtime_error("array index out of range");
    }
}

} // namespace

RuntimeLValue::RuntimeLValue(Getter getter, Setter setter, bool writable) :
    m_getter(std::move(getter)),
    m_setter(std::move(setter)),
    m_writable(writable)
{
}

RuntimeLValue RuntimeLValue::variable(std::shared_ptr<Value> value, bool writable)
{
    return RuntimeLValue{[value]()
        {
            if (!value)
            {
                throw std::runtime_error("invalid variable lvalue");
            }
            return *value;
        },
        [value](Value next)
        {
            if (!value)
            {
                throw std::runtime_error("invalid variable lvalue");
            }
            *value = std::move(next);
        },
        writable};
}

RuntimeLValue RuntimeLValue::array_element(Value::ArrayPtr array, std::size_t index, bool writable)
{
    return RuntimeLValue{[array, index]()
        {
            check_array_index(array, index);
            return array->elements[index];
        },
        [array, index](Value next)
        {
            check_array_index(array, index);
            array->elements[index] = std::move(next);
        },
        writable};
}

bool RuntimeLValue::valid() const
{
    return static_cast<bool>(m_getter);
}

bool RuntimeLValue::writable() const
{
    return valid() && m_writable;
}

RuntimeLValue RuntimeLValue::read_only() const
{
    return RuntimeLValue{m_getter, m_setter, false};
}

Value RuntimeLValue::get() const
{
    if (!valid())
    {
        throw std::runtime_error("invalid lvalue");
    }
    return m_getter();
}

void RuntimeLValue::set(Value value) const
{
    if (!valid())
    {
        throw std::runtime_error("invalid lvalue");
    }
    if (!m_writable)
    {
        throw std::runtime_error("lvalue is read-only");
    }
    m_setter(std::move(value));
}

void ExtendedRuntimeState::set_formula_value(std::string_view name, Value value)
{
    const std::string key{name};
    if (RuntimeBinding *binding = find_binding(key))
    {
        binding->value.set(std::move(value));
        return;
    }
    m_formula_scope[key] = make_binding(name, std::move(value), true);
}

void ExtendedRuntimeState::declare_local_value(std::string_view name, Value value, bool writable)
{
    if (m_local_scopes.empty())
    {
        m_formula_scope[std::string{name}] = make_binding(name, std::move(value), writable);
        return;
    }
    m_local_scopes.back()[std::string{name}] = make_binding(name, std::move(value), writable);
}

void ExtendedRuntimeState::bind_local_reference(std::string_view name, RuntimeLValue value)
{
    if (!value.valid())
    {
        throw std::runtime_error("cannot bind invalid lvalue");
    }
    if (m_local_scopes.empty())
    {
        push_local_scope();
    }
    m_local_scopes.back()[std::string{name}] = RuntimeBinding{std::string{name}, std::move(value)};
}

Value ExtendedRuntimeState::value(std::string_view name) const
{
    if (const RuntimeBinding *binding = find_binding(name))
    {
        return binding->value.get();
    }
    return {};
}

RuntimeLValue ExtendedRuntimeState::lvalue(std::string_view name)
{
    if (RuntimeBinding *binding = find_binding(name))
    {
        return binding->value;
    }
    m_formula_scope[std::string{name}] = make_binding(name, Value{}, true);
    return m_formula_scope[std::string{name}].value;
}

std::string ExtendedRuntimeState::source_name(std::string_view name) const
{
    if (const RuntimeBinding *binding = find_binding(name))
    {
        return binding->source_name;
    }
    return {};
}

void ExtendedRuntimeState::push_local_scope()
{
    m_local_scopes.emplace_back();
}

void ExtendedRuntimeState::pop_local_scope()
{
    if (m_local_scopes.empty())
    {
        throw std::runtime_error("no local scope to pop");
    }
    m_local_scopes.pop_back();
}

void ExtendedRuntimeState::push_function_frame()
{
    m_function_frame_markers.push_back(m_local_scopes.size());
    push_local_scope();
}

void ExtendedRuntimeState::pop_function_frame()
{
    if (m_function_frame_markers.empty())
    {
        throw std::runtime_error("no function frame to pop");
    }
    m_local_scopes.resize(m_function_frame_markers.back());
    m_function_frame_markers.pop_back();
}

void ExtendedRuntimeState::set_parameter_value(std::string_view name, Value value, bool writable)
{
    const std::string key{parameter_key(name)};
    m_parameters[key] = make_binding(key, std::move(value), writable);
}

Value ExtendedRuntimeState::parameter_value(std::string_view name) const
{
    const std::string key{parameter_key(name)};
    if (const auto it = m_parameters.find(key); it != m_parameters.end())
    {
        return it->second.value.get();
    }
    return {};
}

RuntimeLValue ExtendedRuntimeState::parameter_lvalue(std::string_view name)
{
    const std::string key{parameter_key(name)};
    if (const auto it = m_parameters.find(key); it != m_parameters.end())
    {
        return it->second.value;
    }
    m_parameters[key] = make_binding(key, Value{}, false);
    return m_parameters[key].value;
}

void ExtendedRuntimeState::set_predefined_value(std::string_view name, Value value, bool writable)
{
    const std::string key{predefined_key(name)};
    m_predefined[key] = make_binding(key, std::move(value), writable);
}

Value ExtendedRuntimeState::predefined_value(std::string_view name) const
{
    const std::string key{predefined_key(name)};
    if (const auto it = m_predefined.find(key); it != m_predefined.end())
    {
        return it->second.value.get();
    }
    return {};
}

RuntimeLValue ExtendedRuntimeState::predefined_lvalue(std::string_view name)
{
    const std::string key{predefined_key(name)};
    if (const auto it = m_predefined.find(key); it != m_predefined.end())
    {
        return it->second.value;
    }
    m_predefined[key] = make_binding(key, Value{}, true);
    return m_predefined[key].value;
}

void ExtendedRuntimeState::add_message(std::string message)
{
    m_messages.push_back(std::move(message));
}

const std::vector<std::string> &ExtendedRuntimeState::messages() const
{
    return m_messages;
}

RuntimeBinding ExtendedRuntimeState::make_binding(std::string_view name, Value value, bool writable)
{
    return RuntimeBinding{
        std::string{name}, RuntimeLValue::variable(std::make_shared<Value>(std::move(value)), writable)};
}

std::string ExtendedRuntimeState::parameter_key(std::string_view name)
{
    return remove_prefix(name, '@');
}

std::string ExtendedRuntimeState::predefined_key(std::string_view name)
{
    return remove_prefix(name, '#');
}

const RuntimeBinding *ExtendedRuntimeState::find_binding(std::string_view name) const
{
    const std::string key{name};
    for (auto scope = m_local_scopes.rbegin(); scope != m_local_scopes.rend(); ++scope)
    {
        if (const auto it = scope->find(key); it != scope->end())
        {
            return &it->second;
        }
    }
    if (const auto it = m_formula_scope.find(key); it != m_formula_scope.end())
    {
        return &it->second;
    }
    return nullptr;
}

RuntimeBinding *ExtendedRuntimeState::find_binding(std::string_view name)
{
    const std::string key{name};
    for (auto scope = m_local_scopes.rbegin(); scope != m_local_scopes.rend(); ++scope)
    {
        if (const auto it = scope->find(key); it != scope->end())
        {
            return &it->second;
        }
    }
    if (const auto it = m_formula_scope.find(key); it != m_formula_scope.end())
    {
        return &it->second;
    }
    return nullptr;
}

} // namespace formula
