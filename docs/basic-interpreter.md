# BASIC Interpreter Correctness Plan

## Summary

The BASIC parser now owns static validation. The BASIC interpreter should run
valid BASIC ASTs according to `basic-formula.txt` and should not be responsible
for reporting malformed BASIC syntax.

The interpreter is mostly correct for expression evaluation, assignment,
section dispatch, unknown variables, truthiness, and eager logical operators.
The remaining gaps are runtime-state semantics for selected builtin functions
and predefined variables.

## Current Correct Behavior

- Unknown variables read as complex zero.
- User variables are created by assignment.
- All BASIC values are represented as complex values.
- `z` is mutable; other predefined variables and builtin function names are
  rejected as assignment targets by the parser.
- Assignment is a statement, not an expression.
- Chained assignment statements store the same value in each target.
- `&&` and `||` evaluate both operands.
- Truthiness uses the real part of the complex value.
- Ordering comparisons use real parts.
- Equality comparisons use both real and imaginary parts.
- `|z|` returns modulus squared with zero imaginary part.
- Builtin function names are normalized to lowercase by the parser.
- `f(3, 4)` is parsed as a one-argument call with complex literal `(3, 4)`.
- `cosxx(x + iy)` returns `cos(x) cosh(y) + i sin(x) sinh(y)`.
- `sqr()` updates `lastsqr` with the argument modulus squared before
  returning the square.
- `ismand` defaults to true, represented as `(1, 0)`.
- `lastsqr` defaults to `(0, 0)`.
- `rand` defaults to `(0, 0)` before the first iteration. It advances once
  per iteration section execution to a complex value with both components in
  the range `[0, 1)`. `srand()` resets the per-formula random sequence and
  returns `(0, 0)`.
- The client owns the initial random seed. The runtime uses a stable seed of
  zero until the client calls `set_random_seed` or the formula calls `srand()`.
- The client supplies rendering inputs with `set_value`: `p1`, `p2`, `p3`,
  `p4`, `p5`, `pixel`, `maxit`, `scrnmax`, `scrnpix`, `whitesq`, `center`,
  `magxmag`, `rotskew`, and optional `ismand` override.
- `fn1`, `fn2`, `fn3`, and `fn4` default to `sin`, `sqr`, `sinh`, and `cosh`,
  matching the Id engine defaults. They can be bound by the client to supported
  BASIC builtin functions. Selector names and target names are matched
  case-insensitively and stored in lowercase.

## Gaps

No BASIC interpreter-only gaps remain. JIT and GLSL parity work is tracked
below and in `glsl-emitter.md`.

## Non-Gaps

- Periodicity checking is a renderer responsibility, though it depends on
  formulas leaving the orbit value in `z`.
- `p1` through `p5` defaulting to zero when not supplied follows the general
  unknown-variable behavior.
- `pi` and `e` are already initialized by the formula runtime.
- Parser diagnostics already cover unknown functions, bad builtin arity, and
  invalid assignment targets.

## Implementation Slices

### 1. Keep JIT And GLSL In Sync

- Mirror each corrected BASIC interpreter semantic in the JIT compiler.
- Keep GLSL-specific work in `glsl-emitter.md`.
- Add shared test fixtures where interpreter and compiler should agree.

## Verification

- Run `cmake --workflow rt-default` after each implementation slice.
- Keep source text ASCII.
- Normalize touched text files to CRLF on Windows.
