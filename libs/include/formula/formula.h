#pragma once

#include <memory>
#include <string_view>

namespace formula
{

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

    virtual void set_value(std::string_view name, double value) = 0;
    virtual double get_value(std::string_view name) const = 0;

    virtual double interpret(Part part) = 0;
    virtual bool compile() = 0;
    virtual double run(Part part) = 0;
};

std::shared_ptr<Formula> parse(std::string_view text);

}
