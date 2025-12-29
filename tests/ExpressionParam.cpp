// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "ExpressionParam.h"

namespace formula::test
{

// TODO: is power operator left associative or right associative?
// TODO: is an empty section allowed?
std::array<ExpressionParam, 100> g_expression_params{
    ExpressionParam{"one", "1", Section::BAILOUT, 1.0, 0.0},                           //
    {"two", "2", Section::BAILOUT, 2.0, 0.0},                                          //
    {"add", "1+1", Section::BAILOUT, 2.0, 0.0},                                        //
    {"unaryMinusNegativeOne", "--1", Section::BAILOUT, 1.0, 0.0},                      //
    {"multiply", "2*3", Section::BAILOUT, 6.0, 0.0},                                   //
    {"divide", "6/2", Section::BAILOUT, 3.0, 0.0},                                     //
    {"addMultiply", "1+3*2", Section::BAILOUT, 7.0, 0.0},                              //
    {"multiplyAdd", "3*2+1", Section::BAILOUT, 7.0, 0.0},                              //
    {"addAddAdd", "1+1+1", Section::BAILOUT, 3.0, 0.0},                                //
    {"mulMulMul", "2*2*2", Section::BAILOUT, 8.0, 0.0},                                //
    {"twoPi", "2*pi", Section::BAILOUT, 6.2831853071795862, 0.0},                      //
    {"unknownIdentifierIsZero", "a", Section::BAILOUT, 0.0, 0.0},                      //
    {"power", "2^3", Section::BAILOUT, 8.0, 0.0},                                      //
    {"powerLeftAssociative", "2^3^2", Section::BAILOUT, 64.0, 0.0},                    //
    {"powerPrecedence", "2*3^2", Section::BAILOUT, 18.0, 0.0},                         //
    {"modulus", "|-3.0 + flip(-2)|", Section::BAILOUT, 13.0, 0.0},                     //
    {"modulusReal", "|-3.0|", Section::BAILOUT, 9.0, 0.0},                             //
    {"modulusImaginary", "|flip(3)|", Section::BAILOUT, 9.0, 0.0},                     //
    {"modulusZero", "|0|", Section::BAILOUT, 0.0, 0.0},                                //
    {"compareLessFalse", "4<3", Section::BAILOUT, 0.0, 0.0},                           //
    {"compareLessTrue", "3<4", Section::BAILOUT, 1.0, 0.0},                            //
    {"compareLessEqualTrueEquality", "3<=3", Section::BAILOUT, 1.0, 0.0},              //
    {"compareLessEqualTrueLess", "3<=4", Section::BAILOUT, 1.0, 0.0},                  //
    {"compareLessEqualFalse", "3<=2", Section::BAILOUT, 0.0, 0.0},                     //
    {"compareAssociatesLeft", "4<3<4", Section::BAILOUT, 1.0, 0.0},                    //
    {"compareGreaterFalse", "3>4", Section::BAILOUT, 0.0, 0.0},                        //
    {"compareGreaterTrue", "4>3", Section::BAILOUT, 1.0, 0.0},                         //
    {"compareGreaterEqualTrueEquality", "3>=3", Section::BAILOUT, 1.0, 0.0},           //
    {"compareGreaterEqualTrueGreater", "4>=3", Section::BAILOUT, 1.0, 0.0},            //
    {"compareGreaterEqualFalse", "2>=3", Section::BAILOUT, 0.0, 0.0},                  //
    {"compareEqualTrue", "3==3", Section::BAILOUT, 1.0, 0.0},                          //
    {"compareEqualFalse", "3==4", Section::BAILOUT, 0.0, 0.0},                         //
    {"compareNotEqualTrue", "3!=4", Section::BAILOUT, 1.0, 0.0},                       //
    {"compareNotEqualFalse", "3!=3", Section::BAILOUT, 0.0, 0.0},                      //
    {"compareComplexLess", "(1+flip(5))<(2+flip(1))", Section::BAILOUT, 1.0, 0.0},     // 1<2 (ignores imag)
    {"compareComplexGreater", "(2+flip(5))>(1+flip(10))", Section::BAILOUT, 1.0, 0.0}, // 2>1 (ignores imag)
    {"compareComplexEqual", "(1+flip(2))==(1+flip(2))", Section::BAILOUT, 1.0, 0.0},   // both parts match
    {"compareComplexNotEqualSameReal", "(1+flip(2))==(1+flip(3))", Section::BAILOUT,   // different imag
        0.0, 0.0},                                                                     //
    {"compareComplexNotEqualDiffReal", "(1+flip(2))==(2+flip(2))", Section::BAILOUT,   // different real
        0.0, 0.0},                                                                     //
    {"logicalAndTrue", "1&&1", Section::BAILOUT, 1.0, 0.0},                            //
    {"logicalAndFalse", "1&&0", Section::BAILOUT, 0.0, 0.0},                           //
    {"logicalAndPrecedence", "1+2&&3+4", Section::BAILOUT, 1.0, 0.0},                  //
    {"logicalOrTrue", "1||0", Section::BAILOUT, 1.0, 0.0},                             //
    {"logicalOrFalse", "0||0", Section::BAILOUT, 0.0, 0.0},                            //
    {"logicalOrPrecedence", "1+2||3+4", Section::BAILOUT, 1.0, 0.0},                   //
    {"logicalAndComplexTrue", "(1+flip(1))&&(2+flip(2))", Section::BAILOUT, 1.0, 0.0}, //
    {"logicalAndComplexLeftFalse", "0&&flip(1)", Section::BAILOUT, 0.0, 0.0},          //
    {"logicalAndComplexRightFalse", "flip(1)&&0", Section::BAILOUT, 0.0, 0.0},         // flip(1) real=0.0 (false)
    {"logicalAndPureImaginaryTrue", "flip(1)&&flip(2)", Section::BAILOUT, 0.0,
        0.0},                                                           // flip(1) real=0.0 (false), short-circuits
    {"logicalOrComplexTrue", "flip(1)||0", Section::BAILOUT, 0.0, 0.0}, // flip(1) real=0.0, 0 real=0.0, both false
    {"logicalOrComplexBothFalse", "(0+flip(1))||(0+flip(1))", Section::BAILOUT,         // both have real=0.0
        0.0, 0.0},                                                                      //
    {"statementsIterate", "3\n4\n", Section::ITERATE, 3.0, 0.0},                        //
    {"statementsBailout", "3\n4\n", Section::BAILOUT, 4.0, 0.0},                        //
    {"commaSeparatedStatementsIterate", "3,4", Section::ITERATE, 3.0, 0.0},             //
    {"commaSeparatedStatementsBailout", "3,4", Section::BAILOUT, 4.0, 0.0},             //
    {"flip", "flip(1)", Section::BAILOUT, 0.0, 1.0},                                    //
    {"complexAdd", "1+flip(1)", Section::BAILOUT, 1.0, 1.0},                            //
    {"complexSubtract", "1-flip(1)", Section::BAILOUT, 1.0, -1.0},                      //
    {"complexMultiply", "flip(1)*flip(1)", Section::BAILOUT, -1.0, 0.0},                //
    {"complexDivideScalar", "(1+flip(1))/2", Section::BAILOUT, 0.5, 0.5},               //
    {"complexDivide", "(1+flip(1))/(2+flip(2))", Section::BAILOUT, 0.5, 0.0},           //
    {"realTimesComplex", "3*flip(2)", Section::BAILOUT, 0.0, 6.0},                      //
    {"complexTimesReal", "flip(2)*3", Section::BAILOUT, 0.0, 6.0},                      //
    {"complexMinusReal", "(2+flip(3))-5", Section::BAILOUT, -3.0, 3.0},                 //
    {"realMinusComplex", "5-(2+flip(3))", Section::BAILOUT, 3.0, -3.0},                 //
    {"realPlusComplex", "5+flip(3)", Section::BAILOUT, 5.0, 3.0},                       //
    {"complexPlusReal", "flip(3)+5", Section::BAILOUT, 5.0, 3.0},                       //
    {"realDivideComplex", "2/(1+flip(1))", Section::BAILOUT, 1.0, -1.0},                // 2/(1+i) = 1-i
    {"globalSection", "global:\n1\n", Section::PER_IMAGE, 1.0, 0.0},                    //
    {"initSection", "init:\n2\n", Section::INITIALIZE, 2.0, 0.0},                       //
    {"loopSection", "loop:\n3\n", Section::ITERATE, 3.0, 0.0},                          //
    {"bailoutSection", "bailout:\n4\n", Section::BAILOUT, 4.0, 0.0},                    //
    {"perturbInitSection", "perturbinit:\n5\n", Section::PERTURB_INITIALIZE, 5.0, 0.0}, //
    {"perturbLoopSection", "perturbloop:\n6\n", Section::PERTURB_ITERATE, 6.0, 0.0},    //
    {"powerISquared", "flip(1)^2", Section::BAILOUT, -1.0, 0.0},                        //
    {"powerZeroZero", "0^0", Section::BAILOUT, 1.0, 0.0},                               //
    {"powerZeroOne", "0^1", Section::BAILOUT, 0.0, 0.0},                                //
    {"powerZeroTwo", "0^2", Section::BAILOUT, 0.0, 0.0},                                //
    {"powerComplexExponent", "2^flip(1)", Section::BAILOUT,                             //
        0.76923890136397211, 0.63896127631363475},                                      // e^(i*ln2)
    {"powerComplexBase", "flip(2)^3", Section::BAILOUT, 0.0, -8.0},                     // (2i)^3 = -8i
    {"powerBothComplex", "(1+flip(1))^(1+flip(1))", Section::BAILOUT,                   //
        0.27395725383012109, 0.58370075875861471},                                      //
    {"powerNegativeBase", "(-1)^0.5", Section::BAILOUT, 0.0, 1.0},                      // sqrt(-1) = i
    {"powerOneOneSquared", "(1+flip(1))^2", Section::BAILOUT, 0.0, 2.0},                //
    {"powerNegativeExponent", "2^(-1)", Section::BAILOUT, 0.5, 0.0},                    //
    {"powerComplexNegativeExponent", "flip(1)^(-1)", Section::BAILOUT, 0.0, -1.0},      // 1/i = -i
    {"powerFractionalExponent", "4^0.5", Section::BAILOUT, 2.0, 0.0},                   //
    {"powerComplexFractional", "(4+flip(0))^0.5", Section::BAILOUT, 2.0, 0.0},          //
    {"powerNegativeComplexBase", "(-1-flip(1))^2", Section::BAILOUT, 0.0, 2.0},         // (-1-i)^2 = 2i
    {"unaryMinus", "-5", Section::BAILOUT, -5.0, 0.0},                                  //
    {"unaryMinusComplex", "-(1+flip(1))", Section::BAILOUT, -1.0, -1.0},                //
    {"unaryPlus", "+5", Section::BAILOUT, 5.0, 0.0},                                    //
    {"unaryPlusComplex", "+(1+flip(1))", Section::BAILOUT, 1.0, 1.0},                   //
    {"assignComplexMultiply", "z=(1+flip(1))*(2+flip(2))", Section::BAILOUT, 0.0, 4.0}, //
    {"assignChainedComplex", "a=b=flip(5)", Section::BAILOUT, 0.0, 5.0},                //
    {"assignComplexPower", "z=flip(1)^2", Section::BAILOUT, -1.0, 0.0},                 //
    {"relOpExpr", "c=whitesq*cabs(b4)-(whitesq==0)*sqr(b4)", Section::BAILOUT},         //
    {"modulusConjunctive", "|(1 < 2) && (2 < 3)|", Section::BAILOUT, 1.0, 0.0},         //
    {"fnConjunctive", "real((1 < 2) && (2 < 3))", Section::BAILOUT, 1.0, 0.0},          //
    {"oneVar", "one=1", Section::BAILOUT, 1.0, 0.0},                                    //
    {"zeroVar", "zero=1", Section::BAILOUT, 1.0, 0.0},                                  //
};

} // namespace formula::test
