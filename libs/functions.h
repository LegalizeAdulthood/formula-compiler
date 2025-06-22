#pragma once

#include <formula/Complex.h>

#include <string_view>

namespace formula
{

using RealFunction = double(double);
using ComplexFunction = Complex(const Complex &);

double evaluate(std::string_view name, double value);
Complex evaluate(std::string_view name, Complex value);

RealFunction *lookup_real(std::string_view name);
ComplexFunction *lookup_complex(std::string_view name);

} // namespace formula
