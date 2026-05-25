# BASIC Parser Verification Plan

## Summary

BASIC formulas cannot reference user functions, other formulas, classes,
imports, parameters, or extended formula metadata. Every valid BASIC construct
should therefore be verifiable during parsing. No semantic analyzer should be
needed for BASIC formulas.

The parser should reject BASIC inputs that would otherwise fail later in the
interpreter or compiler. Unknown variables remain valid and evaluate as zero.

Extended formulas add syntax and entity references on top of BASIC semantics.
They do not weaken BASIC rules. Any constraint described here for BASIC
expressions, builtin functions, builtin variables, assignment, precedence, or
runtime operator behavior should also hold for extended formulas unless a later
Ultra Fractal feature explicitly extends that specific construct.

The BASIC language semantics are described in `basic-formula.txt`. The
functions listed there are the complete BASIC builtin function set. Each listed
builtin function takes exactly one argument, even where the text does not show
an explicit argument. A call written as `f(x, y)` with numeric literal parts is
the BASIC spelling of a one-argument call with the complex literal `(x, y)`.

## BASIC Builtins

The parser should treat these predefined variables as known BASIC names:

- `z`
- `p1`, `p2`, `p3`, `p4`, `p5`
- `pixel`
- `lastsqr`
- `rand`
- `pi`
- `e`
- `maxit`
- `scrnmax`
- `scrnpix`
- `whitesq`
- `ismand`
- `center`
- `magxmag`
- `rotskew`

`z` is mutable. The other predefined variables are read-only and should not be
valid assignment targets.

Variable lookup remains permissive. Any other identifier is a user variable and
is initialized to zero by runtime behavior.

For validation, use lowercase builtin names in code. The documentation shows
`LastSqr` in mixed case, but the parser should validate against `lastsqr`.

All BASIC variable names, function names, and keywords match
case-insensitively. Preserve identifier spelling in the AST and diagnostics,
but normalize names for validation.

The parser should treat these names as the complete BASIC builtin function set:

- `sin`, `cos`, `sinh`, `cosh`, `cosxx`
- `tan`, `cotan`, `tanh`, `cotanh`
- `sqr`, `log`, `exp`, `abs`, `conj`, `real`, `imag`, `flip`
- `fn1`, `fn2`, `fn3`, `fn4`
- `srand`
- `asin`, `asinh`, `acos`, `acosh`, `atan`, `atanh`
- `sqrt`, `cabs`
- `floor`, `ceil`, `trunc`, `round`
- `ident`, `zero`, `one`

Each builtin function must parse with exactly one argument. This includes
`zero` and `one`. A numeric pair written directly inside the call parentheses,
such as `sin(3, 4)`, is one complex literal argument and should parse as
`sin((3, 4))`.

## Parse Mode

BASIC has one parser contract. It should reject:

- Unknown function calls.
- Builtin calls with anything other than one argument.
- Assignment to read-only builtin variables.
- Assignment to builtin function names.
- Any extended-only syntax.

The same static restrictions should be enforced in extended formulas for
shared BASIC constructs. Extended-only constructs can add valid forms, such as
user-defined function calls or class member calls, but they should not make
read-only builtin assignment, builtin arity errors, malformed assignment, or
other BASIC syntax violations valid.

## Closed Parser Gaps

The parser now rejects the BASIC constructs that previously required later
runtime behavior:

- Unknown function calls such as `foo(1)`.
- Builtin function calls with anything other than one argument.
- Zero-argument calls such as `sin()`.
- Generic multi-argument calls such as `sin(a, b)`.
- Numeric pair calls such as `sin(1, 2)` are parsed as one complex
  literal argument, matching legacy parser behavior and the `round(2.5, 3.4)`
  example in `basic-formula.txt`.
- Assignment to read-only builtin variables.

## Closed Runtime Semantics Gaps

These are not semantic-analysis requirements, but they must remain locked so
parsed BASIC formulas execute according to `basic-formula.txt`:

- Logical `&&` and `||` short-circuit left to right.
- Power associativity is locked by parser, interpreter, and compiler tests.

## Non-Gaps

These already match BASIC parse-time verification:

- Unknown variables parse as identifiers and evaluate as zero.
- Invalid assignment targets are parser errors.
- Reserved words cannot be assigned unless used as longer identifiers.
- Malformed numeric and complex literals are parser errors.
- BASIC expression operands are numeric or complex after parse.
- Bailout expressions are numeric or complex and use existing truthiness rules.
- Read-only builtin assignment is rejected during parsing.

## Desired BASIC Contract

After parsing succeeds in BASIC dialect:

- All function calls target known BASIC builtins.
- Every function call has exactly one argument.
- Numeric pair calls such as `f(3, 4)` are represented as one complex literal
  argument, equivalent to `f((3, 4))`.
- Variable names, function names, and keywords were matched
  case-insensitively.
- No extended-only nodes are present.
- No import, class, declaration, return, array, member, object, or parameter
  reference syntax is present.
- Assignment targets are syntactically valid BASIC targets.
- Unknown identifiers are variables initialized to zero by runtime behavior.
- Interpreter and compiler do not need to report static BASIC errors.

## BASIC Expression Rules

The parser should enforce the expression rules from `basic-formula.txt`:

- Assignment is a statement operator only.
- Assignment is not valid inside an expression.
- Chained assignment statements such as `z=q=6` are valid.
- Function calls bind tighter than unary operators.
- Unary negation and power bind tighter than multiplication and division.
- Multiplication and division bind tighter than addition and subtraction.
- Addition and subtraction bind tighter than assignment statements.
- Assignment statements bind tighter than comparisons.
- Comparisons bind tighter than logical operators.
- Parentheses override precedence.
- Modulus bars form a parenthetic modulus-squared expression.
- Nested modulus expressions require inner parentheses.

Logical operators short-circuit left to right. This intentionally differs from
`docs/id.txt`; Iterated Dynamics will be updated to match this behavior.

## Parser Changes

### Function Calls

For BASIC constructs, distinguish builtin function calls from generic calls:

- Accept `sin(expr)` and other known builtins.
- Accept `sin(3, 4)` as one complex literal argument, equivalent to
  `sin((3, 4))`.
- Accept case variants of known builtins, such as `Sin(expr)`.
- Reject `foo(expr)` as an unknown function call.
- Reject `sin()`.
- Reject `sin(a, b)`. Only numeric literal pairs get the legacy complex-literal
  treatment.
- Reject bare calls to documented constant-like functions such as `zero()` and
  `one()` without an argument. They are still BASIC builtin functions and take
  one argument by parser contract.

Add parser error codes for:

- Unknown BASIC function call.
- Invalid BASIC function arity.

Continue to use existing delimiter errors for malformed call syntax, such as a
missing close parenthesis.

In extended formulas, generic calls can be valid user-defined function,
constructor, or method calls. Builtin calls should still follow the BASIC
builtin rules above.

### Builtin Assignment

Make the BASIC verification policy explicit:

- Shared BASIC semantics should reject assignment to builtin variables and
  builtin function names, using case-insensitive matching.
- Both BASIC and extended formulas reject invalid builtin assignment in shared
  expression contexts.

Do not use a semantic analyzer to reject read-only builtin assignments.

Add parser error codes for:

- Assignment to a read-only BASIC builtin variable.
- Assignment to a BASIC builtin function name.

Do not report read-only variable assignment as a generic expected-statement
error. Use a specific diagnostic so tests can verify the rule directly.

### Extended Syntax Rejection

Keep rejecting extended-only syntax in BASIC through lexer/parser dialect
checks:

- typed declarations
- arrays
- member access
- parameter refs
- constant refs
- strings
- bool literals
- `new`
- `return`
- loops other than existing BASIC `if`
- functions and classes

Add regression tests for any extended token that currently becomes a generic
BASIC expression by accident.

## Implementation Slices

Before starting code implementation, present the completed BASIC parser plan
for review and wait for approval.

1. Add parse tests that document the intended BASIC contract.
   - Unknown variables still parse and evaluate as zero.
   - Unknown function calls fail during parse.
   - Builtin calls require exactly one argument.
   - Numeric pair calls such as `sin(1, 2)` parse as one complex literal
     argument.
   - Generic multi-argument and zero-argument builtin calls fail during parse.
   - Read-only builtin variable assignment reports a specific parser error.
   - Builtin function assignment reports a specific parser error.

2. Add BASIC-only function-call validation.
   - Reject postfix calls where the callee is not a known BASIC builtin.
   - Reject known builtin calls with argument count other than one.
   - Treat numeric literal pairs inside call parentheses as one complex
     literal argument.
   - Match builtin function names case-insensitively.
   - Preserve extended multi-argument calls.
   - Follow up by applying builtin arity validation to extended builtin calls
     without rejecting extended user-defined calls.

3. Add BASIC builtin-assignment tests.
   - Verify builtin variable assignment is rejected.
   - Verify builtin function assignment is rejected.
   - Verify read-only builtin matching is case-insensitive.
   - Verify the same shared builtin-assignment rules in extended formulas.
   - Remove warning-mode expectations from parser tests.

4. Review default parser options.
   - Remove options that allow builtin assignment in BASIC.
   - Ensure BASIC parse success means full static BASIC validation.

5. Add extended-token regression tests under BASIC.
   - Ensure extended-only tokens remain invalid in BASIC.
   - Ensure no extended AST nodes can be produced in BASIC.

6. Update interpreter and compiler assumptions.
   - Remove or simplify BASIC runtime checks that duplicate parser validation.
   - Keep runtime handling for unknown variables as zero.
   - Keep runtime errors for internal misuse only.

7. Fix BASIC logical evaluation.
   - Ensure interpreted `&&` and `||` short-circuit left to right.
   - Ensure compiled `&&` and `||` short-circuit left to right.
   - Preserve truthiness based on the real part of complex values.

8. Lock BASIC precedence behavior.
   - Add tests for assignment as statement-only syntax.
   - Add tests for chained assignment.
   - Add tests for power, unary negation, multiplication, addition,
     comparison, and logical precedence.
   - Add tests for modulus grouping and nested modulus with parentheses.

Suggested implementation order:

1. Add failing parser tests for BASIC function call validation.
2. Add parser error codes.
3. Implement BASIC function call validation.
4. Add failing parser tests for read-only builtin assignment.
5. Implement BASIC builtin assignment validation.
6. Backfill shared builtin assignment validation onto extended formulas.
7. Backfill shared builtin arity validation onto extended builtin calls.
8. Add case-insensitive matching tests.
9. Normalize BASIC validation lookups while preserving source spelling.
10. Add runtime tests for short-circuit logical evaluation.
11. Fix interpreter and compiler logical evaluation.
12. Add precedence and modulus regression tests.

## Tests

Add parser tests for:

- `a` parses as an identifier.
- `foo(1)` fails in BASIC.
- `sin(1)` parses in BASIC.
- `Sin(1)` parses in BASIC.
- `sin(1, 2)` parses in BASIC as `sin((1, 2))`.
- `round(2.5, 3.4)` parses in BASIC as `round((2.5, 3.4))`.
- `sin()` fails in BASIC.
- `sin(a, b)` fails in BASIC.
- `(sin)(1)` fails in BASIC if representable.
- `z=1` parses in BASIC.
- `Z=1` parses in BASIC.
- `p1=1` fails in BASIC.
- `P1=1` fails in BASIC.
- `sin=1` fails in BASIC.
- `Sin=1` fails in BASIC.
- Extended call syntax remains valid in extended dialect where appropriate.
- Extended builtin calls follow the same builtin arity and numeric-pair rules.
- Extended formulas reject assignment to read-only builtin variables and
  builtin function names.
- `q=3+(w=6)` fails in BASIC.
- `z=q=6` parses in BASIC.
- `0&&srand(1)` skips the right operand in BASIC.
- `1||srand(1)` skips the right operand in BASIC.

Add interpreter/compiler regression tests showing that valid BASIC formulas no
longer rely on runtime detection for unknown functions or bad arity.

## Relationship To Semantic Analysis

The semantic analyzer should not analyze BASIC formulas. BASIC parse success is
the complete static validation boundary.

Extended formulas and resolved parameter sets still require semantic analysis
because they can reference user functions, classes, imported entities, typed
parameters, builtin objects, and cross-entry bindings.
