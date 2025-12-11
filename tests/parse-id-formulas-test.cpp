// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include "formula-data.h"

#include <gtest/gtest.h>

using namespace formula;
using namespace testing;

namespace formula::test
{
inline void PrintTo(const FormulaEntry &entry, std::ostream *os)
{
    *os << entry.name;
}

class ParseIdFormulaSuite : public TestWithParam<FormulaEntry>
{
};

static std::string prepare_content(std::string_view text)
{
    std::string content{text};
    auto brace = content.find_first_of('{');
    content.erase(0, brace + 1);
    brace = content.find_last_of('}');
    content.erase(brace, std::string::npos);
    return content;
}

TEST_P(ParseIdFormulaSuite, parsed)
{
    const FormulaEntry &entry{GetParam()};

    FormulaPtr result{create_formula(prepare_content(entry.content))};

    EXPECT_TRUE(result) << entry.name;
}

INSTANTIATE_TEST_SUITE_P(TestIdFormula, ParseIdFormulaSuite, ValuesIn(formulas, formulas + formula_count));

} // namespace formula::test
