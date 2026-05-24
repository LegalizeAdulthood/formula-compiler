// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace formula
{

enum class ValueKind
{
    EMPTY,
    BOOL,
    INT,
    FLOAT,
    COMPLEX,
    COLOR,
    STRING,
    ARRAY,
    IMAGE,
    ENUM,
};

struct ColorValue
{
    double red{};
    double green{};
    double blue{};
    double alpha{};
};

struct ArrayValue;
struct ImageValue;
struct EnumValue;

class Value
{
public:
    using ArrayPtr = std::shared_ptr<ArrayValue>;
    using ImagePtr = std::shared_ptr<ImageValue>;
    using EnumPtr = std::shared_ptr<EnumValue>;
    using Storage =
        std::variant<std::monostate, bool, int, double, Complex, ColorValue, std::string, ArrayPtr, ImagePtr, EnumPtr>;

    Value() = default;
    explicit Value(bool value);
    explicit Value(int value);
    explicit Value(double value);
    explicit Value(Complex value);
    explicit Value(ColorValue value);
    explicit Value(std::string value);
    explicit Value(ArrayPtr value);
    explicit Value(ImagePtr value);
    explicit Value(EnumPtr value);

    ValueKind kind() const;
    const Storage &storage() const;

private:
    Storage m_storage;
};

struct ArrayValue
{
    ValueKind element_kind{ValueKind::EMPTY};
    bool dynamic{};
    std::vector<int> dimensions;
    std::vector<Value> elements;
};

struct ImageValue
{
    std::string name;
    bool empty{true};
    int width{};
    int height{};
    std::vector<ColorValue> pixels;
};

struct EnumValue
{
    int index{};
    std::string label;
    std::vector<std::string> labels;
};

bool operator==(const ColorValue &lhs, const ColorValue &rhs);
bool operator!=(const ColorValue &lhs, const ColorValue &rhs);
bool operator==(const ArrayValue &lhs, const ArrayValue &rhs);
bool operator!=(const ArrayValue &lhs, const ArrayValue &rhs);
bool operator==(const ImageValue &lhs, const ImageValue &rhs);
bool operator!=(const ImageValue &lhs, const ImageValue &rhs);
bool operator==(const EnumValue &lhs, const EnumValue &rhs);
bool operator!=(const EnumValue &lhs, const EnumValue &rhs);
bool operator==(const Value &lhs, const Value &rhs);
bool operator!=(const Value &lhs, const Value &rhs);

std::string_view type_name(ValueKind kind);
std::string_view type_name(const Value &value);
bool is_numeric(ValueKind kind);
ValueKind promote_numeric_kind(ValueKind lhs, ValueKind rhs);
Value default_value(ValueKind kind);
Value make_array_value(ArrayValue value);
Value make_image_value(ImageValue value);
Value make_enum_value(EnumValue value);
Value convert_value(const Value &value, ValueKind target);
bool is_truthy(const Value &value);
std::string format_value(const Value &value);

} // namespace formula
