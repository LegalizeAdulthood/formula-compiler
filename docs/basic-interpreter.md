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

## Gaps

### `cosxx`

The expected BASIC behavior is:

```text
cosxx(x + iy) = cos(x) cosh(y) + i sin(x) sinh(y)
```

The current implementation returns `cos(z) * cosh(z)`. Existing tests encode
that current behavior for real input, so both implementation and tests need to
be corrected.

### `lastsqr`

`lastsqr` should contain the modulus from the last `sqr()` function call.
Current `sqr()` returns the square but does not update `lastsqr`.

The intended runtime shape is:

- evaluate the `sqr` argument
- store `arg.re * arg.re + arg.im * arg.im` in `lastsqr` as `(value, 0)`
- return `arg * arg`

### `fn1` Through `fn4`

`fn1`, `fn2`, `fn3`, and `fn4` are runtime-selected functions. The current
implementation hardcodes them as identity functions.

The interpreter needs per-formula selector state so a client can bind each
token to one of the supported BASIC builtin functions.

### `rand` And `srand`

`rand` should be a complex random value whose real and imaginary parts are in
the range `[0, 1)`. It should change each iteration. `srand()` should seed the
random sequence.

The current implementation seeds the C library RNG in `srand()`, but `rand`
is only a normal symbol value. It is not generated, advanced, or tied to the
seeded state.

The interpreter should use per-formula random state, not global C RNG state.

### Predefined Runtime Inputs

Some predefined variables are supplied by the rendering client:

- `p1`, `p2`, `p3`, `p4`, `p5`
- `pixel`
- `maxit`
- `scrnmax`
- `scrnpix`
- `whitesq`
- `center`
- `magxmag`
- `rotskew`

The existing `set_value` API can bind these values, but the interpreter does
not make the runtime contract explicit. The plan should document and test the
client-supplied values that must be present for complete image rendering.

`ismand` has a documented default of true. The interpreter should initialize it
to `(1, 0)` unless the client overrides it.

## Non-Gaps

- Periodicity checking is a renderer responsibility, though it depends on
  formulas leaving the orbit value in `z`.
- `p1` through `p5` defaulting to zero when not supplied follows the general
  unknown-variable behavior.
- `pi` and `e` are already initialized by the formula runtime.
- Parser diagnostics already cover unknown functions, bad builtin arity, and
  invalid assignment targets.

## Test Status

Interpreter tests now cover client-supplied predefined values through the
existing `set_value` binding API.

Disabled tests record the currently missing runtime semantics for:

- documented `cosxx` real and complex behavior
- `sqr()` updating `lastsqr`
- `ismand` defaulting to true

Enable those tests as each implementation slice closes the corresponding gap.

## Implementation Slices

### 1. Correct `cosxx`

- Replace the current `cos(z) * cosh(z)` implementation.
- Update real-input expectations so `cosxx(x)` equals `cos(x)`.
- Add complex-input tests for both real and imaginary parts.
- Enable the disabled `cosxx` interpreter tests.

### 2. Add `lastsqr` Runtime State

- Special-case `sqr()` evaluation in the interpreter so it can update
  `lastsqr`.
- Keep `lastsqr` read-only at parse time.
- Add tests that read `lastsqr` after one and multiple `sqr()` calls.
- Add tests that direct `|z|` modulus expressions do not update `lastsqr`.
- Enable the disabled `lastsqr` interpreter tests.

### 3. Initialize Documented Defaults

- Initialize `ismand` to `(1, 0)`.
- Initialize `lastsqr` to `(0, 0)`.
- Decide whether initial `rand` is zero, client-supplied, or generated at
  formula construction. Record that decision in this document before code.
- Enable the disabled `ismand` interpreter test.

### 4. Add Runtime Function Selectors

- Add per-formula selector storage for `fn1`, `fn2`, `fn3`, and `fn4`.
- Add a client API to bind each selector by BASIC builtin function name.
- Validate selector tokens against the BASIC runtime-selectable builtin set.
- Keep default selector behavior as identity for backward compatibility.
- Add tests for selected functions and invalid selector names.

### 5. Add Per-Formula Random State

- Replace `srand()` global C RNG seeding with per-formula seed state.
- Make `srand(seed)` reset that state and return the documented BASIC value
  shape.
- Make `rand` read from per-formula random state.
- Advance `rand` once per iteration section execution.
- Add deterministic tests that seed with `srand()` and verify repeatability.

### 6. Document Runtime Input Contract

- Document which predefined variables are supplied by the client.
- Keep `set_value` as the binding mechanism for complex predefined values.
- Keep the existing round-trip tests for client-supplied predefined values
  passing.

### 7. Keep JIT And GLSL In Sync

- Mirror each corrected BASIC interpreter semantic in the JIT compiler.
- Keep GLSL-specific work in `glsl-emitter.md`.
- Add shared test fixtures where interpreter and compiler should agree.

## Verification

- Run `cmake --workflow rt-default` after each implementation slice.
- Keep source text ASCII.
- Normalize touched text files to CRLF on Windows.
