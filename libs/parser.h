#pragma once

#include <formula/Node.h>

#include <string_view>

namespace formula
{

ast::FormulaSectionsPtr parse(std::string_view text);

} // namespace formula
