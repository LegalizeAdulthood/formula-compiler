// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "functions.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <variant>

namespace formula
{

namespace
{

struct FunctionMap
{
    std::string_view name;
    RealFunction *fn{};
    ComplexFunction *cmplx{};
};

bool operator<(const FunctionMap &entry, const std::string_view &name)
{
    return entry.name < name;
}

double cabs(double arg)
{
    return std::abs(arg);
}

double cosxx(double arg)
{
    // TODO: this is bogus and needs to be corrected for complex argument z=x+iy
    // cos(x)cosh(y) + isin(x)sinh(y)
    return std::cos(arg) * std::cosh(arg);
}

double conj(double arg)
{
    return -arg;
}

Complex conj(const Complex &arg)
{
    return {arg.re, -arg.im};
}

double cotan(double arg)
{
    return std::cos(arg) / std::sin(arg);
}

double cotanh(double arg)
{
    return std::cosh(arg) / std::sinh(arg);
}

double flip(double arg)
{
    return -arg;
}

Complex flip(const Complex &arg)
{
    return {arg.im, arg.re};
}

double fn1(double arg)
{
    return arg;
}

double fn2(double arg)
{
    return arg;
}

double fn3(double arg)
{
    return arg;
}

double fn4(double arg)
{
    return arg;
}

double ident(double arg)
{
    return arg;
}

Complex ident(const Complex &arg)
{
    return arg;
}

double imag(double arg)
{
    return -arg;
}

double one(double /*arg*/)
{
    return 1.0;
}

Complex one(const Complex & /*arg*/)
{
    return {1.0, 0.0};
}

double real(double arg)
{
    return arg;
}

double sqr(double arg)
{
    return arg * arg;
}

double zero(double /*arg*/)
{
    return 0.0;
}

Complex zero(const Complex & /*arg*/)
{
    return {0.0, 0.0};
}

double set_rand(double arg)
{
    std::srand(static_cast<int>(arg));
    return 0.0;
}

const FunctionMap s_standard_functions[]{
    {"abs", std::abs, abs},  //
    {"acos", std::acos},     //
    {"acosh", std::acosh},   //
    {"asin", std::asin},     //
    {"asinh", std::asinh},   //
    {"atan", std::atan},     //
    {"atanh", std::atanh},   //
    {"cabs", cabs},          //
    {"ceil", std::ceil},     //
    {"conj", conj, conj},    //
    {"cos", std::cos},       //
    {"cosh", std::cosh},     //
    {"cosxx", cosxx},        //
    {"cotan", cotan},        //
    {"cotanh", cotanh},      //
    {"exp", std::exp},       //
    {"flip", flip, flip},    //
    {"floor", std::floor},   //
    {"fn1", fn1},            //
    {"fn2", fn2},            //
    {"fn3", fn3},            //
    {"fn4", fn4},            //
    {"ident", ident, ident}, //
    {"imag", imag},          //
    {"log", std::log},       //
    {"one", one, one},       //
    {"real", real},          //
    {"round", std::round},   //
    {"sin", std::sin},       //
    {"sinh", std::sinh},     //
    {"sqr", sqr},            //
    {"sqrt", std::sqrt},     //
    {"srand", set_rand},     //
    {"tan", std::tan},       //
    {"tanh", std::tanh},     //
    {"trunc", std::trunc},   //
    {"zero", zero, zero},    //
};

} // namespace

RealFunction *lookup_real(std::string_view name)
{
    const auto it = std::lower_bound(std::begin(s_standard_functions), std::end(s_standard_functions), name);
    if (it != std::end(s_standard_functions) && it->name == name)
    {
        return it->fn;
    }
    return nullptr;
}

ComplexFunction *lookup_complex(std::string_view name)
{
    const auto it = std::lower_bound(std::begin(s_standard_functions), std::end(s_standard_functions), name);
    if (it != std::end(s_standard_functions) && it->name == name)
    {
        return it->cmplx;
    }
    return nullptr;
}

double evaluate(std::string_view name, double value)
{
    if (const auto fn = lookup_real(name))
    {
        return (*fn)(value);
    }
    throw std::runtime_error("function '" + std::string(name) + "' not found");
}

Complex evaluate(std::string_view name, Complex value)
{
    if (const auto fn = lookup_complex(name))
    {
        return fn(value);
    }
    if (const auto fn = lookup_real(name))
    {
        return {fn(value.re), 0.0};
    }
    throw std::runtime_error("function '" + std::string(name) + "' not found");
}

} // namespace formula
