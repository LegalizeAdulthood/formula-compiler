# BASIC GLSL Emitter Correctness Plan

## Summary

The current GLSL emitter is example code. It should not be treated as a
correct BASIC formula backend until it is rewritten around BASIC formula
semantics, typed lowering, and shader-compilation tests.

The goal of this plan is to make the GLSL emitter correct for BASIC fractal
formulas only. Extended formulas, coloring formulas, transformations, user
objects, typed declarations, arrays, and parameters are out of scope here.

## Goals

- Preserve BASIC parser, interpreter, and JIT semantics.
- Emit syntactically valid GLSL for every parsed BASIC formula supported by
  the interpreter and JIT.
- Match BASIC numeric behavior for complex values, truthiness, comparisons,
  builtins, and predefined variables.
- Reject unsupported shader-generation inputs with diagnostics instead of
  emitting invalid or misleading shader code.
- Add tests that compare GLSL-emitter semantics against existing BASIC
  interpreter expectations wherever possible.

## Non-Goals

- Do not support EXTENDED formula syntax in this emitter pass.
- Do not implement direct-coloring or transformation shader generation.
- Do not implement host GPU integration, pipeline creation, or image upload.
- Do not optimize generated GLSL before correctness is covered.
- Do not reuse this emitter for the extended JIT compiler plan.

## Current Gaps

The current emitter has these BASIC correctness gaps:

- User variables are assigned without declarations.
- Scalar literals are emitted as scalar GLSL values even when complex `vec2`
  values are required.
- Arithmetic helpers are used blindly and can receive scalar operands.
- `|z|` emits magnitude instead of modulus squared.
- `abs(z)` emits magnitude instead of component-wise absolute value.
- Comparisons emit raw GLSL operators against complex values.
- `if` conditions emit expressions directly instead of BASIC real-part
  truthiness.
- Logical operators rely on GLSL short-circuit behavior, unlike BASIC.
- Several predefined variables are missing or have wrong types.
- `pi`, `e`, and `maxit` are not represented as complex values.
- `fn1` through `fn4` are hardcoded identity functions.
- `flip` emits `(-imag, real)` instead of `(imag, real)`.
- `sqr` does not update `lastsqr`.
- `rand` and `srand` are not modeled.
- `scrnmax`, `scrnpix`, `whitesq`, `ismand`, `center`, `magxmag`, and
  `rotskew` are not modeled.
- Output coloring is a placeholder iteration gradient rather than a defined
  client contract.

## Architecture

Add a BASIC-only GLSL lowering layer before string emission.

The lowering layer should build a small typed model:

- `complex`: emitted as `vec2`
- `bool`: emitted as `bool` only for internal branch/control helpers
- `void`: statement-only result

All BASIC expressions have complex runtime value semantics. Comparisons and
logical operators return complex truth values: `(1, 0)` or `(0, 0)`.

Do not infer types from variable assignment. Every BASIC user variable should
be declared as `vec2` the first time it is assigned or referenced as storage.
Unknown reads should resolve to `(0, 0)` if no prior declaration exists.

Keep emission phases explicit:

1. Collect used symbols and builtins from the AST.
2. Build shader prelude and uniforms.
3. Emit helper functions.
4. Emit declarations for formula variables.
5. Emit section bodies.
6. Emit client-defined output plumbing.

## Runtime Model

Represent every BASIC formula value as `vec2`.

Truthiness:

- False is `value.x == 0.0`.
- True is `value.x != 0.0`.
- Imaginary part is ignored for truth tests.

Comparison:

- `<`, `<=`, `>`, and `>=` compare only real parts.
- `==` and `!=` compare both real and imaginary parts.
- Result is `vec2(1.0, 0.0)` or `vec2(0.0, 0.0)`.

Logical operators:

- Both operands must be evaluated.
- `&&` and `||` combine real-part truthiness.
- Result is `vec2(1.0, 0.0)` or `vec2(0.0, 0.0)`.

Modulus:

- `|z|` returns `vec2(dot(z, z), 0.0)`.
- `cabs(z)` returns `vec2(sqrt(dot(z, z)), 0.0)`.
- `abs(z)` returns `vec2(abs(z.x), abs(z.y))`.

`sqr(z)`:

- Returns `z * z`.
- Updates `lastsqr` to `dot(z, z)`.

## Shader Inputs

Expose enough inputs for BASIC predefined variables:

- `p1`, `p2`, `p3`, `p4`, `p5`
- `pixel`
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

The emitter may compute these from lower-level uniforms, but formula code must
see the BASIC names as complex `vec2` values.

`z` is mutable. All other predefined variables are read-only by parser
contract and should not need generated assignment guards.

## Output Contract

Keep image coloring outside the core BASIC emitter until a client contract is
defined.

The BASIC GLSL emitter should initially emit:

- a loop result
- final `z`
- final iteration count
- bailout state

The caller or a later coloring pass can map these values to pixels. If the
current `imageStore` placeholder is kept temporarily, document it as debug
output and test only the generated formula kernel semantics.

## Example Program Framework

Use `glfw3` plus `glad` from vcpkg for the smallest practical
cross-platform OpenGL example program.

- `glfw3` provides window creation, context creation, and minimal input.
- `glad` provides the modern OpenGL function loader needed for shader and
  compute-shader entry points.
- The example should create a hidden GLFW window for automated tests, request
  an OpenGL 4.5 core context, load GLAD, compile the generated shader, execute
  it, and read back pixels or buffer contents.
- Linux CI may need a display provider such as Xvfb.

Prefer these manifest dependencies:

```json
{
  "name": "glfw3"
},
{
  "name": "glad",
  "features": ["gl-api-45", "loader"]
}
```

Avoid SDL2, raylib, and freeglut for this test harness. They either add more
abstraction than needed or are less suitable for modern OpenGL shader tests.

## Implementation Slices

### 1. Mark Current Emitter Unsupported For Production

- Update public comments to state the emitter is experimental.
- Add tests that assert it is not used by the BASIC compiler/JIT path.
- Add a small smoke test that `emit_shader` still returns a shader string.
- Do not attempt correctness in this slice.

### 2. Add GLSL Snapshot Test Harness

- Add tests that parse a BASIC formula, emit GLSL, and compare stable snippets.
- Cover headers, uniforms, helper declarations, and section structure.
- Keep snapshots narrow enough to survive formatting improvements.
- Add a helper that normalizes line endings before comparison.

### 3. Collect Formula Symbols

- Add a pre-pass over BASIC AST sections.
- Collect user variables assigned or referenced.
- Exclude predefined variables and builtin functions.
- Declare collected user variables as `vec2 name = vec2(0.0, 0.0);`.
- Test unknown user variable reads and assigned user variables.

### 4. Emit Complex Literals Everywhere

- Emit integer and floating literals as `vec2(value, 0.0)`.
- Preserve complex literals as `vec2(real, imag)`.
- Add helpers for float constants used inside GLSL helper functions.
- Test `z = 0`, `z + 1`, and `z + (1, 2)`.

### 5. Replace Raw Arithmetic With Complex Helpers

- Emit `+`, `-`, `*`, `/`, and `^` through helpers that all take `vec2`.
- Ensure unary `-` returns `vec2(-x.x, -x.y)`.
- Ensure unary `+` returns the operand unchanged.
- Test addition, subtraction, multiplication, division, and power.

### 6. Correct Modulus And Absolute Builtins

- Emit `|expr|` as `c_mod_sqr(expr)`.
- Emit `abs(expr)` as component-wise absolute value.
- Emit `cabs(expr)` as magnitude.
- Test `|z|`, `abs((3,-4))`, and `cabs((3,4))`.

### 7. Correct Comparison Operators

- Add helper functions for all comparisons.
- Ordering helpers compare real parts only.
- Equality helpers compare both components.
- Return complex truth values.
- Test `<`, `<=`, `>`, `>=`, `==`, and `!=`.

### 8. Correct Logical Operators

- Emit logical operators through helper calls, not GLSL `&&` or `||`.
- Force both operands to be evaluated before the helper call.
- Return complex truth values.
- Document that observable short-circuit tests need extended user functions;
  BASIC has no expression side effects.
- Test generated code shape for eager evaluation.

### 9. Correct Conditional Emission

- Emit `if (c_truth(expr))`.
- Ensure empty then/else branches preserve BASIC result behavior only where
  section result matters.
- Test `if`, `elseif`, `else`, and nested blocks.

### 10. Implement Complete BASIC Builtin Mapping

- Emit helpers for all BASIC builtin functions listed in `basic-formula.txt`.
- Correct `flip` to return `vec2(z.y, z.x)`.
- Correct `cosxx` to match documented BASIC behavior.
- Keep builtin function names normalized to lowercase in emitted helper calls.
- Test every builtin at least for symbol mapping.

### 11. Model `lastsqr`

- Emit `lastsqr` as `vec2(lastsqr_value, 0.0)` when read.
- Store the scalar backing value separately.
- Make `sqr(expr)` update `lastsqr_value` from its input before returning the
  square.
- Test formulas that read `lastsqr` after `sqr`.

### 12. Add Runtime-Selected `fn1` Through `fn4`

- Add uniform selectors for `fn1`, `fn2`, `fn3`, and `fn4`.
- Emit dispatch helpers that call the selected builtin.
- Limit selectors to the BASIC runtime-selectable function set.
- Test emitted dispatch for each selector token.

### 13. Add Random Support

- Add deterministic per-pixel random state.
- Implement `rand` as current random value.
- Implement `srand(seed)` to reset random state and return the documented
  value shape.
- Define how client seed input enters the shader.
- Test emitted state updates for `rand` and `srand`.

### 14. Add Remaining Predefined Variables

- Emit or compute `scrnmax`, `scrnpix`, `whitesq`, `ismand`, `center`,
  `magxmag`, and `rotskew`.
- Keep all predefined variables as `vec2`.
- Test each predefined variable appears with correct initialization.

### 15. Define Section Result And Output ABI

- Decide the shader ABI for loop result, final `z`, iteration count, and
  bailout result.
- Keep debug image output separate from formula result storage.
- Update `emit_shader` docs to describe what the shader writes.
- Test the generated output block.

### 16. Add GLSL Compile Validation

- Add an optional test path that runs a GLSL validator when available.
- Keep the test skipped if the validator is not installed.
- Validate emitted shaders for representative BASIC formulas.
- Include formulas with user variables, conditionals, builtins, and bailout.

### 17. Add Interpreter Equivalence Fixtures

- Build a small corpus of BASIC formulas with known interpreter results.
- For each formula, assert emitted GLSL contains the expected lowered
  operations.
- If a GLSL execution harness becomes available, compare numeric results
  against interpreter output.

### 18. Remove Example-Only Caveats

- Once tests cover the supported BASIC surface, replace example caveats with
  a precise supported/unsupported feature list.
- Keep unsupported EXTENDED features explicit.
- Ensure docs and header comments match actual behavior.

## Verification

- Run `cmake --workflow rt-default` after every implementation slice.
- Keep existing BASIC interpreter and JIT tests passing.
- Add GLSL emitter tests with each slice.
- Keep generated source text ASCII.
- Normalize touched text files to CRLF on Windows.
