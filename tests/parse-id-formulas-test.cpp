// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include <formula/ParseOptions.h>

#include "formula-data.h"

#include <gtest/gtest.h>

using namespace formula;
using namespace formula::parser;
using namespace testing;

namespace formula
{

inline void PrintTo(const FormulaEntry &entry, std::ostream *os)
{
    *os << entry.file_entry.name;
}

} // namespace formula

namespace formula::test
{

class ParseIdFormulaSuite : public TestWithParam<FormulaEntry>
{
};

static std::string prepare_content(std::string_view text)
{
    std::string content{text};
    auto brace = content.find_first_of('{');
    if (brace != std::string::npos)
    {
        content.erase(0, brace + 1);
    }
    brace = content.find_last_of('}');
    if (brace != std::string::npos)
    {
        content.erase(brace, std::string::npos);
    }
    return content;
}

TEST_P(ParseIdFormulaSuite, parsed)
{
    const FormulaEntry &entry{GetParam()};
    Options options;
    if (entry.is_class)
    {
        options.entry_kind = EntryKind::CLASS;
    }

    FormulaPtr result{create_formula(prepare_content(entry.file_entry.body), options)};

    EXPECT_TRUE(result) << entry.file_entry.name;
}

INSTANTIATE_TEST_SUITE_P(TestIdFormula, ParseIdFormulaSuite, ValuesIn(g_formulas, g_formulas + g_formula_count));

} // namespace formula::test
