#include <formula/Complex.h>

#include <cmath>

namespace formula
{

Complex exp(const Complex &z)
{
    const double exp_re = std::exp(z.re);
    return {exp_re * std::cos(z.im), exp_re * std::sin(z.im)};
}

Complex log(const Complex &z)
{
    const double magnitude = std::sqrt(z.re * z.re + z.im * z.im);
    // Treat -0.0 as +0.0 for imaginary part
    double im = (z.im == 0.0) ? 0.0 : z.im;
    const double phase = std::atan2(im, z.re);
    return {std::log(magnitude), phase};
}

Complex pow(const Complex &z, const Complex &w)
{
    // Handle special case: 0^0 = 1 by convention
    if (z == Complex{0.0, 0.0} && w == Complex{0.0, 0.0})
    {
        return {1.0, 0.0};
    }

    // Handle special case: 0^w = 0 for any w != 0
    if (z == Complex{0.0, 0.0})
    {
        return {0.0, 0.0};
    }

    // General case: z^w = exp(w * log(z))
    return exp(w * log(z));
}

} // namespace formula
