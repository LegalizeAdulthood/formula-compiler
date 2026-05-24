// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Value.h>

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace formula
{

namespace
{

int numeric_rank(ValueKind kind)
{
    switch (kind)
    {
    case ValueKind::BOOL:
        return 0;
    case ValueKind::INT:
        return 1;
    case ValueKind::FLOAT:
        return 2;
    case ValueKind::COMPLEX:
    case ValueKind::ENUM:
        return 3;
    default:
        throw std::runtime_error("non-numeric value kind: " + std::string{type_name(kind)});
    }
}

double numeric_real_part(const Value &value)
{
    switch (value.kind())
    {
    case ValueKind::BOOL:
        return std::get<bool>(value.storage()) ? 1.0 : 0.0;
    case ValueKind::INT:
        return static_cast<double>(std::get<int>(value.storage()));
    case ValueKind::FLOAT:
        return std::get<double>(value.storage());
    case ValueKind::COMPLEX:
        return std::get<Complex>(value.storage()).re;
    case ValueKind::ENUM:
    {
        const Value::EnumPtr enum_value{std::get<Value::EnumPtr>(value.storage())};
        return enum_value ? static_cast<double>(enum_value->index) : 0.0;
    }
    default:
        throw std::runtime_error("cannot read numeric value from " + std::string{type_name(value)});
    }
}

Complex numeric_complex_value(const Value &value)
{
    if (value.kind() == ValueKind::COMPLEX)
    {
        return std::get<Complex>(value.storage());
    }
    return {numeric_real_part(value), 0.0};
}

std::runtime_error conversion_error(const Value &value, ValueKind target)
{
    return std::runtime_error(
        "cannot convert " + std::string{type_name(value)} + " to " + std::string{type_name(target)});
}

template <typename Ptr>
bool ptr_equal(const Ptr &lhs, const Ptr &rhs)
{
    if (!lhs || !rhs)
    {
        return lhs == rhs;
    }
    return *lhs == *rhs;
}

} // namespace

Value::Value(bool value) :
    m_storage(value)
{
}

Value::Value(int value) :
    m_storage(value)
{
}

Value::Value(double value) :
    m_storage(value)
{
}

Value::Value(Complex value) :
    m_storage(value)
{
}

Value::Value(ColorValue value) :
    m_storage(value)
{
}

Value::Value(std::string value) :
    m_storage(std::move(value))
{
}

Value::Value(ArrayPtr value) :
    m_storage(std::move(value))
{
}

Value::Value(ImagePtr value) :
    m_storage(std::move(value))
{
}

Value::Value(EnumPtr value) :
    m_storage(std::move(value))
{
}

Value::Value(PluginPtr value) :
    m_storage(std::move(value))
{
}

ValueKind Value::kind() const
{
    switch (m_storage.index())
    {
    case 0:
        return ValueKind::EMPTY;
    case 1:
        return ValueKind::BOOL;
    case 2:
        return ValueKind::INT;
    case 3:
        return ValueKind::FLOAT;
    case 4:
        return ValueKind::COMPLEX;
    case 5:
        return ValueKind::COLOR;
    case 6:
        return ValueKind::STRING;
    case 7:
        return ValueKind::ARRAY;
    case 8:
        return ValueKind::IMAGE;
    case 9:
        return ValueKind::ENUM;
    case 10:
        return ValueKind::PLUGIN;
    default:
        throw std::runtime_error("value variant index out of range");
    }
}

const Value::Storage &Value::storage() const
{
    return m_storage;
}

bool operator==(const ColorValue &lhs, const ColorValue &rhs)
{
    return lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue && lhs.alpha == rhs.alpha;
}

bool operator!=(const ColorValue &lhs, const ColorValue &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const ArrayValue &lhs, const ArrayValue &rhs)
{
    return lhs.element_kind == rhs.element_kind && lhs.dynamic == rhs.dynamic && lhs.dimensions == rhs.dimensions &&
        lhs.elements == rhs.elements;
}

bool operator!=(const ArrayValue &lhs, const ArrayValue &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const ImageValue &lhs, const ImageValue &rhs)
{
    return lhs.name == rhs.name && lhs.empty == rhs.empty && lhs.width == rhs.width && lhs.height == rhs.height &&
        lhs.pixels == rhs.pixels;
}

bool operator!=(const ImageValue &lhs, const ImageValue &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const EnumValue &lhs, const EnumValue &rhs)
{
    return lhs.index == rhs.index && lhs.label == rhs.label && lhs.labels == rhs.labels;
}

bool operator!=(const EnumValue &lhs, const EnumValue &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const PluginValue &lhs, const PluginValue &rhs)
{
    return lhs.filename == rhs.filename && lhs.class_name == rhs.class_name && lhs.base_class == rhs.base_class &&
        lhs.ast == rhs.ast && lhs.nested_values == rhs.nested_values && lhs.object_fields == rhs.object_fields &&
        lhs.object_initialized == rhs.object_initialized;
}

bool operator!=(const PluginValue &lhs, const PluginValue &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Value &lhs, const Value &rhs)
{
    if (lhs.kind() != rhs.kind())
    {
        return false;
    }
    switch (lhs.kind())
    {
    case ValueKind::EMPTY:
        return true;
    case ValueKind::BOOL:
        return std::get<bool>(lhs.storage()) == std::get<bool>(rhs.storage());
    case ValueKind::INT:
        return std::get<int>(lhs.storage()) == std::get<int>(rhs.storage());
    case ValueKind::FLOAT:
        return std::get<double>(lhs.storage()) == std::get<double>(rhs.storage());
    case ValueKind::COMPLEX:
        return std::get<Complex>(lhs.storage()) == std::get<Complex>(rhs.storage());
    case ValueKind::COLOR:
        return std::get<ColorValue>(lhs.storage()) == std::get<ColorValue>(rhs.storage());
    case ValueKind::STRING:
        return std::get<std::string>(lhs.storage()) == std::get<std::string>(rhs.storage());
    case ValueKind::ARRAY:
        return ptr_equal(std::get<Value::ArrayPtr>(lhs.storage()), std::get<Value::ArrayPtr>(rhs.storage()));
    case ValueKind::IMAGE:
        return ptr_equal(std::get<Value::ImagePtr>(lhs.storage()), std::get<Value::ImagePtr>(rhs.storage()));
    case ValueKind::ENUM:
        return ptr_equal(std::get<Value::EnumPtr>(lhs.storage()), std::get<Value::EnumPtr>(rhs.storage()));
    case ValueKind::PLUGIN:
        return ptr_equal(std::get<Value::PluginPtr>(lhs.storage()), std::get<Value::PluginPtr>(rhs.storage()));
    }
    throw std::runtime_error("unknown value kind");
}

bool operator!=(const Value &lhs, const Value &rhs)
{
    return !(lhs == rhs);
}

std::string_view type_name(ValueKind kind)
{
    switch (kind)
    {
    case ValueKind::EMPTY:
        return "empty";
    case ValueKind::BOOL:
        return "bool";
    case ValueKind::INT:
        return "int";
    case ValueKind::FLOAT:
        return "float";
    case ValueKind::COMPLEX:
        return "complex";
    case ValueKind::COLOR:
        return "color";
    case ValueKind::STRING:
        return "string";
    case ValueKind::ARRAY:
        return "array";
    case ValueKind::IMAGE:
        return "image";
    case ValueKind::ENUM:
        return "enum";
    case ValueKind::PLUGIN:
        return "plugin";
    }
    throw std::runtime_error("unknown value kind");
}

std::string_view type_name(const Value &value)
{
    return type_name(value.kind());
}

bool is_numeric(ValueKind kind)
{
    return kind == ValueKind::BOOL || kind == ValueKind::INT || kind == ValueKind::FLOAT ||
        kind == ValueKind::COMPLEX || kind == ValueKind::ENUM;
}

ValueKind promote_numeric_kind(ValueKind lhs, ValueKind rhs)
{
    if (!is_numeric(lhs) || !is_numeric(rhs))
    {
        throw std::runtime_error("cannot promote non-numeric value kinds");
    }
    return numeric_rank(lhs) > numeric_rank(rhs) ? lhs : rhs;
}

Value default_value(ValueKind kind)
{
    switch (kind)
    {
    case ValueKind::EMPTY:
        return Value{};
    case ValueKind::BOOL:
        return Value{false};
    case ValueKind::INT:
        return Value{0};
    case ValueKind::FLOAT:
        return Value{0.0};
    case ValueKind::COMPLEX:
        return Value{Complex{0.0, 0.0}};
    case ValueKind::COLOR:
        return Value{ColorValue{}};
    case ValueKind::STRING:
        return Value{std::string{}};
    case ValueKind::ARRAY:
        return make_array_value(ArrayValue{});
    case ValueKind::IMAGE:
        return make_image_value(ImageValue{});
    case ValueKind::ENUM:
        return make_enum_value(EnumValue{});
    case ValueKind::PLUGIN:
        return make_plugin_value(PluginValue{});
    }
    throw std::runtime_error("unknown value kind");
}

Value make_array_value(ArrayValue value)
{
    return Value{std::make_shared<ArrayValue>(std::move(value))};
}

Value make_image_value(ImageValue value)
{
    return Value{std::make_shared<ImageValue>(std::move(value))};
}

Value make_enum_value(EnumValue value)
{
    return Value{std::make_shared<EnumValue>(std::move(value))};
}

Value make_plugin_value(PluginValue value)
{
    return Value{std::make_shared<PluginValue>(std::move(value))};
}

Value convert_value(const Value &value, ValueKind target)
{
    if (value.kind() == target)
    {
        return value;
    }
    if (target == ValueKind::BOOL && is_numeric(value.kind()))
    {
        return Value{is_truthy(value)};
    }
    if (value.kind() == ValueKind::ENUM && is_numeric(target))
    {
        switch (target)
        {
        case ValueKind::INT:
            return Value{static_cast<int>(numeric_real_part(value))};
        case ValueKind::FLOAT:
            return Value{numeric_real_part(value)};
        case ValueKind::COMPLEX:
            return Value{numeric_complex_value(value)};
        default:
            break;
        }
    }
    if (is_numeric(value.kind()) && is_numeric(target) && numeric_rank(value.kind()) <= numeric_rank(target))
    {
        switch (target)
        {
        case ValueKind::INT:
            return Value{static_cast<int>(numeric_real_part(value))};
        case ValueKind::FLOAT:
            return Value{numeric_real_part(value)};
        case ValueKind::COMPLEX:
            return Value{numeric_complex_value(value)};
        default:
            break;
        }
    }
    throw conversion_error(value, target);
}

bool is_truthy(const Value &value)
{
    switch (value.kind())
    {
    case ValueKind::EMPTY:
        return false;
    case ValueKind::BOOL:
        return std::get<bool>(value.storage());
    case ValueKind::INT:
        return std::get<int>(value.storage()) != 0;
    case ValueKind::FLOAT:
        return std::get<double>(value.storage()) != 0.0;
    case ValueKind::COMPLEX:
    {
        const Complex complex{std::get<Complex>(value.storage())};
        return complex.re != 0.0 || complex.im != 0.0;
    }
    case ValueKind::ENUM:
    {
        const Value::EnumPtr enum_value{std::get<Value::EnumPtr>(value.storage())};
        return enum_value && enum_value->index != 0;
    }
    default:
        throw std::runtime_error("cannot test truthiness of " + std::string{type_name(value)});
    }
}

std::string format_value(const Value &value)
{
    std::ostringstream out;
    switch (value.kind())
    {
    case ValueKind::EMPTY:
        out << "empty";
        break;
    case ValueKind::BOOL:
        out << (std::get<bool>(value.storage()) ? "true" : "false");
        break;
    case ValueKind::INT:
        out << std::get<int>(value.storage());
        break;
    case ValueKind::FLOAT:
        out << std::get<double>(value.storage());
        break;
    case ValueKind::COMPLEX:
        out << std::get<Complex>(value.storage());
        break;
    case ValueKind::COLOR:
    {
        const ColorValue color{std::get<ColorValue>(value.storage())};
        out << "rgba(" << color.red << ',' << color.green << ',' << color.blue << ',' << color.alpha << ')';
        break;
    }
    case ValueKind::STRING:
        out << std::get<std::string>(value.storage());
        break;
    case ValueKind::ARRAY:
    {
        const Value::ArrayPtr array{std::get<Value::ArrayPtr>(value.storage())};
        out << "array[" << (array ? array->elements.size() : 0) << ']';
        break;
    }
    case ValueKind::IMAGE:
    {
        const Value::ImagePtr image{std::get<Value::ImagePtr>(value.storage())};
        out << "image(" << (!image || image->empty ? "empty" : image->name) << ')';
        break;
    }
    case ValueKind::ENUM:
    {
        const Value::EnumPtr enum_value{std::get<Value::EnumPtr>(value.storage())};
        out << "enum(" << (!enum_value ? 0 : enum_value->index) << ')';
        break;
    }
    case ValueKind::PLUGIN:
    {
        const Value::PluginPtr plugin{std::get<Value::PluginPtr>(value.storage())};
        out << "plugin(";
        if (plugin)
        {
            if (!plugin->filename.empty())
            {
                out << plugin->filename << ':';
            }
            out << plugin->class_name;
        }
        out << ')';
        break;
    }
    }
    return out.str();
}

} // namespace formula
