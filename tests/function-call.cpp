// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "function-call.h"

namespace formula::test
{

std::array<FunctionCallParam, 37> g_calls{
    FunctionCallParam{"abs(-1.0)", {1.0, 0.0}},
    FunctionCallParam{"acos(-1.0)", {3.1415926535897931, 0.0}},
    FunctionCallParam{"acosh(10.0)", {2.9932228461263808, 0.0}},
    FunctionCallParam{"asin(1.0)", {1.5707963267948966, 0.0}},
    FunctionCallParam{"asinh(1.0)", {0.88137358701954305, 0.0}},
    FunctionCallParam{"atan(1.0)", {0.78539816339744828, 0.0}},
    FunctionCallParam{"atanh(0.9)", {1.4722194895832204, 0.0}},
    FunctionCallParam{"cabs(-1.0)", {1.0, 0.0}},
    FunctionCallParam{"ceil(1.1)", {2.0, 0.0}},
    FunctionCallParam{"conj(1.0)", {1.0, 0.0}},
    FunctionCallParam{"cos(pi)", {-1.0, 0.0}},
    FunctionCallParam{"cosh(1)", {1.5430806348152437, 0.0}},
    FunctionCallParam{"cosxx(1)", {0.83373002513114913, 0.0}},
    FunctionCallParam{"cotan(1.0)", {0.64209261593433076, 0.0}},
    FunctionCallParam{"cotanh(1.0)", {1.3130352854993312, 0.0}},
    FunctionCallParam{"exp(1.0)", {2.7182818284590451, 0.0}},
    FunctionCallParam{"flip(1.0)", {0.0, 1.0}},
    FunctionCallParam{"floor(2.1)", {2.0, 0.0}},
    FunctionCallParam{"fn1(1.0)", {1.0, 0.0}},
    FunctionCallParam{"fn2(1.0)", {1.0, 0.0}},
    FunctionCallParam{"fn3(1.0)", {1.0, 0.0}},
    FunctionCallParam{"fn4(1.0)", {1.0, 0.0}},
    FunctionCallParam{"ident(1.0)", {1.0, 0.0}},
    FunctionCallParam{"imag(1.0)", {0.0, 0.0}},
    FunctionCallParam{"log(e)", {1.0, 0.0}},
    FunctionCallParam{"one(0.0)", {1.0, 0.0}},
    FunctionCallParam{"real(1.0)", {1.0, 0.0}},
    FunctionCallParam{"round(1.6)", {2.0, 0.0}},
    FunctionCallParam{"sin(pi/2)", {1.0, 0.0}},
    FunctionCallParam{"sinh(1)", {1.1752011936438014, 0.0}},
    FunctionCallParam{"sqr(2.0)", {4.0, 0.0}},
    FunctionCallParam{"sqrt(4.0)", {2.0, 0.0}},
    FunctionCallParam{"srand(1.0)", {0.0, 0.0}},
    FunctionCallParam{"tan(pi/4)", {1.0, 0.0}},
    FunctionCallParam{"tanh(1.0)", {0.76159415595576485, 0.0}},
    FunctionCallParam{"trunc(1.6)", {1.0, 0.0}},
    FunctionCallParam{"zero(1.0)", {0.0, 0.0}},
};

} // namespace formula::test
