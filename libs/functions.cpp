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

Complex cabs(const Complex &arg)
{
    return {std::sqrt(arg.re * arg.re + arg.im * arg.im), 0.0};
}

Complex sqr(const Complex &arg)
{
    return arg * arg;
}

Complex cos(const Complex &arg)
{
    return {std::cos(arg.re) * std::cosh(arg.im), -std::sin(arg.re) * std::sinh(arg.im)};
}

Complex sin(const Complex &arg)
{
    return {std::sin(arg.re) * std::cosh(arg.im), std::cos(arg.re) * std::sinh(arg.im)};
}

Complex cosh(const Complex &arg)
{
    return {std::cosh(arg.re) * std::cos(arg.im), std::sinh(arg.re) * std::sin(arg.im)};
}

Complex sinh(const Complex &arg)
{
    return {std::sinh(arg.re) * std::cos(arg.im), std::cosh(arg.re) * std::sin(arg.im)};
}

Complex sqrt(const Complex &arg)
{
    const double magnitude = std::sqrt(arg.re * arg.re + arg.im * arg.im);
    const double phase = std::atan2(arg.im, arg.re);
    const double sqrt_mag = std::sqrt(magnitude);
    return {sqrt_mag * std::cos(phase / 2.0), sqrt_mag * std::sin(phase / 2.0)};
}

Complex tan(const Complex &arg)
{
    return sin(arg) / cos(arg);
}

Complex tanh(const Complex &arg)
{
    return sinh(arg) / cosh(arg);
}

Complex cotan(const Complex &arg)
{
    return cos(arg) / sin(arg);
}

Complex cotanh(const Complex &arg)
{
    return cosh(arg) / sinh(arg);
}

Complex asin(const Complex &arg)
{
    const Complex i{0.0, 1.0};
    const Complex one_c{1.0, 0.0};
    const Complex arg_sqr = arg * arg;
    const Complex inner = sqrt(one_c - arg_sqr);
    const Complex logged = log(i * arg + inner);
    return {logged.im, -logged.re};
}

Complex acos(const Complex &arg)
{
    const Complex i{0.0, 1.0};
    const Complex one_c{1.0, 0.0};
    const Complex arg_sqr = arg * arg;
    const Complex inner = sqrt(one_c - arg_sqr);
    const Complex logged = log(arg + i * inner);
    return {logged.im, -logged.re};
}

Complex atan(const Complex &arg)
{
    const Complex i{0.0, 1.0};
    const Complex one_c{1.0, 0.0};
    const Complex numerator = log(one_c - i * arg);
    const Complex denominator = log(one_c + i * arg);
    const Complex result = (numerator - denominator) * Complex{0.0, 0.5};
    return result;
}

Complex asinh(const Complex &arg)
{
    const Complex one_c{1.0, 0.0};
    const Complex arg_sqr = arg * arg;
    return log(arg + sqrt(arg_sqr + one_c));
}

Complex acosh(const Complex &arg)
{
    const Complex one_c{1.0, 0.0};
    const Complex arg_sqr = arg * arg;
    return log(arg + sqrt(arg_sqr - one_c));
}

Complex atanh(const Complex &arg)
{
    const Complex one_c{1.0, 0.0};
    const Complex numerator = log(one_c + arg);
    const Complex denominator = log(one_c - arg);
    return (numerator - denominator) * Complex{0.5, 0.0};
}

Complex real(const Complex &arg)
{
    return {arg.re, 0.0};
}

Complex imag(const Complex &arg)
{
    return {arg.im, 0.0};
}

Complex fn1(const Complex &arg)
{
    return arg;
}

Complex fn2(const Complex &arg)
{
    return arg;
}

Complex fn3(const Complex &arg)
{
    return arg;
}

Complex fn4(const Complex &arg)
{
    return arg;
}

Complex cosxx(const Complex &arg)
{
    return cos(arg) * cosh(arg);
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
    return 0.0;
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
    {"abs", std::abs, abs},       //
    {"acos", std::acos, acos},    //
    {"acosh", std::acosh, acosh}, //
    {"asin", std::asin, asin},    //
    {"asinh", std::asinh, asinh}, //
    {"atan", std::atan, atan},    //
    {"atanh", std::atanh, atanh}, //
    {"cabs", cabs, cabs},         //
    {"ceil", std::ceil},          //
    {"conj", conj, conj},         //
    {"cos", std::cos, cos},       //
    {"cosh", std::cosh, cosh},    //
    {"cosxx", cosxx, cosxx},      //
    {"cotan", cotan, cotan},      //
    {"cotanh", cotanh, cotanh},   //
    {"exp", std::exp, exp},       //
    {"flip", flip, flip},         //
    {"floor", std::floor},        //
    {"fn1", fn1, fn1},            //
    {"fn2", fn2, fn2},            //
    {"fn3", fn3, fn3},            //
    {"fn4", fn4, fn4},            //
    {"ident", ident, ident},      //
    {"imag", imag, imag},         //
    {"log", std::log, log},       //
    {"one", one, one},            //
    {"real", real, real},         //
    {"round", std::round},        //
    {"sin", std::sin, sin},       //
    {"sinh", std::sinh, sinh},    //
    {"sqr", sqr, sqr},            //
    {"sqrt", std::sqrt, sqrt},    //
    {"srand", set_rand},          //
    {"tan", std::tan, tan},       //
    {"tanh", std::tanh, tanh},    //
    {"trunc", std::trunc},        //
    {"zero", zero, zero},         //
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
