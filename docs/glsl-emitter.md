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

Keep image coloring outside the core BASIC emitter. The shader writes one
`FormulaResult` per pixel to storage buffer binding 2:

- `vec2 z`: final formula value
- `vec2 bailout`: last bailout expression value
- `uint iter`: final iteration count
- `uint escaped`: 1 if the loop exited because the bailout test failed

The caller or a later coloring pass can map these values to pixels. The
`imageStore` output is debug-only iteration coloring on image binding 0.

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

### 1. Add Interpreter Equivalence Fixtures

- Build a small corpus of BASIC formulas with known interpreter results.
- For each formula, assert emitted GLSL contains the expected lowered
  operations.
- If a GLSL execution harness becomes available, compare numeric results
  against interpreter output.

### 2. Remove Example-Only Caveats

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
