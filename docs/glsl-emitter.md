# BASIC GLSL Emitter

## Summary

The GLSL emitter is a BASIC fractal formula source translator. It emits a
GLSL 4.50 compute shader for parsed BASIC formulas and follows the same core
numeric semantics as the BASIC interpreter and JIT for the supported surface.

Extended formulas, coloring formulas, transformations, user objects, typed
declarations, arrays, and parameter declarations are out of scope for this
emitter.

## Supported Surface

- BASIC fractal sections: global, init, loop, and bailout.
- Complex values represented as `vec2`.
- User variables and unknown reads initialized to `(0, 0)`.
- BASIC arithmetic, comparisons, logical operators, modulus, conditionals,
  and bailout truthiness.
- BASIC builtin functions, including `fn1` through `fn4` selectors.
- BASIC predefined values exposed through the shader uniform block or derived
  inside the shader.
- Deterministic `rand` and `srand` state from a client-supplied seed.
- Result output through `FormulaResult` records on storage binding 2.
- Debug image output on image binding 0.

The translator is covered by snapshot tests, interpreter-equivalence fixtures,
an optional `glslangValidator` shader compilation test, and the
`glsl-renderer` OpenGL smoke test.

## Unsupported Surface

- EXTENDED syntax and AST nodes.
- Coloring formulas and transformations.
- User-defined functions, methods, classes, objects, imports, and casts.
- Typed declarations and arrays.
- Parameter blocks as executable shader input definitions.
- Host rendering orchestration beyond the smoke-test example.
- Optimized GLSL output.

## Architecture

The emitter uses a BASIC-only lowering layer before string emission.

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

- Operands short-circuit left to right.
- `&&` and `||` combine real-part truthiness.
- Result is `vec2(1.0, 0.0)` or `vec2(0.0, 0.0)`.
- Logical expressions lower to statement-level temporaries and branches so the
  skipped operand is not evaluated.

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

## Verification

- Run `cmake --workflow rt-default` after every implementation slice.
- Keep existing BASIC interpreter and JIT tests passing.
- Add GLSL emitter tests with each slice.
- Keep generated source text ASCII.
- Normalize touched text files to CRLF on Windows.
