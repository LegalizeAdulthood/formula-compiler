// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/Value.h>

#include <gtest/gtest.h>

#include <stdexcept>

namespace formula::test
{

TEST(TestValue, defaultValueCreatesExpectedRuntimeValues)
{
    EXPECT_EQ(Value{false}, default_value(ValueKind::BOOL));
    EXPECT_EQ(Value{0}, default_value(ValueKind::INT));
    EXPECT_EQ(Value{0.0}, default_value(ValueKind::FLOAT));
    EXPECT_EQ((Value{Complex{0.0, 0.0}}), default_value(ValueKind::COMPLEX));
    EXPECT_EQ((Value{ColorValue{}}), default_value(ValueKind::COLOR));
    EXPECT_EQ(Value{std::string{}}, default_value(ValueKind::STRING));

    const Value array{default_value(ValueKind::ARRAY)};
    ASSERT_EQ(ValueKind::ARRAY, array.kind());
    ASSERT_TRUE(std::get<Value::ArrayPtr>(array.storage()));
    EXPECT_TRUE(std::get<Value::ArrayPtr>(array.storage())->elements.empty());

    const Value image{default_value(ValueKind::IMAGE)};
    ASSERT_EQ(ValueKind::IMAGE, image.kind());
    ASSERT_TRUE(std::get<Value::ImagePtr>(image.storage()));
    EXPECT_TRUE(std::get<Value::ImagePtr>(image.storage())->empty);
}

TEST(TestValue, convertsNumericValuesUpward)
{
    EXPECT_EQ(Value{1}, convert_value(Value{true}, ValueKind::INT));
    EXPECT_EQ(Value{1.0}, convert_value(Value{true}, ValueKind::FLOAT));
    EXPECT_EQ((Value{Complex{1.0, 0.0}}), convert_value(Value{true}, ValueKind::COMPLEX));
    EXPECT_EQ(Value{3.0}, convert_value(Value{3}, ValueKind::FLOAT));
    EXPECT_EQ((Value{Complex{3.0, 0.0}}), convert_value(Value{3}, ValueKind::COMPLEX));
    EXPECT_EQ((Value{Complex{1.5, 0.0}}), convert_value(Value{1.5}, ValueKind::COMPLEX));
    EXPECT_EQ(Value{true}, (convert_value(Value{Complex{0.0, 2.0}}, ValueKind::BOOL)));
    EXPECT_EQ(Value{false}, (convert_value(Value{Complex{0.0, 0.0}}, ValueKind::BOOL)));
}

TEST(TestValue, rejectsInvalidConversions)
{
    EXPECT_THROW(convert_value(Value{1.5}, ValueKind::INT), std::runtime_error);
    EXPECT_THROW((convert_value(Value{Complex{1.0, 0.0}}, ValueKind::FLOAT)), std::runtime_error);
    EXPECT_THROW((convert_value(Value{ColorValue{1.0, 0.0, 0.0, 1.0}}, ValueKind::COMPLEX)), std::runtime_error);
    EXPECT_THROW(convert_value(Value{std::string{"yes"}}, ValueKind::BOOL), std::runtime_error);
}

TEST(TestValue, truthinessUsesNumericValues)
{
    EXPECT_FALSE(is_truthy(Value{}));
    EXPECT_FALSE(is_truthy(Value{false}));
    EXPECT_TRUE(is_truthy(Value{true}));
    EXPECT_FALSE(is_truthy(Value{0}));
    EXPECT_TRUE(is_truthy(Value{2}));
    EXPECT_FALSE(is_truthy(Value{0.0}));
    EXPECT_TRUE(is_truthy(Value{0.25}));
    EXPECT_FALSE(is_truthy(Value{Complex{0.0, 0.0}}));
    EXPECT_TRUE(is_truthy(Value{Complex{0.0, 1.0}}));
    EXPECT_THROW(is_truthy(Value{ColorValue{0.0, 0.0, 0.0, 1.0}}), std::runtime_error);
}

TEST(TestValue, formatsValuesForRuntimeMessages)
{
    ArrayValue array;
    array.element_kind = ValueKind::INT;
    array.elements.push_back(Value{1});

    EXPECT_EQ("empty", format_value(Value{}));
    EXPECT_EQ("true", format_value(Value{true}));
    EXPECT_EQ("7", format_value(Value{7}));
    EXPECT_EQ("1.5", format_value(Value{1.5}));
    EXPECT_EQ("(1,2)", format_value(Value{Complex{1.0, 2.0}}));
    EXPECT_EQ("rgba(0.1,0.2,0.3,0.4)", format_value(Value{ColorValue{0.1, 0.2, 0.3, 0.4}}));
    EXPECT_EQ("hello", format_value(Value{std::string{"hello"}}));
    EXPECT_EQ("array[1]", format_value(make_array_value(array)));
    EXPECT_EQ("image(empty)", format_value(make_image_value(ImageValue{})));
    EXPECT_EQ("image(source)", format_value(make_image_value(ImageValue{"source", false})));
}

TEST(TestValue, comparesArrayAndImageValuesByContents)
{
    ArrayValue lhs;
    lhs.element_kind = ValueKind::INT;
    lhs.dimensions = {2};
    lhs.elements = {Value{1}, Value{2}};

    ArrayValue rhs{lhs};
    EXPECT_EQ(make_array_value(lhs), make_array_value(rhs));

    rhs.elements[1] = Value{3};
    EXPECT_NE(make_array_value(lhs), make_array_value(rhs));

    EXPECT_EQ(make_image_value(ImageValue{"source", false}), make_image_value(ImageValue{"source", false}));
    EXPECT_NE(make_image_value(ImageValue{"source", false}), make_image_value(ImageValue{"other", false}));
}

} // namespace formula::test
