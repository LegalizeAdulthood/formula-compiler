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
- Runtime inputs bound before `compile()` are embedded in the compiled data
  section.
- Unsupported extended AST nodes make compilation fail.

## Gaps

### Assignment Stores Only Real Values

`AssignmentNode` emits only the low lane to the target symbol. Assigning a
complex value updates the real component but leaves the imaginary component at
its old value or zero.

### Compiled Results Are Incomplete For Some Sections

The compiler returns `_result`, but symbol state is copied back only through
recorded symbol bindings. Complex symbol updates are affected by the assignment
imaginary-store gap.

### `sqr()` Does Not Update `lastsqr`

The interpreter stores the argument modulus squared in `lastsqr` before
returning the square. The compiler calls the shared `sqr` function directly and
does not update `lastsqr`.

### `rand` And `srand()` Are Not Compiled

The interpreter advances `rand` before each iterate and perturb-iterate section.
It also lets `srand()` reset the per-formula random sequence. The compiler does
not advance `rand`, does not support `srand()` as a per-formula seed reset, and
cannot currently update random state from generated code.

### Runtime Inputs Are Frozen At Compile Time

Compiled code embeds symbol values into the generated data section during
`compile()`. Later `set_value()` calls update `m_state.symbols`, but not the
compiled data section. This is already visible in existing tests and should be
made explicit or replaced with live symbol storage so clients can bind per-pixel
values after compiling.

### Recompilation Does Not Reset Compiled Runtime State

`compile()` reuses `m_state.data` and function pointers. Recompiling after
changing selector state, symbol values, or random seed should either rebuild all
compiled data from a clean state or be rejected clearly.

## Implementation Slices

### 1. Store Full Complex Assignments

- Emit both real and imaginary lanes when assigning to a symbol.
- Add compiled tests for assigning complex expressions and reading them back.
- Add a section test where a complex assignment in `loop` is consumed by
  `bailout`.

### 2. Update `lastsqr` For Compiled `sqr()`

- Detect selected or direct `sqr()` calls before invoking the runtime function.
- Emit code that stores the argument modulus squared in `lastsqr`.
- Add compiled tests matching interpreter `sqr` and selected-`sqr` behavior.

### 3. Define Live Runtime Input Binding

- Decide whether compiled formulas support `set_value()` after `compile()`.
- If supported, store compiled symbols in memory owned by `m_state.symbols` so
  later client bindings are visible without recompilation.
- If not supported, document and enforce the pre-compile binding contract.
- Add tests for `pixel`, `p1` through `p5`, and the documented rendering inputs.

### 4. Compile `rand` Advancement

- Move per-formula random state into a helper callable from generated code.
- Advance `rand` before compiled iterate and perturb-iterate sections.
- Keep startup `rand` as `(0, 0)` until the first iteration.
- Add deterministic tests using `set_random_seed`.

### 5. Compile `srand()`

- Add a generated-call path that resets the per-formula random state and
  returns `(0, 0)`.
- Verify that a formula-level `srand(seed)` overrides the client seed.
- Add repeatability tests shared with interpreter expectations.

### 6. Reset State On Recompile

- Clear compiled data bindings and function pointers before rebuilding code.
- Preserve user-visible runtime state that should survive recompilation.
- Add tests that compile, change function selectors or values, recompile, and
  verify the new compiled behavior.

### 7. Expand Interpreter/Compiler Parity Fixtures

- Add paired tests for every BASIC runtime semantic in `basic-interpreter.md`.
- Keep direct AST-only tests only for semantics that parsed BASIC formulas
  cannot express.
- Leave GLSL-specific parity work in `glsl-emitter.md`.

## Verification

- Run `cmake --workflow rt-default` after each implementation slice.
- Keep source text ASCII.
- Normalize touched text files to CRLF on Windows.
