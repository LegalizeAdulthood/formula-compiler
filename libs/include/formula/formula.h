#pragma once

#include <memory>
#include <string_view>

namespace formula
{

struct Complex
{
    double re;
    double im;
};

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
