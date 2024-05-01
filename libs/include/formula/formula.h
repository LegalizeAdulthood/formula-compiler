#pragma once

#include <memory>
#include <string_view>

namespace formula
{

class Node;

class Formula
{
public:
    virtual ~Formula() = default;

    virtual void set_value(std::string_view name, double value) = 0;

    virtual double interpret() = 0;
    virtual bool compile() = 0;
    virtual double run() = 0;
};

std::shared_ptr<Formula> parse(std::string_view text);

}
