// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <cmath>
#include <iostream>

namespace formula
{

struct Complex
{
    double re;
    double im;
};

inline bool operator==(const Complex &lhs, const Complex &rhs)
{
    return lhs.re == rhs.re && lhs.im == rhs.im;
}
inline bool operator!=(const Complex &lhs, const Complex &rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(const Complex &lhs, const Complex &rhs)
{
    return lhs.re < rhs.re || (lhs.re == rhs.re && lhs.im < rhs.im);
}

inline std::ostream &operator<<(std::ostream &str, const Complex &value)
{
    return str << '(' << value.re << ',' << value.im << ')';
}

inline Complex operator+(const Complex &lhs, const Complex &rhs)
{
    return {lhs.re + rhs.re, lhs.im + rhs.im};
}

inline Complex &operator+=(Complex &lhs, const Complex &rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline Complex operator-(const Complex &lhs, const Complex &rhs)
{
    return {lhs.re - rhs.re, lhs.im - rhs.im};
}

inline Complex &operator-=(Complex &lhs, const Complex &rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

inline Complex operator*(const Complex &lhs, const Complex &rhs)
{
    return {lhs.re * rhs.re - lhs.im * rhs.im, lhs.re * rhs.im + lhs.im * rhs.re};
}

inline Complex &operator*=(Complex &lhs, const Complex &rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

inline Complex operator/(const Complex &lhs, const Complex &rhs)
{
    const double denom = rhs.re * rhs.re + rhs.im * rhs.im;
    return {(lhs.re * rhs.re + lhs.im * rhs.im) / denom, (lhs.im * rhs.re - lhs.re * rhs.im) / denom};
}

inline Complex &operator/=(Complex &lhs, const Complex &rhs)
{
    lhs = lhs / rhs;
    return lhs;
}

inline Complex abs(const Complex &value)
{
    return {std::abs(value.re), std::abs(value.im)};
}

Complex exp(const Complex &z);
Complex log(const Complex &z);
Complex pow(const Complex &z, const Complex &w);

} // namespace formula
