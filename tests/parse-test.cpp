// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/Parser.h>

#include <formula/Formula.h>
#include <formula/ParseOptions.h>

#include "ExpressionParam.h"
#include "NodeFormatter.h"
#include "trim_ws.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

using namespace formula::parser;
using namespace testing;

namespace formula::parser
{

std::ostream &operator<<(std::ostream &os, ErrorCode code)
{
    return os << to_string(code);
}

} // namespace formula::parser

namespace formula::test
{

namespace
{

struct SimpleExpressionParam
{
    std::string_view name;
    std::string_view text;
    std::string_view expected;
};

inline void PrintTo(const SimpleExpressionParam &param, std::ostream *os)
{
    *os << param.name;
}

class SimpleExpressions : public TestWithParam<SimpleExpressionParam>
{
};

struct MultiStatementParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const MultiStatementParam &param, std::ostream *os)
{
    *os << param.name;
}

class MultiStatements : public TestWithParam<MultiStatementParam>
{
};

class BuiltinVariables : public TestWithParam<std::string>
{
};

class ReservedWords : public TestWithParam<std::string>
{
};

class Functions : public TestWithParam<std::string>
{
};

struct ParseFailureParam
{
    std::string_view name;
    std::string_view text;
    ErrorCode expected_error{};
};

inline void PrintTo(const ParseFailureParam &param, std::ostream *os)
{
    *os << param.name;
}

class ParseFailures : public TestWithParam<ParseFailureParam>
{
};

struct ParseParam
{
    std::string_view name;
    std::string_view text;
    Section section;
};

inline void PrintTo(const ParseParam &param, std::ostream *os)
{
    *os << param.name;
}

class SingleSections : public TestWithParam<ParseParam>
{
};

class DefaultSection : public TestWithParam<SimpleExpressionParam>
{
};

struct InvalidSectionParam
{
    std::string_view name;
    std::string_view text;
    ErrorCode expected_error{};
};

inline void PrintTo(const InvalidSectionParam &param, std::ostream *os)
{
    *os << param.name;
}

struct BuiltinDisallowsParam
{
    std::string_view name;
    std::string_view text;
};

inline void PrintTo(const BuiltinDisallowsParam &param, std::ostream *os)
{
    *os << param.name;
}

class BuiltinDisallows : public TestWithParam<BuiltinDisallowsParam>
{
};

} // namespace

static SimpleExpressionParam s_simple_expressions[]{
    {"intLiteral", "1", "literal:1"},                                                                               //
    {"floatLiteral", "1.5", "literal:1.5"},                                                                         //
    {"complexLiteral", "(1,2)", "literal:(1,2)"},                                                                   //
    {"identifier", "z2", "identifier:z2"},                                                                          //
    {"parenExpr", "(z)", "identifier:z"},                                                                           //
    {"add", "1+2", "binary_op:+ literal:1 literal:2"},                                                              //
    {"subtract", "1-2", "binary_op:- literal:1 literal:2"},                                                         //
    {"multiply", "1*2", "binary_op:* literal:1 literal:2"},                                                         //
    {"divide", "1/2", "binary_op:/ literal:1 literal:2"},                                                           //
    {"multiplyAdd", "1*2+4", "binary_op:+ binary_op:* literal:1 literal:2 literal:4"},                              //
    {"parenthesisExpr", "1*(2+4)", "binary_op:* literal:1 binary_op:+ literal:2 literal:4"},                        //
    {"unaryMinus", "-(1)", "unary_op:- literal:1"},                                                                 //
    {"unaryPlus", "+(1)", "unary_op:+ literal:1"},                                                                  //
    {"unaryMinusNegativeOne", "--1", "unary_op:- unary_op:- literal:1"},                                            //
    {"addAddAdd", "1+2+3", "binary_op:+ binary_op:+ literal:1 literal:2 literal:3"},                                //
    {"capitalLetterInIdentifier", "A", "identifier:a"},                                                             //
    {"numberInIdentifier", "a1", "identifier:a1"},                                                                  //
    {"underscoreInIdentifier", "A_1", "identifier:a_1"},                                                            //
    {"power", "2^3", "binary_op:^ literal:2 literal:3"},                                                            //
    {"powerLeftAssociative", "1^2^3", "binary_op:^ binary_op:^ literal:1 literal:2 literal:3"},                     //
    {"assignment", "z=4", "assignment:z literal:4"},                                                                //
    {"assignmentLongVariable", "this_is_another4_variable_name2=4",                                                 //
        "assignment:this_is_another4_variable_name2 literal:4"},                                                    //
    {"modulus", "|-3.0|", "unary_op:| unary_op:- literal:3"},                                                       //
    {"compareLess", "4<3", "binary_op:< literal:4 literal:3"},                                                      //
    {"compareLessEqual", "4<=3", "binary_op:<= literal:4 literal:3"},                                               //
    {"compareGreater", "4>3", "binary_op:> literal:4 literal:3"},                                                   //
    {"compareAssociatesLeft", "4>3<4", "binary_op:< binary_op:> literal:4 literal:3 literal:4"},                    //
    {"compareGreaterEqual", "4>=3", "binary_op:>= literal:4 literal:3"},                                            //
    {"compareEqual", "4==3", "binary_op:== literal:4 literal:3"},                                                   //
    {"compareNotEqual", "4!=3", "binary_op:!= literal:4 literal:3"},                                                //
    {"logicalAnd", "4==3&&5==6", "binary_op:&& binary_op:== literal:4 literal:3 binary_op:== literal:5 literal:6"}, //
    {"logicalOr", "4==3||5==6", "binary_op:|| binary_op:== literal:4 literal:3 binary_op:== literal:5 literal:6"},  //
    {"ignoreComments", "3; z=6 oh toodlee doo", "literal:3"},                                                       //
    {"ignoreCommentsLF", "3; z=6 oh toodlee doo\n", "literal:3"},                                                   //
    {"ignoreCommentsCRLF", "3; z=6 oh toodlee doo\r\n", "literal:3"},                                               //
    {"reservedVariablePrefixToUserVariable", "e2=1", "assignment:e2 literal:1"},                                    //
    {"reservedFunctionPrefixToUserVariable", "sine=1", "assignment:sine literal:1"},                                //
    {"reservedKeywordPrefixToUserVariable", "if1=1", "assignment:if1 literal:1"},                                   //
    {"builtinVariableP1", "p1", "identifier:p1"},                                                                   //
    {"builtinVariableP2", "p2", "identifier:p2"},                                                                   //
    {"builtinVariableP3", "p3", "identifier:p3"},                                                                   //
    {"builtinVariableP4", "p4", "identifier:p4"},                                                                   //
    {"builtinVariableP5", "p5", "identifier:p5"},                                                                   //
    {"builtinVariablePixel", "pixel", "identifier:pixel"},                                                          //
    {"builtinVariableLastsqr", "lastsqr", "identifier:lastsqr"},                                                    //
    {"builtinVariableRand", "rand", "identifier:rand"},                                                             //
    {"builtinVariablePi", "pi", "identifier:pi"},                                                                   //
    {"builtinVariableE", "e", "identifier:e"},                                                                      //
    {"builtinVariableMaxit", "maxit", "identifier:maxit"},                                                          //
    {"builtinVariableScrnmax", "scrnmax", "identifier:scrnmax"},                                                    //
    {"builtinVariableScrnpix", "scrnpix", "identifier:scrnpix"},                                                    //
    {"builtinVariableWhitesq", "whitesq", "identifier:whitesq"},                                                    //
    {"builtinVariableIsmand", "ismand", "identifier:ismand"},                                                       //
    {"builtinVariableCenter", "center", "identifier:center"},                                                       //
    {"builtinVariableMagxmag", "magxmag", "identifier:magxmag"},                                                    //
    {"builtinVariableRotskew", "rotskew", "identifier:rotskew"},                                                    //
    {"ifEmptyBody",
        "if(0)\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifWithoutParens",
        "if 0\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifBlankLinesBody",
        "if(0)\n"
        "\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifThenBody",
        "if(0)\n"
        "1\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } endif"},
    {"ifThenComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } } endif"},
    {"ifComparisonCondition",
        "if(1<2)\n"
        "endif",
        "if_statement:( binary_op:< literal:1 literal:2 ) { } endif"},
    {"ifConjunctiveCondition",
        "if(1&&2)\n"
        "endif",
        "if_statement:( binary_op:&& literal:1 literal:2 ) { } endif"},
    {"ifElseEmptyBody",
        "if(0)\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifElseBlankLinesBody",
        "if(0)\n"
        "\n"
        "else\n"
        "\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifThenElseBody",
        "if(0)\n"
        "else\n"
        "1\n"
        "endif",
        "if_statement:( literal:0 ) { } else { literal:1 } endif"},
    {"ifThenBodyElse",
        "if(0)\n"
        "1\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } endif"},
    {"ifThenElseComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "else\n"
        "3\n"
        "4\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } "
        "} else { statement_seq:2 { literal:3 literal:4 } } endif"},
    {"ifElseIfEmptyBody",
        "if(0)\n"
        "elseif(1)\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { } else { if_statement:( literal:1 ) { } endif } endif"},
    {"elseIfWithoutOpenParen",
        "if 0\n"
        "elseif 1\n"
        "endif\n",
        "if_statement:( literal:0 ) { } else { if_statement:( literal:1 ) { } endif } endif"},
    {"ifMultipleElseIfEmptyBody",
        "if(0)\n"
        "elseif(0)\n"
        "elseif(0)\n"
        "else\n"
        "endif",
        "if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { } "
        "endif } "
        "endif } "
        "endif"},
    {"ifElseIfBlankLinesBody",
        "if(0)\n"
        "\n"
        "elseif(0)\n"
        "\n"
        "else\n"
        "\n"
        "endif",
        "if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { } "
        "endif } "
        "endif"},
    {"ifThenElseIfBody",
        "if(0)\n"
        "elseif(0)\n"
        "1\n"
        "endif",
        "if_statement:( literal:0 ) { } "
        "else { if_statement:( literal:0 ) { literal:1 } "
        "endif } "
        "endif"},
    {"ifThenBodyElseIf",
        "if(0)\n"
        "1\n"
        "elseif(0)\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } "
        "else { if_statement:( literal:0 ) { } "
        "endif } "
        "endif"},
    {"ifThenElseIfComplexBody",
        "if(0)\n"
        "1\n"
        "2\n"
        "elseif(1)\n"
        "3\n"
        "4\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } } "
        "else { if_statement:( literal:1 ) { statement_seq:2 { literal:3 literal:4 } } "
        "endif } "
        "endif"},
    {"chainedElseIf",
        "if(0)\n"
        "1\n"
        "elseif(2)\n"
        "3\n"
        "elseif(4)\n"
        "5\n"
        "elseif(6)\n"
        "7\n"
        "endif",
        "if_statement:( literal:0 ) { literal:1 } "
        "else { if_statement:( literal:2 ) { literal:3 } "
        "else { if_statement:( literal:4 ) { literal:5 } "
        "else { if_statement:( literal:6 ) { literal:7 } "
        "endif } "
        "endif } "
        "endif } "
        "endif"},
    {"backslashContinuesStatement",
        "if(\\\n"
        "0)\n"
        "endif",
        "if_statement:( literal:0 ) { } endif"},
    {"ifSequence",
        "if(0)\n"
        "1,2\n"
        "endif",
        "if_statement:( literal:0 ) { statement_seq:2 { literal:1 literal:2 } } endif"},
    {"elseSequence",
        "if(0)\n"
        "else\n"
        "1,2\n"
        "endif",
        "if_statement:( literal:0 ) { } else { statement_seq:2 { literal:1 literal:2 } } endif"},
    {"elseIfSequence",
        "if(0)\n"
        "elseif(3)\n"
        "1,2\n"
        "endif",
        "if_statement:( literal:0 ) { } else { "
        "if_statement:( literal:3 ) { statement_seq:2 { literal:1 literal:2 } } "
        "endif } "
        "endif"},
    {"conjunctiveAssignment",   //
        "b = z > x || z > y\n", //
        "assignment:b binary_op:||"
        " binary_op:> identifier:z identifier:x"
        " binary_op:> identifier:z identifier:y"},
};

TEST_P(SimpleExpressions, parse)
{
    const SimpleExpressionParam &param{GetParam()};

    const ast::FormulaSectionsPtr result{parse(param.text, Options{})};

    ASSERT_TRUE(result);
    const ast::Expr bailout = result->bailout;
    ASSERT_TRUE(bailout);
    EXPECT_EQ(param.expected, trim_ws(to_string(bailout)));
    const ast::Expr init = result->initialize;
    ASSERT_TRUE(init);
    EXPECT_EQ("statement_seq:0 { }", trim_ws(to_string(init)));
    const ast::Expr iter = result->iterate;
    ASSERT_TRUE(iter);
    EXPECT_EQ("statement_seq:0 { }", trim_ws(to_string(iter)));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, SimpleExpressions, ValuesIn(s_simple_expressions));

TEST(TestFormulaParse, multipleDefaults)
{
    ParserPtr parser{create_parser("default:\n"
                                   "angle = 180\n"
                                   "center = (-1.5, 0)\n",
        Options{})};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_TRUE(result->defaults);
    EXPECT_EQ("statement_seq:2 {"
              " setting:angle=180"
              " setting:center=(-1.5,0)"
              " }",
        trim_ws(to_string(result->defaults)));
}

TEST(TestFormulaParse, invalidIdentifier)
{
    ParserPtr parser{create_parser("1a", Options{})};

    const ast::FormulaSectionsPtr result{parser->parse()};

    EXPECT_FALSE(result);
    EXPECT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
}

static MultiStatementParam s_multi_statements[]{
    {"sequence", "3,4"}, //
    {"statements", "3\n4\n"},
    {"assignmentStatements", "z=3\nz=4\n"},
    {"assignmentWithComments", "z=3; comment\nz=4; another comment\n"},
    {"assignmentWithBlankLines", "z=3; comment\n\r\n\nz=4; another comment\n"},
    {"commaSeparatedStatements", "3,4"},
    {"commaSeparatedAssignmentStatements", "z=3,z=4"},
    {"mixedNewlineAndCommaSeparators", "z=3,z=4\nz=5"},
    {"commaWithSpaces", "3 , 4"},
    {"compactIf", "if(1),1,elseif(2),2,else,0,endif"},
};

TEST_P(MultiStatements, parse)
{
    const MultiStatementParam &param{GetParam()};
    const ast::FormulaSectionsPtr result{parse(param.text, Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->iterate);
    EXPECT_TRUE(result->bailout);
    const ast::Expr init = result->initialize;
    ASSERT_TRUE(init);
    EXPECT_EQ("statement_seq:0 { }", trim_ws(to_string(init)));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, MultiStatements, ValuesIn(s_multi_statements));

TEST(TestFormulaParse, initializeIterateBailout)
{
    const ast::FormulaSectionsPtr result{parse("z=pixel:z=z*z+pixel,|z|>4", Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->initialize);
    EXPECT_TRUE(result->iterate);
    EXPECT_TRUE(result->bailout);
}

TEST(TestFormulaParse, statementSequenceInitialize)
{
    const ast::FormulaSectionsPtr result{parse("z=pixel,d=0:z=z*z+pixel,|z|>4", Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->initialize);
    EXPECT_TRUE(result->iterate);
    EXPECT_TRUE(result->bailout);
}

static std::vector<std::string> s_builtin_vars{
    "p1", "p2", "p3", "p4", "p5",                       //
    "pixel", "lastsqr", "rand", "pi", "e",              //
    "maxit", "scrnmax", "scrnpix", "whitesq", "ismand", //
    "center", "magxmag", "rotskew",                     //
};

TEST_P(BuiltinVariables, notAssignable)
{
    const std::string text{GetParam() + "=1"};
    Options options;
    options.allow_builtin_assignment = false;
    options.dialect = Dialect::BASIC;
    ParserPtr parser{create_parser(text, options)};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result) << "should have parsed '" << text << "'";
    ASSERT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
    const auto &error{parser->get_errors().front()};
    EXPECT_EQ(ErrorCode::EXPECTED_STATEMENT, error.code);
    EXPECT_EQ(GetParam().length() + 2, error.position.column);
}

TEST_P(BuiltinVariables, assignable)
{
    const ast::FormulaSectionsPtr result{parse(GetParam() + "=1", Options{true})};

    ASSERT_TRUE(result);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, BuiltinVariables, ValuesIn(s_builtin_vars));

static std::vector<std::string> s_functions{
    "sin", "cos", "sinh", "cosh", "cosxx",      //
    "tan", "cotan", "tanh", "cotanh", "sqr",    //
    "log", "exp", "abs", "conj", "real",        //
    "imag", "flip", "fn1", "fn2", "fn3",        //
    "fn4", "srand", "asin", "acos", "asinh",    //
    "acosh", "atan", "atanh", "sqrt", "cabs",   //
    "floor", "ceil", "trunc", "round", "ident", //
};

TEST_P(Functions, notAssignable)
{
    const std::string text{GetParam() + "=1"};
    ParserPtr parser{create_parser(text, Options{false})};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result) << "Formula should not have parsed";
    ASSERT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
    const auto &error{parser->get_errors().front()};
    EXPECT_EQ(ErrorCode::BUILTIN_FUNCTION_ASSIGNMENT, error.code);
    EXPECT_EQ(GetParam().length() + 2, error.position.column);
}

TEST_P(Functions, functionOne)
{
    const ast::FormulaSectionsPtr result{parse(GetParam() + "(1)", Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->bailout);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, Functions, ValuesIn(s_functions));

TEST_P(ReservedWords, notAssignable)
{
    const std::string text{GetParam() + "=1"};
    const ParserPtr parser{create_parser(text, Options{})};

    const ast::FormulaSectionsPtr result1{parser->parse()};
    const ast::FormulaSectionsPtr result2{parse(GetParam() + "2=1", Options{})};

    ASSERT_FALSE(result1);
    EXPECT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result2->bailout);
}

static std::string s_reserved_words[]{
    "if",
    "else",
    "elseif",
    "endif",
};

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ReservedWords, ValuesIn(s_reserved_words));

static ParseFailureParam s_parse_failures[]{
    {"ifWithBadCondition", "if(:)\n", ErrorCode::EXPECTED_PRIMARY},                                      //
    {"ifWithoutSeparator", "if(1)", ErrorCode::EXPECTED_STATEMENT_SEPARATOR},                            //
    {"ifWithoutEndIf", "if(1)\n", ErrorCode::EXPECTED_ENDIF},                                            //
    {"ifElseWithoutSeparator", "if(1)\nelse", ErrorCode::EXPECTED_STATEMENT_SEPARATOR},                  //
    {"ifElseWithoutEndIf", "if(1)\nelse\n", ErrorCode::EXPECTED_ENDIF},                                  //
    {"ifElseIfWithoutSeparator", "if(1)\nelseif(0)", ErrorCode::EXPECTED_STATEMENT_SEPARATOR},           //
    {"ifElseIfWithoutEndIf", "if(1)\nelseif(0)\n", ErrorCode::EXPECTED_ENDIF},                           //
    {"ifElseIfElseWithoutSeparator", "if(1)\nelseif(0)\nelse", ErrorCode::EXPECTED_STATEMENT_SEPARATOR}, //
    {"ifElseIfElseWithoutEndIf", "if(1)\nelseif(0)\nelse\n", ErrorCode::EXPECTED_ENDIF},                 //
    {"builtinNoTerminator", "builtin:", ErrorCode::EXPECTED_TERMINATOR},                                 //
    {"builtinInvalidKey", "builtin:\n1", ErrorCode::EXPECTED_IDENTIFIER},                                //
    {"builtinNoAssignment", "builtin:\ntype 0", ErrorCode::EXPECTED_ASSIGNMENT},                         //
    {"builtinBadKey", "builtin:\ntipe=1", ErrorCode::BUILTIN_SECTION_INVALID_KEY},                       //
    {"builtinBadType", "builtin:\ntype=0", ErrorCode::BUILTIN_SECTION_INVALID_TYPE},                     //
    {"builtinBadValue", "builtin:\ntype=1.0", ErrorCode::EXPECTED_INTEGER},                              //
    {"builtinValueNoTerminator", "builtin:\ntype=1 switch:", ErrorCode::EXPECTED_TERMINATOR},            //
    {"defaultNoTerminator", "default:", ErrorCode::EXPECTED_TERMINATOR},                                 //
    {"defaultInvalidKey", "default:\n1", ErrorCode::EXPECTED_IDENTIFIER},                                //
    {"defaultBadKey", "default:\ntipe=1", ErrorCode::DEFAULT_SECTION_INVALID_KEY},                       //
    {"defaultMaxIterNoAssignment", "default:\nmaxiter 0", ErrorCode::EXPECTED_ASSIGNMENT},               //
    {"defaultMaxIterNoValue", "default:\nmaxiter=", ErrorCode::EXPECTED_INTEGER},                        //
    {"defaultMaxIterBadValue", "default:\nmaxiter=1.0", ErrorCode::EXPECTED_INTEGER},                    //
    {"defaultMaxIterNoTerminator", "default:\nmaxiter=1", ErrorCode::EXPECTED_TERMINATOR},               //
    {"defaultAngleNoAssignment", "default:\nangle 0", ErrorCode::EXPECTED_ASSIGNMENT},                   //
    {"defaultAngleNoValue", "default:\nangle=", ErrorCode::EXPECTED_FLOATING_POINT},                     //
    {"defaultAngleBadValue", "default:\nangle=enum", ErrorCode::EXPECTED_FLOATING_POINT},                //
    {"defaultAngleNoTerminator", "default:\nangle=1", ErrorCode::EXPECTED_TERMINATOR},                   //
    {"defaultCenterNoAssignment", "default:\ncenter 0", ErrorCode::EXPECTED_ASSIGNMENT},                 //
    {"defaultCenterNoValue", "default:\ncenter=", ErrorCode::EXPECTED_OPEN_PAREN},                       //
    {"defaultCenterBadValue", "default:\ncenter=enum", ErrorCode::EXPECTED_OPEN_PAREN},                  //
    {"defaultCenterNoRealPart", "default:\ncenter=(", ErrorCode::EXPECTED_FLOATING_POINT},               //
    {"defaultCenterNoComma", "default:\ncenter=(1.0", ErrorCode::EXPECTED_COMMA},                        //
    {"defaultCenterNoImagPart", "default:\ncenter=(1.0,", ErrorCode::EXPECTED_FLOATING_POINT},           //
    {"defaultCenterNoCloseParen", "default:\ncenter=(1.0,1.0", ErrorCode::EXPECTED_CLOSE_PAREN},         //
    {"defaultCenterNoTerminator", "default:\ncenter=(1.0,1.0)", ErrorCode::EXPECTED_TERMINATOR},         //
    {"defaultTitleNoAssignment", "default:\ntitle enum", ErrorCode::EXPECTED_ASSIGNMENT},                //
    {"defaultTitleNoValue", "default:\ntitle=", ErrorCode::EXPECTED_STRING},                             //
    {"defaultTitleBadValue", "default:\ntitle=enum", ErrorCode::EXPECTED_STRING},                        //
    {"defaultMethodNoAssignment", "default:\nmethod", ErrorCode::EXPECTED_ASSIGNMENT},                   //
    {"defaultMethodNoValue", "default:\nmethod=", ErrorCode::EXPECTED_IDENTIFIER},                       //
    {"defaultMethodBadValue", "default:\nmethod=foo", ErrorCode::DEFAULT_SECTION_INVALID_METHOD},        //
    {"defaultMethodNoTerminator", "default:\nmethod=guessing", ErrorCode::EXPECTED_TERMINATOR},          //
    {"defaultPerturbNoAssignment", "default:\nperturb", ErrorCode::EXPECTED_ASSIGNMENT},                 //
    {"defaultPerturbNoValue", "default:\nperturb=", ErrorCode::EXPECTED_PRIMARY},                        //
    {"defaultPerturbBadValue", "default:\nperturb=if", ErrorCode::EXPECTED_PRIMARY},                     //
    {"defaultPerturbNoTerminator", "default:\nperturb=true", ErrorCode::EXPECTED_TERMINATOR},            //
    {"switchNoTerminator", "switch:", ErrorCode::EXPECTED_TERMINATOR},                                   //
    {"switchInvalidKey", "switch:\n1", ErrorCode::EXPECTED_IDENTIFIER},                                  //
    {"switchTypeNoAssignment", "switch:\ntype \"Julia\"", ErrorCode::EXPECTED_ASSIGNMENT},               //
    {"switchTypeNoTerminator", "switch:\ntype=\"Julia\"", ErrorCode::EXPECTED_TERMINATOR},               //
    {"switchTypeNoString", "switch:\ntype=3\n", ErrorCode::EXPECTED_STRING},                             //
    {"switchParamInvalidValue", "switch:\nfoo=3\n", ErrorCode::EXPECTED_IDENTIFIER},                     //
    {"switchParamNoTerminator", "switch:\nfoo=foo", ErrorCode::EXPECTED_TERMINATOR},                     //
    {"invalidToken", "1a", ErrorCode::EXPECTED_PRIMARY},                                                 //
    {"ifWithoutCloseParen", "if(1", ErrorCode::EXPECTED_CLOSE_PAREN},                                    //
    {"elseIfWithoutCloseParen", "if(1)\nelseif(1\nendif\n", ErrorCode::EXPECTED_CLOSE_PAREN},            //
    {"exprAssignment", "1/c=pixel", ErrorCode::EXPECTED_STATEMENT},                                      //
    {"functionWithoutCloseParen", "sin(z", ErrorCode::EXPECTED_CLOSE_PAREN},                             //
    {"complexLiteralWithoutComma", "(6 4)", ErrorCode::EXPECTED_CLOSE_PAREN},                            //
    {"complexLiteralWithoutClosingParen", "(6,4", ErrorCode::EXPECTED_CLOSE_PAREN},                      //
    {"unbalancedModulus", "|4", ErrorCode::EXPECTED_CLOSE_MODULUS},                                      //
    {"assignmentAsExpression", "2+z=4", ErrorCode::EXPECTED_STATEMENT},                                  //
};

TEST_P(ParseFailures, parse)
{
    const ParseFailureParam &param{GetParam()};
    const ParserPtr parser{create_parser(param.text, Options{})};

    const ast::FormulaSectionsPtr result{parser->parse()};

    EXPECT_FALSE(result);
    ASSERT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
    const Diagnostic &error{parser->get_errors().back()};
    EXPECT_EQ(param.expected_error, error.code);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ParseFailures, ValuesIn(s_parse_failures));

TEST(TestFormulaParse, diagnosticsRetainSourceFilename)
{
    Options options;
    options.source_filename = "common.ulb";
    ParserPtr parser{create_parser("1a", options)};

    const ast::FormulaSectionsPtr result{parser->parse()};

    EXPECT_FALSE(result);
    ASSERT_FALSE(parser->get_errors().empty());
    EXPECT_EQ("common.ulb", parser->get_errors().back().position.filename);
}

TEST(TestFormulaParse, extendedExpressionForms)
{
    const ast::FormulaSectionsPtr result{
        parse("foo(1, 2, @p, #c) + !bar.baz[3] % new thing(4) + Derived(base)", Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->bailout);
    EXPECT_EQ("binary_op:+ "
              "binary_op:+ "
              "function_call:foo( literal:1 literal:2 parameter_ref:p constant_ref:c ) "
              "binary_op:% unary_op:! index:[ member:baz { identifier:bar } literal:3 ] new:thing( literal:4 ) "
              "function_call:derived( identifier:base )",
        trim_ws(to_string(result->bailout)));
}

TEST(TestFormulaParse, extendedDeclarationsAndLoops)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "float x=1\n"
                                               "int a[2,3]\n"
                                               "while x\n"
                                               "x=x-1\n"
                                               "endwhile\n"
                                               "repeat\n"
                                               "x=x+1\n"
                                               "until x>3\n"
                                               "return x\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("statement_seq:5 { "
              "declaration:float,x literal:1 "
              "declaration:int,a[ literal:2 , literal:3 ] "
              "while:( identifier:x ) { assignment:x binary_op:- identifier:x literal:1 } "
              "repeat { assignment:x binary_op:+ identifier:x literal:1 } until ( binary_op:> identifier:x literal:3 ) "
              "return: identifier:x }",
        trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedTypedDeclarations)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "bool flag=true\n"
                                               "complex c=(1,2)\n"
                                               "color shade\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("statement_seq:3 { "
              "declaration:bool,flag literal:true "
              "declaration:complex,c literal:(1,2) "
              "declaration:color,shade }",
        trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedDynamicArrays)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "int values[]\n"
                                               "setLength(values, 3)\n"
                                               "values[0]=1\n"
                                               "values[1]=values[0]+1\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("statement_seq:4 { "
              "declaration:int,values[] "
              "function_call:setlength( identifier:values literal:3 ) "
              "assignment:{ index:[ identifier:values literal:0 ] } literal:1 "
              "assignment:{ index:[ identifier:values literal:1 ] } "
              "binary_op:+ index:[ identifier:values literal:0 ] literal:1 }",
        trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedReturnWithoutExpression)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "return\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("return", trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedFunctionDeclaration)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "float func blend(const float a, color &b) const\n"
                                               "return a\n"
                                               "endfunc\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("function_decl:float blend(const float a,color &b) const { return: identifier:a }",
        trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedVoidFunctionDeclaration)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "func square2(complex &x)\n"
                                               "complex old=x\n"
                                               "x=x*x\n"
                                               "return\n"
                                               "endfunc\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("function_decl:square2(complex &x) { "
              "statement_seq:3 { "
              "declaration:complex,old identifier:x "
              "assignment:x binary_op:* identifier:x identifier:x "
              "return } }",
        trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedFunctionDeclarationsCanAppearInBlocks)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "if enabled\n"
                                               "int func nested(int n)\n"
                                               "if n\n"
                                               "return nested(n-1)\n"
                                               "endif\n"
                                               "return 0\n"
                                               "endfunc\n"
                                               "endif\n",
        Options{})};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->per_image);
    EXPECT_EQ("if_statement:( identifier:enabled ) { "
              "function_decl:int nested(int n) { "
              "statement_seq:2 { "
              "if_statement:( identifier:n ) { "
              "return: function_call:nested( binary_op:- identifier:n literal:1 ) "
              "} endif "
              "return: literal:0 } } } endif",
        trim_ws(to_string(result->per_image)));
}

TEST(TestFormulaParse, extendedFormulaKinds)
{
    Options coloring;
    coloring.entry_kind = EntryKind::COLORING;
    const ast::FormulaSectionsPtr color_result{parse("global:\n"
                                                     "int x=1\n"
                                                     "final:\n"
                                                     "return x\n"
                                                     "default:\n"
                                                     "title=\"color\"\n",
        coloring)};

    ASSERT_TRUE(color_result);
    EXPECT_TRUE(color_result->per_image);
    EXPECT_TRUE(color_result->final);
    EXPECT_TRUE(color_result->defaults);

    Options transform;
    transform.entry_kind = EntryKind::TRANSFORMATION;
    const ast::FormulaSectionsPtr transform_result{parse("global:\n"
                                                         "int x=1\n"
                                                         "transform:\n"
                                                         "return x\n",
        transform)};

    ASSERT_TRUE(transform_result);
    EXPECT_TRUE(transform_result->per_image);
    EXPECT_TRUE(transform_result->transform);
}

TEST(TestFormulaParse, extendedClassSections)
{
    Options options;
    options.entry_kind = EntryKind::CLASS;
    const ast::FormulaSectionsPtr result{parse("public:\n"
                                               "func Texture()\n"
                                               "endfunc\n"
                                               "protected:\n"
                                               "float amount\n"
                                               "private:\n"
                                               "int seed\n"
                                               "default:\n"
                                               "title=\"Texture\"\n",
        options)};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->public_members);
    EXPECT_TRUE(result->protected_members);
    EXPECT_TRUE(result->private_members);
    EXPECT_TRUE(result->defaults);
}

TEST(TestFormulaParse, extendedClassObjectSyntax)
{
    Options options;
    options.entry_kind = EntryKind::CLASS;
    const ast::FormulaSectionsPtr result{parse("import \"common.ulb\"\n"
                                               "public:\n"
                                               "import \"math.ulb\"\n"
                                               "func Texture()\n"
                                               "amount=0.1\n"
                                               "endfunc\n"
                                               "static int func min(int a, int b)\n"
                                               "return a\n"
                                               "endfunc\n"
                                               "protected:\n"
                                               "float amount\n"
                                               "private:\n"
                                               "complex func distort(const complex x)\n"
                                               "return this.amount + x\n"
                                               "endfunc\n"
                                               "default:\n"
                                               "title=\"Texture\"\n",
        options)};

    ASSERT_TRUE(result);
    ASSERT_EQ(2U, result->imports.size());
    EXPECT_EQ("common.ulb", result->imports[0].filename);
    EXPECT_EQ(1U, result->imports[0].location.line);
    EXPECT_EQ(1U, result->imports[0].location.column);
    EXPECT_FALSE(result->imports[0].implicit);
    EXPECT_EQ("math.ulb", result->imports[1].filename);
    EXPECT_EQ(3U, result->imports[1].location.line);
    EXPECT_EQ(1U, result->imports[1].location.column);
    EXPECT_FALSE(result->imports[1].implicit);
    ASSERT_TRUE(result->public_members);
    EXPECT_EQ("statement_seq:2 { "
              "function_decl:texture() { assignment:amount literal:0.1 } "
              "function_decl:int min(int a,int b) static { return: identifier:a } "
              "}",
        trim_ws(to_string(result->public_members)));
    ASSERT_TRUE(result->protected_members);
    EXPECT_EQ("declaration:float,amount", trim_ws(to_string(result->protected_members)));
    ASSERT_TRUE(result->private_members);
    EXPECT_EQ("function_decl:complex distort(const complex x) { "
              "return: binary_op:+ member:amount { identifier:this } identifier:x "
              "}",
        trim_ws(to_string(result->private_members)));
    ASSERT_TRUE(result->defaults);
    EXPECT_EQ("setting:title=\"Texture\"", trim_ws(to_string(result->defaults)));
}

TEST(TestFormulaParse, extendedUnsectionedClassDefaultsToPublic)
{
    Options options;
    options.entry_kind = EntryKind::CLASS;
    const ast::FormulaSectionsPtr result{parse("func Texture()\n"
                                               "endfunc\n",
        options)};

    ASSERT_TRUE(result);
    ASSERT_TRUE(result->public_members);
    EXPECT_EQ("function_decl:texture() { }", trim_ws(to_string(result->public_members)));
}

TEST(TestFormulaParse, coloringRejectsFractalSections)
{
    Options options;
    options.entry_kind = EntryKind::COLORING;
    ParserPtr parser{create_parser("bailout:\n"
                                   "1\n",
        options)};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result);
    ASSERT_FALSE(parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, parser->get_errors().back().code);
}

TEST(TestFormulaParse, transformationRejectsColoringSections)
{
    Options options;
    options.entry_kind = EntryKind::TRANSFORMATION;
    ParserPtr parser{create_parser("final:\n"
                                   "1\n",
        options)};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result);
    ASSERT_FALSE(parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, parser->get_errors().back().code);
}

TEST(TestFormulaParse, classRejectsFormulaSections)
{
    Options options;
    options.entry_kind = EntryKind::CLASS;
    ParserPtr parser{create_parser("global:\n"
                                   "1\n",
        options)};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result);
    ASSERT_FALSE(parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, parser->get_errors().back().code);
}

TEST(TestFormulaParse, extendedKindSectionsMustBeOrdered)
{
    Options coloring;
    coloring.entry_kind = EntryKind::COLORING;
    ParserPtr color_parser{create_parser("final:\n"
                                         "1\n"
                                         "loop:\n"
                                         "1\n",
        coloring)};

    const ast::FormulaSectionsPtr color_result{color_parser->parse()};

    ASSERT_FALSE(color_result);
    ASSERT_FALSE(color_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION_ORDER, color_parser->get_errors().back().code);

    Options klass;
    klass.entry_kind = EntryKind::CLASS;
    ParserPtr class_parser{create_parser("private:\n"
                                         "int seed\n"
                                         "public:\n"
                                         "func Texture()\n"
                                         "endfunc\n",
        klass)};

    const ast::FormulaSectionsPtr class_result{class_parser->parse()};

    ASSERT_FALSE(class_result);
    ASSERT_FALSE(class_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION_ORDER, class_parser->get_errors().back().code);
}

TEST(TestFormulaParse, fractalRejectsOtherKindSections)
{
    ParserPtr final_parser{create_parser("final:\n"
                                         "1\n",
        Options{})};

    const ast::FormulaSectionsPtr final_result{final_parser->parse()};

    ASSERT_FALSE(final_result);
    ASSERT_FALSE(final_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, final_parser->get_errors().back().code);

    ParserPtr transform_parser{create_parser("transform:\n"
                                             "1\n",
        Options{})};

    const ast::FormulaSectionsPtr transform_result{transform_parser->parse()};

    ASSERT_FALSE(transform_result);
    ASSERT_FALSE(transform_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, transform_parser->get_errors().back().code);
}

TEST(TestFormulaParse, defaultSettingsRespectEntryKind)
{
    Options coloring;
    coloring.entry_kind = EntryKind::COLORING;
    const ast::FormulaSectionsPtr color_allowed{parse("default:\n"
                                                      "helpfile=\"Help.html\"\n"
                                                      "helptopic=\"topic\"\n"
                                                      "precision=0\n"
                                                      "render=false\n"
                                                      "rating=recommended\n"
                                                      "title=\"Color\"\n",
        coloring)};

    ASSERT_TRUE(color_allowed);
    EXPECT_TRUE(color_allowed->defaults);

    ParserPtr color_parser{create_parser("default:\n"
                                         "angle=0\n",
        coloring)};

    const ast::FormulaSectionsPtr color_result{color_parser->parse()};

    ASSERT_FALSE(color_result);
    ASSERT_FALSE(color_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::DEFAULT_SECTION_INVALID_KEY, color_parser->get_errors().back().code);

    Options transform;
    transform.entry_kind = EntryKind::TRANSFORMATION;
    const ast::FormulaSectionsPtr transform_allowed{parse("default:\n"
                                                          "render=false\n"
                                                          "rating=average\n"
                                                          "title=\"Transform\"\n",
        transform)};

    ASSERT_TRUE(transform_allowed);
    EXPECT_TRUE(transform_allowed->defaults);

    ParserPtr transform_parser{create_parser("default:\n"
                                             "maxiter=100\n",
        transform)};

    const ast::FormulaSectionsPtr transform_result{transform_parser->parse()};

    ASSERT_FALSE(transform_result);
    ASSERT_FALSE(transform_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::DEFAULT_SECTION_INVALID_KEY, transform_parser->get_errors().back().code);

    Options klass;
    klass.entry_kind = EntryKind::CLASS;
    const ast::FormulaSectionsPtr class_allowed{parse("default:\n"
                                                      "rating=notRecommended\n"
                                                      "title=\"Texture\"\n",
        klass)};

    ASSERT_TRUE(class_allowed);
    EXPECT_TRUE(class_allowed->defaults);

    ParserPtr class_fractal_parser{create_parser("default:\n"
                                                 "magn=1\n",
        klass)};

    const ast::FormulaSectionsPtr class_fractal_result{class_fractal_parser->parse()};

    ASSERT_FALSE(class_fractal_result);
    ASSERT_FALSE(class_fractal_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::DEFAULT_SECTION_INVALID_KEY, class_fractal_parser->get_errors().back().code);

    ParserPtr class_formula_parser{create_parser("default:\n"
                                                 "helpfile=\"Help.html\"\n",
        klass)};

    const ast::FormulaSectionsPtr class_formula_result{class_formula_parser->parse()};

    ASSERT_FALSE(class_formula_result);
    ASSERT_FALSE(class_formula_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::DEFAULT_SECTION_INVALID_KEY, class_formula_parser->get_errors().back().code);
}

TEST(TestFormulaParse, defaultAndSwitchStopOnExtendedSections)
{
    ParserPtr default_parser{create_parser("default:\n"
                                           "title=\"Fractal\"\n"
                                           "final:\n"
                                           "1\n",
        Options{})};

    const ast::FormulaSectionsPtr default_result{default_parser->parse()};

    ASSERT_FALSE(default_result);
    ASSERT_FALSE(default_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, default_parser->get_errors().back().code);

    ParserPtr switch_parser{create_parser("switch:\n"
                                          "type=\"Julia\"\n"
                                          "transform:\n"
                                          "1\n",
        Options{})};

    const ast::FormulaSectionsPtr switch_result{switch_parser->parse()};

    ASSERT_FALSE(switch_result);
    ASSERT_FALSE(switch_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION, switch_parser->get_errors().back().code);

    Options klass;
    klass.entry_kind = EntryKind::CLASS;
    ParserPtr class_parser{create_parser("default:\n"
                                         "title=\"Texture\"\n"
                                         "public:\n"
                                         "func Texture()\n"
                                         "endfunc\n",
        klass)};

    const ast::FormulaSectionsPtr class_result{class_parser->parse()};

    ASSERT_FALSE(class_result);
    ASSERT_FALSE(class_parser->get_errors().empty());
    EXPECT_EQ(ErrorCode::INVALID_SECTION_ORDER, class_parser->get_errors().back().code);
}

static ParseParam s_single_sections[]{
    {"globalSection",
        "global:\n"
        "1\n",
        Section::PER_IMAGE},
    {"initSection",
        "init:\n"
        "1\n",
        Section::INITIALIZE},
    {"loopSection",
        "loop:\n"
        "1\n",
        Section::ITERATE},
    {"bailoutSection",
        "bailout:\n"
        "1\n",
        Section::BAILOUT},
    {"perturbInitSection",
        "perturbinit:\n"
        "1\n",
        Section::PERTURB_INITIALIZE},
    {"perturbLoopSection",
        "perturbloop:\n"
        "1\n",
        Section::PERTURB_ITERATE},
    {"defaultSection",
        "default:\n"
        "angle=0\n",
        Section::DEFAULT},
    {"switchSection",
        "switch:\n"
        "type=\"Julia\"\n",
        Section::SWITCH},
};

ast::Expr get_section(const ast::FormulaSectionsPtr &sections, Section section)
{
    if (!sections)
    {
        return nullptr;
    }

    switch (section)
    {
    case Section::PER_IMAGE:
        return sections->per_image;
    case Section::INITIALIZE:
        return sections->initialize;
    case Section::ITERATE:
        return sections->iterate;
    case Section::BAILOUT:
        return sections->bailout;
    case Section::PERTURB_INITIALIZE:
        return sections->perturb_initialize;
    case Section::PERTURB_ITERATE:
        return sections->perturb_iterate;
    case Section::DEFAULT:
        return sections->defaults;
    case Section::SWITCH:
        return sections->type_switch;
    case Section::BUILTIN:
        return sections->builtin;
    default:
        break;
    }

    return nullptr;
}

TEST_P(SingleSections, parse)
{
    const ParseParam &param{GetParam()};

    const ast::FormulaSectionsPtr result{parse(param.text, Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(get_section(result, param.section));
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, SingleSections, ValuesIn(s_single_sections));

TEST(TestFormulaParse, builtinSectionMandelbrot)
{
    const ast::FormulaSectionsPtr result{parse("builtin:\n"
                                               "type=1\n",
        Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->builtin);
    EXPECT_EQ("setting:type=1\n", to_string(result->builtin));
}

TEST(TestFormulaParse, builtinSectionJulia)
{
    const ast::FormulaSectionsPtr result{parse("builtin:\n"
                                               "type=2\n",
        Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->builtin);
    EXPECT_EQ("setting:type=2\n", to_string(result->builtin));
}

static SimpleExpressionParam s_default_values[]{
    {"angle", "angle=0", "setting:angle=0\n"},                  //
    {"center", "center=(-0.5,0)", "setting:center=(-0.5,0)\n"}, //
    {"helpFile", R"(helpfile="HelpFile.html")",
        R"(setting:helpfile="HelpFile.html")"
        "\n"},
    {"helpTopic", R"(helptopic="DivideBrot5")",
        R"(setting:helptopic="DivideBrot5")"
        "\n"},
    {"magn", "magn=4.5", "setting:magn=4.5\n"},                            //
    {"maxIter", "maxiter=256", "setting:maxiter=256\n"},                   //
    {"methodGuessing", "method=guessing", "setting:method=guessing\n"},    //
    {"methodMultiPass", "method=multipass", "setting:method=multipass\n"}, //
    {"methodOnePass", "method=onepass", "setting:method=onepass\n"},       //
    {"periodicity0", "periodicity=0", "setting:periodicity=0\n"},          //
    {"periodicity1", "periodicity=1", "setting:periodicity=1\n"},          //
    {"periodicity2", "periodicity=2", "setting:periodicity=2\n"},          //
    {"periodicity3", "periodicity=3", "setting:periodicity=3\n"},          //
    {"perturbFalse", "perturb=false", "setting:perturb=false\n"},          //
    {"perturbTrue", "perturb=true", "setting:perturb=true\n"},             //
    {"perturbExpr", "perturb=power==2 || power == 3",
        "setting:perturb={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:3\n"
        "}\n"},
    {"precisionNumber", "precision=30",
        "setting:precision={\n"
        "literal:30\n"
        "}\n"}, //
    {"precisionExpr", "precision=log(fracmagn) / log(10)",
        "setting:precision={\n"
        "binary_op:/\n"
        "function_call:log(\n"
        "identifier:fracmagn\n"
        ")\n"
        "function_call:log(\n"
        "literal:10\n"
        ")\n"
        "}\n"},
    {"ratingRecommended", "rating=recommended", "setting:rating=recommended\n"},              //
    {"ratingAverage", "rating=average", "setting:rating=average\n"},                          //
    {"ratingNotRecommended", "rating=notRecommended", "setting:rating=notRecommended\n"},     //
    {"renderTrue", "render=true", "setting:render=true\n"},                                   //
    {"renderFalse", "render=false", "setting:render=false\n"},                                //
    {"skew", "skew=-4.5", "setting:skew=-4.5\n"},                                             //
    {"stretch", "stretch=4.5", "setting:stretch=4.5\n"},                                      //
    {"title", R"(title="This is a fancy one!")", "setting:title=\"This is a fancy one!\"\n"}, //
    {"emptyBoolParamBlock",
        "bool param foo\n"
        "endparam",
        "param_block:bool,foo {\n"
        "}\n"}, //
    {"emptyIntParamBlock",
        "int param foo\n"
        "endparam",
        "param_block:int,foo {\n"
        "}\n"}, //
    {"emptyFloatParamBlock",
        "float param foo\n"
        "endparam",
        "param_block:float,foo {\n"
        "}\n"}, //
    {"emptyComplexParamBlock",
        "complex param foo\n"
        "endparam",
        "param_block:complex,foo {\n"
        "}\n"}, //
    {"emptyColorParamBlock",
        "color param foo\n"
        "endparam",
        "param_block:color,foo {\n"
        "}\n"}, //
    {"emptyClassParamBlock",
        "Bailout param foo\n"
        "endparam",
        "param_block:bailout,foo {\n"
        "}\n"}, //
    {"captionParamBlock",
        "bool param foo\n"
        "caption=\"My parameter\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:caption=\"My parameter\"\n"
        "}\n"},
    {"multipleSettingsParamBlock",
        "float param foo\n"
        "caption=\"Foo\"\n"
        "default=4.5\n"
        "min=0\n"
        "max=10\n"
        "expanded=true\n"
        "endparam",
        "param_block:float,foo {\n"
        "statement_seq:5 {\n"
        "setting:caption=\"Foo\"\n"
        "setting:default=4.5\n"
        "setting:min=0\n"
        "setting:max=10\n"
        "setting:expanded=true\n"
        "}\n"
        "}\n"},
    {"typelessParamBlock",
        "param foo\n"
        "endparam",
        "param_block:foo {\n"
        "}\n"},
    {"emptyFunctionBlock",
        "complex func pre_func\n"
        "endfunc",
        "function_block:complex,pre_func {\n"
        "}\n"},
    {"typelessFunctionBlock",
        "func fn1\n"
        "default=cos()\n"
        "endfunc",
        "function_block:fn1 {\n"
        "setting:default={\n"
        "function_call:cos(\n"
        ")\n"
        "}\n"
        "}\n"},
    {"functionBlockFn2",
        "func fn2\n"
        "default=atanh ()\n"
        "endfunc",
        "function_block:fn2 {\n"
        "setting:default={\n"
        "function_call:atanh(\n"
        ")\n"
        "}\n"
        "}\n"},
    {"functionBlockFn3",
        "func fn3\n"
        "endfunc",
        "function_block:fn3 {\n"
        "}\n"},
    {"functionBlockFn4",
        "func fn4\n"
        "endfunc",
        "function_block:fn4 {\n"
        "}\n"},
    {"multipleSettingsFunctionBlock",
        "complex func pre_func\n"
        "caption=\"Pre Function\"\n"
        "default=ident()\n"
        "enabled=blur\n"
        "endfunc",
        "function_block:complex,pre_func {\n"
        "statement_seq:3 {\n"
        "setting:caption=\"Pre Function\"\n"
        "setting:default={\n"
        "function_call:ident(\n"
        ")\n"
        "}\n"
        "setting:enabled={\n"
        "identifier:blur\n"
        "}\n"
        "}\n"
        "}\n"},
    {"colorFunctionBlock",
        "color func f_mergemode\n"
        "caption=\"Merge Mode\"\n"
        "default=mergenormal()\n"
        "hint=\"Sets the merge mode.\"\n"
        "visible=show_merge\n"
        "endfunc",
        "function_block:color,f_mergemode {\n"
        "statement_seq:4 {\n"
        "setting:caption=\"Merge Mode\"\n"
        "setting:default={\n"
        "function_call:mergenormal(\n"
        ")\n"
        "}\n"
        "setting:hint=\"Sets the merge mode.\"\n"
        "setting:visible={\n"
        "identifier:show_merge\n"
        "}\n"
        "}\n"
        "}\n"},
    {"emptyHeadingBlock",
        "heading\n"
        "endheading",
        "heading_block {\n"
        "}\n"},
    {"multipleSettingsHeadingBlock",
        "heading\n"
        "caption=\"Options\"\n"
        "text=\"Pick carefully.\"\n"
        "hint=\"Shown in corpus.\"\n"
        "enabled=show_options\n"
        "visible=mode == 2\n"
        "expanded=false\n"
        "show=false\n"
        "extended=false\n"
        "endheading",
        "heading_block {\n"
        "statement_seq:8 {\n"
        "setting:caption=\"Options\"\n"
        "setting:text=\"Pick carefully.\"\n"
        "setting:hint=\"Shown in corpus.\"\n"
        "setting:enabled={\n"
        "identifier:show_options\n"
        "}\n"
        "setting:visible={\n"
        "binary_op:==\n"
        "identifier:mode\n"
        "literal:2\n"
        "}\n"
        "setting:expanded=false\n"
        "setting:show=false\n"
        "setting:extended=false\n"
        "}\n"
        "}\n"},
    {"defaultBoolParamBlock",
        "bool param foo\n"
        "default=true\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:default=true\n"
        "}\n"},
    {"defaultIntParamBlock",
        "int param foo\n"
        "default=1964\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:default=1964\n"
        "}\n"},
    {"defaultFloatParamBlock",
        "float param foo\n"
        "default=4.5\n"
        "endparam",
        "param_block:float,foo {\n"
        "setting:default=4.5\n"
        "}\n"},
    {"defaultComplexParamBlock",
        "complex param foo\n"
        "default=(4,5)\n"
        "endparam",
        "param_block:complex,foo {\n"
        "setting:default=(4,5)\n"
        "}\n"},
    {"defaultClassParamBlock",
        "Bailout param foo\n"
        "default=Bailout\n"
        "endparam",
        "param_block:bailout,foo {\n"
        "setting:default=bailout\n"
        "}\n"},
    {"enabledParamBlock",
        "bool param foo\n"
        "enabled=power==2 || power == 3\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:enabled={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:3\n"
        "}\n"
        "}\n"},
    {"enumParamBlock",
        "int param foo\n"
        "enum = \"Choice 1\" \"Choice 2\"\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:enum={\n"
        "\"Choice 1\"\n"
        "\"Choice 2\"\n"
        "}\n"
        "}\n"},
    {"expandedParamBlock",
        "int param foo\n"
        "expanded = false\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:expanded=false\n"
        "}\n"},
    {"exponentialParamBlock",
        "int param foo\n"
        "exponential = true\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:exponential=true\n"
        "}\n"},
    {"hintParamBlock",
        "bool param foo\n"
        "hint=\"look behind you\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:hint=\"look behind you\"\n"
        "}\n"},
    {"minIntParamBlock",
        "int param foo\n"
        "min=10\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:min=10\n"
        "}\n"},
    {"minFloatParamBlock",
        "float param foo\n"
        "min=4.5\n"
        "endparam",
        "param_block:float,foo {\n"
        "setting:min=4.5\n"
        "}\n"},
    {"minComplexParamBlock",
        "complex param foo\n"
        "min=(4.5,4.5)\n"
        "endparam",
        "param_block:complex,foo {\n"
        "setting:min=(4.5,4.5)\n"
        "}\n"},
    {"maxIntParamBlock",
        "int param foo\n"
        "max=10\n"
        "endparam",
        "param_block:int,foo {\n"
        "setting:max=10\n"
        "}\n"},
    {"maxFloatParamBlock",
        "float param foo\n"
        "max=4.5\n"
        "endparam",
        "param_block:float,foo {\n"
        "setting:max=4.5\n"
        "}\n"},
    {"maxComplexParamBlock",
        "complex param foo\n"
        "max=(4.5,4.5)\n"
        "endparam",
        "param_block:complex,foo {\n"
        "setting:max=(4.5,4.5)\n"
        "}\n"},
    {"selectableParamBlock",
        "bool param foo\n"
        "selectable=false\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:selectable=false\n"
        "}\n"},
    {"textParamBlock",
        "bool param foo\n"
        "text=\"Well hello there, pardner.\"\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:text=\"Well hello there, pardner.\"\n"
        "}\n"},
    {"visibleParamBlock",
        "bool param foo\n"
        "visible=power == 2 || power == 3\n"
        "endparam",
        "param_block:bool,foo {\n"
        "setting:visible={\n"
        "binary_op:||\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:2\n"
        "binary_op:==\n"
        "identifier:power\n"
        "literal:3\n"
        "}\n"
        "}\n"},
};

TEST_P(DefaultSection, parse)
{
    const SimpleExpressionParam &param{GetParam()};
    std::string text{"default:\n"};
    text.append(param.text);
    if (text.back() != '\n')
    {
        text.append(1, '\n');
    }

    const ast::FormulaSectionsPtr result{parse(text, Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->defaults);
    EXPECT_EQ(param.expected, to_string(result->defaults));
}

INSTANTIATE_TEST_SUITE_P(TestFormualParse, DefaultSection, ValuesIn(s_default_values));

TEST(TestParse, switchParameter)
{
    const ast::FormulaSectionsPtr result{parse("switch:\n"
                                               "seed=pixel\n",
        Options{})};

    ASSERT_TRUE(result);
    const ast::Expr &section{result->type_switch};
    ASSERT_TRUE(section);
    EXPECT_EQ("setting:seed=pixel\n", to_string(section));
}

TEST(TestParse, switchMultipleParameters)
{
    Options options;
    options.dialect = Dialect::EXTENDED;
    const ast::FormulaSectionsPtr result{parse("switch:\n"
                                               "type=\"Julia\"\n"
                                               "seed=#pixel\n"
                                               "bailout=bailout\n",
        options)};

    ASSERT_TRUE(result);
    const ast::Expr &section{result->type_switch};
    ASSERT_TRUE(section);
    EXPECT_EQ("statement_seq:3 {\n"
              "setting:type=\"Julia\"\n"
              "setting:seed=pixel\n"
              "setting:bailout=bailout\n"
              "}\n",
        to_string(section));
}

static InvalidSectionParam s_invalid_sections[]{
    {"unknownSection",
        "global:\n"
        "1\n"
        "unknown:\n"
        "1\n",
        ErrorCode::INVALID_SECTION},
    {"globalSectionFirst",
        "builtin:\n"
        "type=1\n"
        "global:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"builtinBeforeInit",
        "init:\n"
        "1\n"
        "builtin:\n"
        "type=1\n",
        ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS},
    {"initAfterLoop",
        "loop:\n"
        "1\n"
        "init:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"initAfterBailout",
        "bailout:\n"
        "1\n"
        "init:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"loopAfterBailout",
        "bailout:\n"
        "1\n"
        "loop:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"bailoutAfterPerturbInit",
        "perturbinit:\n"
        "1\n"
        "bailout:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"perturbInitAfterPerturbLoop",
        "perturbloop:\n"
        "1\n"
        "perturbinit:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"perturbLoopAfterDefault",
        "default:\n"
        "angle=0\n"
        "perturbloop:\n"
        "1\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"defaultAfterSwitch",
        "switch:\n"
        "foo=foo\n"
        "default:\n"
        "angle=0\n",
        ErrorCode::INVALID_SECTION_ORDER},
    {"globalTwice",
        "global:\n"
        "1\n"
        "global:\n"
        "1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"builtinTwice",
        "builtin:\n"
        "type=1\n"
        "builtin:\n"
        "type=1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"initTwice",
        "init:\n"
        "1\n"
        "init:\n"
        "1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"loopTwice",
        "loop:\n"
        "1\n"
        "loop:\n"
        "1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"bailoutTwice",
        "bailout:\n"
        "1\n"
        "bailout:\n"
        "1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"perturbInitTwice",
        "perturbinit:\n"
        "1\n"
        "perturbinit:\n"
        "1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"perturbLoopTwice",
        "perturbloop:\n"
        "1\n"
        "perturbloop:\n"
        "1\n",
        ErrorCode::DUPLICATE_SECTION},
    {"defaultTwice",
        "default:\n"
        "angle=120\n"
        "default:\n"
        "angle=120\n",
        ErrorCode::DUPLICATE_SECTION},
    {"switchTwice",
        "switch:\n"
        "foo=foo\n"
        "switch:\n"
        "foo=foo\n",
        ErrorCode::DUPLICATE_SECTION},
    {"invalidMethod",
        "default:\n"
        "method=junk\n",
        ErrorCode::DEFAULT_SECTION_INVALID_METHOD},
};

class InvalidSectionOrdering : public TestWithParam<InvalidSectionParam>
{
};

TEST_P(InvalidSectionOrdering, parse)
{
    const InvalidSectionParam &param{GetParam()};
    ParserPtr parser{create_parser(param.text, Options{})};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result) << "Formula should not have parsed";
    ASSERT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
    if (param.expected_error != ErrorCode::NONE)
    {
        const Diagnostic &error{parser->get_errors().back()};
        EXPECT_EQ(param.expected_error, error.code);
    }
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, InvalidSectionOrdering, ValuesIn(s_invalid_sections));

TEST(TestFormulaParse, maximumSections)
{
    const ast::FormulaSectionsPtr result{parse("global:\n"
                                               "1\n"
                                               "init:\n"
                                               "1\n"
                                               "loop:\n"
                                               "1\n"
                                               "bailout:\n"
                                               "1\n"
                                               "perturbinit:\n"
                                               "1\n"
                                               "perturbloop:\n"
                                               "1\n"
                                               "default:\n"
                                               "angle=0\n"
                                               "switch:\n"
                                               "type=\"Julia\"\n",
        Options{})};

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->per_image);
    EXPECT_FALSE(result->builtin);
    EXPECT_TRUE(result->initialize);
    EXPECT_TRUE(result->iterate);
    EXPECT_TRUE(result->bailout);
    EXPECT_TRUE(result->perturb_initialize);
    EXPECT_TRUE(result->perturb_iterate);
    EXPECT_TRUE(result->defaults);
    EXPECT_TRUE(result->type_switch);
}

TEST(TestFormulaParse, builtinSections)
{
    const ast::FormulaSectionsPtr result{parse("builtin:\n"
                                               "type=1\n"
                                               "perturbinit:\n"
                                               "1\n"
                                               "perturbloop:\n"
                                               "1\n"
                                               "default:\n"
                                               "angle=0\n"
                                               "switch:\n"
                                               "type=\"Julia\"\n",
        Options{})};

    ASSERT_TRUE(result);
    EXPECT_FALSE(result->per_image);
    ast::Expr builtin = result->builtin;
    EXPECT_TRUE(builtin);
    EXPECT_EQ("setting:type=1\n", to_string(builtin));
    EXPECT_FALSE(result->initialize);
    EXPECT_FALSE(result->iterate);
    EXPECT_FALSE(result->bailout);
    EXPECT_TRUE(result->perturb_initialize);
    EXPECT_TRUE(result->perturb_iterate);
    EXPECT_TRUE(result->defaults);
    EXPECT_TRUE(result->type_switch);
}

static BuiltinDisallowsParam s_builtin_disallows[]{
    {"global",
        "global:\n"
        "1\n"
        "builtin:\n"
        "type=1\n"},
    {"init",
        "builtin:\n"
        "type=1\n"
        "init:\n"
        "1\n"},
    {"loop",
        "builtin:\n"
        "type=1\n"
        "loop:\n"
        "1\n"},
    {"bailout",
        "builtin:\n"
        "type=1\n"
        "bailout:\n"
        "1\n"},
};

TEST_P(BuiltinDisallows, parse)
{
    const BuiltinDisallowsParam &param{GetParam()};
    const ParserPtr parser{create_parser(param.text, Options{})};

    const ast::FormulaSectionsPtr result{parser->parse()};

    ASSERT_FALSE(result) << "Formula should not have parsed";
    ASSERT_FALSE(parser->get_errors().empty()) << "parser should have produced an error";
    const Diagnostic &error{parser->get_errors().back()};
    EXPECT_EQ(ErrorCode::BUILTIN_SECTION_DISALLOWS_OTHER_SECTIONS, error.code);
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, BuiltinDisallows, ValuesIn(s_builtin_disallows));

TEST(TestParse, emptyInit)
{
    const ast::FormulaSectionsPtr result{parse(":|imag(pixel)|<p1", Options{})};

    ASSERT_TRUE(result) << "parse failed";
    EXPECT_TRUE(result->initialize);
    EXPECT_TRUE(result->iterate);
    EXPECT_TRUE(result->bailout);
}

class ParsedFormulaSuite : public TestWithParam<ExpressionParam>
{
};

TEST_P(ParsedFormulaSuite, parse)
{
    const ExpressionParam &param{GetParam()};
    const ParserPtr parser{create_parser(param.text, Options{})};
    ASSERT_TRUE(parser) << "couldn't create parser for '" << param.text << "'";

    const ast::FormulaSectionsPtr result{parser->parse()};

    EXPECT_TRUE(get_section(result, param.section)) << "should have parsed section " << to_string(param.section);
}

INSTANTIATE_TEST_SUITE_P(TestParsedFormula, ParsedFormulaSuite, ValuesIn(g_expression_params));

///////////////////////////////////////////////////////////////////////////////
// Extension keywords as identifiers
///////////////////////////////////////////////////////////////////////////////

struct ExtensionKeywordIdentifierParam
{
    std::string_view keyword;
};

inline void PrintTo(const ExtensionKeywordIdentifierParam &param, std::ostream *os)
{
    *os << param.keyword;
}

class ExtensionKeywordIdentifier : public TestWithParam<ExtensionKeywordIdentifierParam>
{
};

static ExtensionKeywordIdentifierParam s_extension_identifier_keywords[] = {
    {"true"}, {"false"}, {"color"},                                          //
    {"param"}, {"endparam"}, {"while"}, {"endwhile"}, {"repeat"}, {"until"}, //
    {"func"}, {"endfunc"}, {"heading"}, {"endheading"},                      //
};

TEST_P(ExtensionKeywordIdentifier, canBeUsedAsIdentifierWhenExtensionsDisabled)
{
    const ExtensionKeywordIdentifierParam &param = GetParam();
    std::string text = std::string(param.keyword) + "=1";
    Options options;
    options.dialect = Dialect::BASIC;
    ParserPtr parser = create_parser(text, options);

    const ast::FormulaSectionsPtr result = parser->parse();

    ASSERT_TRUE(result) << "Should parse successfully with extensions enabled: " << param.keyword;
    ASSERT_TRUE(result->bailout);
}

TEST_P(ExtensionKeywordIdentifier, cannotBeUsedAsIdentifierWhenExtensionsEnabled)
{
    const ExtensionKeywordIdentifierParam &param = GetParam();
    std::string text = std::string(param.keyword) + "=1";
    ParserPtr parser = create_parser(text, Options{});

    const ast::FormulaSectionsPtr result = parser->parse();

    EXPECT_FALSE(result) << "Should not parse with extensions disabled: " << param.keyword;
    EXPECT_FALSE(parser->get_errors().empty());
}

INSTANTIATE_TEST_SUITE_P(TestFormulaParse, ExtensionKeywordIdentifier, ValuesIn(s_extension_identifier_keywords));

} // namespace formula::test
