# BASIC Compiler Correctness Plan

## Summary

The BASIC JIT compiler should match the BASIC interpreter semantics documented
in `basic-interpreter.md` and `basic-formula.txt`. Parsing owns static
validation; the compiler should only compile valid BASIC ASTs and clearly reject
extended AST nodes until extended compilation is implemented.

The current compiler covers many expression, section, function-call, and
control-flow cases. The remaining gaps are runtime-state parity and storage
correctness.

## Current Correct Behavior

- Numeric, complex, and complex-literal expressions compile.
- Basic arithmetic, power, comparisons, eager `&&` and `||`, unary negation,
  and modulus squared compile.
- Unknown variables compile as zero through zero-initialized runtime symbols.
- Assignment statements update formula symbols.
- `if`, `elseif`, and `else` compile.
- BASIC function calls dispatch through the shared runtime function table.
- `fn1`, `fn2`, `fn3`, and `fn4` use the same selector state as the
  interpreter and default to `sin`, `sqr`, `sinh`, and `cosh`.
- Direct and selected `sqr()` calls update `lastsqr` with the argument modulus
  squared.
- Assignment statements store both real and imaginary components.
- Runtime inputs and symbols are read from and written to formula-owned symbol
  storage, so `set_value()` calls after `compile()` are visible to compiled
  code.
- `rand` advances before compiled iterate and perturb-iterate sections, starts
  as `(0, 0)`, and uses the client-supplied seed from `set_random_seed()`.
- Compiled `srand()` resets the per-formula random sequence, resets `rand` to
  `(0, 0)`, and returns `(0, 0)`.
- Unsupported extended AST nodes make compilation fail.

## Gaps

### Recompilation Does Not Reset Compiled Runtime State

`compile()` reuses `m_state.data` and function pointers. Recompiling after
changing selector state, symbol values, or random seed should either rebuild all
compiled data from a clean state or be rejected clearly.

## Implementation Slices

### 1. Reset State On Recompile

- Clear compiled data bindings and function pointers before rebuilding code.
- Preserve user-visible runtime state that should survive recompilation.
- Add tests that compile, change function selectors or values, recompile, and
  verify the new compiled behavior.

### 2. Expand Interpreter/Compiler Parity Fixtures

- Add paired tests for every BASIC runtime semantic in `basic-interpreter.md`.
- Keep direct AST-only tests only for semantics that parsed BASIC formulas
  cannot express.
- Leave GLSL-specific parity work in `glsl-emitter.md`.

## Verification

- Run `cmake --workflow rt-default` after each implementation slice.
- Keep source text ASCII.
- Normalize touched text files to CRLF on Windows.
