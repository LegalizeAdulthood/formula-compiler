# BASIC Parser Verification Plan

## Summary

BASIC formulas cannot reference user functions, other formulas, classes,
imports, parameters, or extended formula metadata. Every valid BASIC construct
should therefore be verifiable during parsing. No semantic analyzer should be
needed for BASIC formulas.

The parser should reject BASIC inputs that would otherwise fail later in the
interpreter or compiler. Unknown variables remain valid and evaluate as zero.

The BASIC language semantics are described in `basic-formula.txt`. The
functions listed there are the complete BASIC builtin function set. Each listed
builtin function takes exactly one argument, even where the text does not show
an explicit argument.

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
`zero` and `one`.

## Parse Mode

BASIC has one parser contract. It should reject:

- Unknown function calls.
- Builtin calls with anything other than one argument.
- Assignment to read-only builtin variables.
- Assignment to builtin function names.
- Any extended-only syntax.

Parser options that allow builtin assignment should be removed or constrained
outside BASIC once this plan is implemented.

## Current Parser Gaps

The current parser still allows a few BASIC constructs that require later
runtime behavior:

- Unknown function calls such as `foo(1)` parse as generic calls and fail only
  when evaluated or compiled.
- Builtin function calls accept any argument count. BASIC builtins listed in
  `basic-formula.txt` should be one-argument calls.
- Zero-argument calls such as `sin()` can parse, but later code assumes at
  least one argument.
- Multi-argument calls such as `sin(1, 2)` can parse even though BASIC has no
  multi-argument function calls.
- Assignment to read-only builtin variables is allowed by default with
  warnings. BASIC should reject read-only builtin names.

## Runtime Semantics Gaps

These are not semantic-analysis requirements, but they must be fixed so parsed
BASIC formulas execute according to `basic-formula.txt`:

- Logical `&&` and `||` currently short-circuit in the interpreter even though
  BASIC semantics require both operands to be evaluated.
- Power associativity should be checked against the intended BASIC semantics
  and locked with parser/interpreter tests.

## Non-Gaps

These already match BASIC parse-time verification:

- Unknown variables parse as identifiers and evaluate as zero.
- Invalid assignment targets are parser errors.
- Reserved words cannot be assigned unless used as longer identifiers.
- Malformed numeric and complex literals are parser errors.
- BASIC expression operands are numeric or complex after parse.
- Bailout expressions are numeric or complex and use existing truthiness rules.
- Read-only builtin assignment can already be rejected with parser options.

## Desired BASIC Contract

After parsing succeeds in BASIC dialect:

- All function calls target known BASIC builtins.
- Every function call has exactly one argument.
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

Logical operators evaluate both operands. They should not be parsed or lowered
as short-circuit operators for BASIC formulas.

## Parser Changes

### Function Calls

In BASIC dialect, distinguish builtin function calls from generic calls:

- Accept `sin(expr)` and other known builtins.
- Accept case variants of known builtins, such as `Sin(expr)`.
- Reject `foo(expr)` as an unknown function call.
- Reject `sin()`.
- Reject `sin(a, b)`.
- Reject bare calls to documented constant-like functions such as `zero()` and
  `one()` without an argument. They are still BASIC builtin functions and take
  one argument by parser contract.

Add a parser error code for unknown BASIC function calls. Reuse existing arity
or delimiter errors where possible for bad argument lists.

### Builtin Assignment

Make the BASIC verification policy explicit:

- BASIC should reject assignment to builtin variables and builtin function
  names, using case-insensitive matching.
- Existing warning behavior should be removed from the BASIC path.

Do not use a semantic analyzer to reject read-only builtin assignments.

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

1. Add parse tests that document the intended BASIC contract.
   - Unknown variables still parse and evaluate as zero.
   - Unknown function calls fail during parse.
   - Builtin calls require exactly one argument.
   - Multi-argument and zero-argument builtin calls fail during parse.

2. Add BASIC-only function-call validation.
   - Reject postfix calls where the callee is not a known BASIC builtin.
   - Reject known builtin calls with argument count other than one.
   - Match builtin function names case-insensitively.
   - Preserve extended multi-argument calls.

3. Add BASIC builtin-assignment tests.
   - Verify builtin variable assignment is rejected.
   - Verify builtin function assignment is rejected.
   - Verify read-only builtin matching is case-insensitive.
   - Remove warning-mode expectations from BASIC tests.

4. Review default parser options.
   - Remove or narrow options that allow builtin assignment in BASIC.
   - Ensure BASIC parse success means full static BASIC validation.

5. Add extended-token regression tests under BASIC.
   - Ensure extended-only tokens remain invalid in BASIC.
   - Ensure no extended AST nodes can be produced in BASIC.

6. Update interpreter and compiler assumptions.
   - Remove or simplify BASIC runtime checks that duplicate parser validation.
   - Keep runtime handling for unknown variables as zero.
   - Keep runtime errors for internal misuse only.

7. Fix BASIC logical evaluation.
   - Ensure interpreted `&&` and `||` evaluate both operands.
   - Ensure compiled `&&` and `||` evaluate both operands.
   - Preserve truthiness based on the real part of complex values.

8. Lock BASIC precedence behavior.
   - Add tests for assignment as statement-only syntax.
   - Add tests for chained assignment.
   - Add tests for power, unary negation, multiplication, addition,
     comparison, and logical precedence.
   - Add tests for modulus grouping and nested modulus with parentheses.

## Tests

Add parser tests for:

- `a` parses as an identifier.
- `foo(1)` fails in BASIC.
- `sin(1)` parses in BASIC.
- `Sin(1)` parses in BASIC.
- `sin()` fails in BASIC.
- `sin(1, 2)` fails in BASIC.
- `(sin)(1)` fails in BASIC if representable.
- `z=1` parses in BASIC.
- `Z=1` parses in BASIC.
- `p1=1` fails in BASIC.
- `P1=1` fails in BASIC.
- `sin=1` fails in BASIC.
- `Sin=1` fails in BASIC.
- Extended call syntax remains valid in extended dialect where appropriate.
- `q=3+(w=6)` fails in BASIC.
- `z=q=6` parses in BASIC.
- `1&&side_effect` evaluates both operands in BASIC once side-effect tests are
  available.
- `1||side_effect` evaluates both operands in BASIC once side-effect tests are
  available.

Add interpreter/compiler regression tests showing that valid BASIC formulas no
longer rely on runtime detection for unknown functions or bad arity.

## Relationship To Semantic Analysis

The semantic analyzer should not analyze BASIC formulas. BASIC parse success is
the complete static validation boundary.

Extended formulas and resolved parameter sets still require semantic analysis
because they can reference user functions, classes, imported entities, typed
parameters, builtin objects, and cross-entry bindings.
