#pragma once

#include <iostream>
#include <memory>
#include <string_view>

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

inline std::ostream &operator<<(std::ostream &str, const Complex &value)
{
    return str << '(' << value.re << ',' << value.im << ')';
}

inline Complex operator+(const Complex &lhs, const Complex &rhs)
{
    return {lhs.re + rhs.re, lhs.im + rhs.im};
}

inline Complex operator-(const Complex &lhs, const Complex &rhs)
{
    return {lhs.re - rhs.re, lhs.im - rhs.im};
}

inline Complex operator*(const Complex &lhs, const Complex &rhs)
{
    return {lhs.re * rhs.re - lhs.im * rhs.im, lhs.re * rhs.im + lhs.im * rhs.re};
}

inline Complex operator/(const Complex &lhs, const Complex &rhs)
{
    const double denom = rhs.re * rhs.re + rhs.im * rhs.im;
    return {(lhs.re * rhs.re + lhs.im * rhs.im) / denom,
            (lhs.im * rhs.re - lhs.re * rhs.im) / denom};
}

enum Part
{
    INITIALIZE = 0,
    ITERATE = 1,
    BAILOUT = 2
};

class Formula
{
public:
    virtual ~Formula() = default;

    virtual void set_value(std::string_view name, Complex value) = 0;
    virtual Complex get_value(std::string_view name) const = 0;

    virtual Complex interpret(Part part) = 0;
    virtual bool compile() = 0;
    virtual Complex run(Part part) = 0;
};

std::shared_ptr<Formula> parse(std::string_view text);

}
