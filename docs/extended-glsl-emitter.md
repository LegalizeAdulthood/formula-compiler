# Extended GLSL Emitter Plan

## Summary

Add an extended formula GLSL source translator with a client-facing facade
similar to the extended interpreter:

- Header: `libs/translator/include/formula/translator/ExtendedGLSLEmitter.h`
- Implementation: `libs/translator/ExtendedGLSLEmitter.cpp`

The facade owns parse, reference resolution, semantic analysis, parameter
metadata collection, default binding, and shader source generation for one
extended formula entry. It does not own OpenGL objects, command submission,
pixel scheduling, textures, buffers, or image orchestration.

The first target is correct source generation and stable client ABI metadata.
Performance optimization comes after semantic parity and test coverage.

## Goals

- Preserve BASIC GLSL emitter behavior.
- Reuse the existing parser, resolver, semantic analyzer, and parameter
  metadata collection.
- Match extended interpreter semantics wherever GLSL can represent them.
- Expose parameter binding through clean parameter APIs, not `p_` or `f_`
  parameter-set prefixes.
- Generate GLSL only after parse, resolution, semantic analysis, and binding
  validation succeed.
- Return diagnostics and shader ABI metadata instead of requiring clients to
  inspect generated source.
- Keep the client responsible for graphics API objects, texture/buffer
  allocation, dispatch layout, and multi-formula orchestration.

## Non-Goals

- Do not replace the extended interpreter.
- Do not compile to machine code.
- Do not add OpenGL, Vulkan, WebGPU, CUDA, HLSL, or Metal runtime ownership to
  the translator facade.
- Do not require the JIT compiler SSA IR to exist before implementation can
  start.
- Do not generate shaders for unresolved or semantically invalid formulas.
- Do not support every object/class feature in the first slice.

## Current Foundation

- The parser accepts extended syntax and rejects syntax, section-order, entry
  kind, predefined-symbol, and read-only predefined assignment errors.
- Reference resolution loads imported files, records resolved entry/class
  references, and retains only referenced imported class ASTs.
- The semantic analyzer validates formulas and resolved parameter sets without
  mutating ASTs.
- The extended interpreter facade already performs the cold pipeline in its
  constructor and exposes parameter metadata plus binding APIs.
- The BASIC GLSL emitter already has shader helper coverage, optional
  `glslangValidator` validation, and an OpenGL smoke example.

## Public API Shape

Use a new facade rather than extending the BASIC emitter entry point.

```cpp
struct ExtendedGLSLEmitterOptions
{
    Options parser;
    const BuiltinRegistry *builtins{};
};

struct GLSLBinding
{
    string name;
    ValueKind kind{};
    unsigned binding{};
    unsigned offset{};
    unsigned size{};
};

struct GLSLShaderSource
{
    Section section{};
    string source;
    vector<GLSLBinding> uniforms;
    vector<GLSLBinding> storage_buffers;
    vector<GLSLBinding> images;
    vector<Diagnostic> diagnostics;
};

class ExtendedGLSLEmitter
{
public:
    ExtendedGLSLEmitter(FileEntry entry, ExtendedGLSLEmitterOptions options);

    const vector<Diagnostic> &diagnostics() const;
    bool ok() const;
    const vector<FormulaParameterInfo> &parameters() const;

    void set_value(string_view name, Value value);
    void set_parameter(string_view name, Value value);
    void set_function_parameter(string_view name, string_view target);
    void set_plugin_parameter(string_view name, string_view selector);
    void set_plugin_parameter_value(
        string_view plugin_name, string_view nested_name, Value value);

    GLSLShaderSource emit(Section section) const;
};
```

Notes:

- The actual header may refine names, but the facade shape should stay close
  to the interpreter.
- `set_parameter` uses source parameter names. `@name` is formula syntax; `p_`
  and `f_` are extended parameter-set file-format conventions.
- Constructor diagnostics cover parse, load, resolution, semantic analysis,
  unsupported formula kinds, and unsupported codegen features known before
  emission.
- `emit` returns source plus binding metadata. It never creates graphics API
  objects.

## Client Flow

1. Construct `ExtendedGLSLEmitter` from a formula `FileEntry` and options.
2. Inspect `diagnostics()` and stop if `ok()` is false.
3. Query `parameters()` to discover bindable formula parameters.
4. Bind client values with `set_value`, `set_parameter`,
   `set_function_parameter`, `set_plugin_parameter`, and
   `set_plugin_parameter_value`.
5. Call `emit(section)` for the section shader needed by the client pipeline.
6. Inspect returned diagnostics and binding metadata.
7. Compile the returned GLSL source with the client graphics API.
8. Allocate and bind buffers, uniforms, textures, images, and storage outputs
   using the returned metadata.
9. Dispatch work according to the client image architecture.

## Architecture

The translator should have explicit phases:

1. Facade cold pipeline.
2. Binding state.
3. Codegen capability checks.
4. Typed lowering model.
5. GLSL ABI layout.
6. GLSL source emission.
7. Validation hooks.

The typed lowering model should not be an ad-hoc AST visitor. It should record:

- typed values
- lvalues
- scopes
- sections
- user functions
- parameters
- constants
- arrays
- object handles
- image handles
- helper calls
- source locations for diagnostics

The first implementation can use a GLSL-specific typed model. Later work may
replace or feed it from a shared SSA IR if the extended JIT compiler IR becomes
available.

## Shader ABI

Return binding metadata for every client-visible resource:

- scalar uniforms
- complex uniforms
- color uniforms
- string selector data, if supported
- function selector uniforms
- plugin selector uniforms
- image parameters
- input/output predefined values
- storage buffers for arrays or result records
- diagnostics/debug buffers, if added

Do not require clients to reverse-engineer uniform block layout from shader
text.

Prefer stable generated names:

- parameter values: `u_param_<name>`
- predefined values: `u_predef_<name>`
- images: `u_image_<name>`
- storage buffers: `b_<purpose>`
- helper functions: `fc_<name>`
- generated locals: `_fc_<id>`

Preserve source spelling in diagnostics and metadata. Generated GLSL names may
be sanitized.

## Section Outputs

Section output shape is section-specific:

- Fractal `init`, `loop`, `bailout`, `perturbinit`, and `perturbloop` emit
  state update shaders or helper functions suitable for client iteration.
- Coloring `final` emits a color-producing shader function or compute shader.
- Transformation `transform` emits a pixel/solid update shader.
- `global` emits one-time state initialization code when a formula uses it.

The facade should not force a single renderer architecture. It should return
enough ABI metadata for clients to compose per-pixel, per-layer, or tiled
pipelines.

## Unsupported Handling

Unsupported codegen constructs should become diagnostics before source is
returned. Do not emit misleading partial GLSL.

Initial unsupported diagnostics are acceptable for:

- user classes and user object allocation
- dynamic dispatch
- strings as runtime values
- dynamic arrays
- recursive user functions
- `print`
- image methods not yet represented in the image ABI
- plug-in object methods before object lowering exists

## Implementation Slices

### 1. Public Header And Build Wiring

- Add `ExtendedGLSLEmitter.h` under the translator include directory.
- Add `ExtendedGLSLEmitter.cpp` under the translator library.
- Add the files to translator CMake.
- Add a no-op facade with constructor, diagnostics, `ok`, parameter query,
  binding methods, and `emit`.
- Add translator tests that construct the facade and verify the initial
  unsupported diagnostic behavior.

### 2. Facade Cold Pipeline

- Run parser, reference resolution, and semantic analysis in the constructor.
- Collect formula parameter metadata using existing analyzer APIs.
- Preserve diagnostics from each phase in order.
- Store resolved formula/load metadata for later codegen.
- Test parse failures, missing imports, semantic failures, and successful
  construction.

### 3. Binding State

- Reuse interpreter-style runtime binding state for values, parameters,
  function parameters, plugin selectors, and nested plugin parameter values.
- Initialize every parameter from effective UF defaults.
- Validate binding names and type conversions eagerly.
- Store binding diagnostics without throwing.
- Test scalar, complex, color, enum, function, plugin, nested plugin, and
  `Image` parameter bindings.

### 4. Codegen Capability Pass

- Add a read-only AST pass that reports unsupported GLSL constructs before
  lowering.
- Gate by formula kind and requested section.
- Accept a minimal procedural subset first; reject object/class, unsupported
  built-ins, strings, unsupported arrays, and unsupported image operations.
- Test one diagnostic per unsupported construct category.

### 5. GLSL Name And ABI Planner

- Add sanitized generated names and collision handling.
- Build uniform/storage/image binding metadata from predefined values,
  parameters, formula state, arrays, and section outputs.
- Keep layout stable and deterministic.
- Test generated names, binding order, offsets, and metadata for representative
  formulas.

### 6. Typed Lowering Skeleton

- Add typed lowering nodes for literals, identifiers, parameters, predefined
  values, lvalues, assignment, section entry, and section return.
- Preserve source locations.
- Add a lowering verifier for missing types, invalid lvalues, and missing
  section outputs.
- Test lowered shapes without relying on final GLSL source text.

### 7. Scalar Numeric Types

- Lower bool, int, float, and complex values.
- Implement UF numeric promotion and conversion diagnostics for codegen.
- Emit GLSL helpers for complex arithmetic and truthiness.
- Test interpreter-equivalence fixtures for literals, conversions,
  arithmetic, comparisons, and truth tests.

### 8. Color Values

- Lower color literals and color parameters.
- Emit `vec4` color operations and conversions that match interpreter
  semantics.
- Support `rgb`, `rgba`, `hsl`, `hsla`, `red`, `green`, `blue`, `alpha`,
  `hue`, `sat`, and `lum`.
- Test coloring `final` output against interpreter-equivalence fixtures.

### 9. Statements And Control Flow

- Lower statement sequences, typed declarations, assignment, `if`, `while`,
  `repeat/until`, and top-level `return`.
- Emit short-circuit logical evaluation.
- Add loop guard support or a diagnostic when a loop cannot be guarded.
- Test nested blocks, branches, loops, and return behavior.

### 10. User Functions

- Build function tables before section lowering.
- Lower non-recursive user functions with typed arguments and returns.
- Support const and by-value arguments first.
- Add diagnostics for recursion and by-ref arguments until supported.
- Test forward declarations, return conversion, and unsupported recursion.

### 11. By-Reference Arguments

- Extend the lvalue model so function calls can pass writable references.
- Reject const assignment during lowering if semantic metadata is inconsistent.
- Emit GLSL inout parameters where possible.
- Test caller mutation and invalid by-ref targets.

### 12. Static Arrays

- Lower static array declarations, indexing, bounds metadata, and element
  assignment.
- Choose uniform/storage representation for arrays based on size and mutability.
- Emit row-major index flattening.
- Test multidimensional indexing and bounds diagnostics.

### 13. Dynamic Arrays

- Decide whether dynamic arrays are supported in GLSL or rejected for the first
  release.
- If supported, represent them with storage buffers plus explicit length state.
- Lower `setLength` and `length`.
- Test resize, default initialization, and unsupported whole-array assignment.

### 14. Predefined Symbols

- Map supported `#` symbols to explicit shader inputs or outputs by section.
- Reject section-inappropriate predefined symbols through diagnostics.
- Support writable `#pixel` and `#solid` for transform sections.
- Test each predefined symbol documented as supported.

### 15. Built-In Functions

- Lower math built-ins shared with BASIC.
- Add extended built-ins supported by the interpreter.
- Use helper functions for complex or color operations.
- Emit diagnostics for value-polymorphic built-ins that cannot be represented
  yet.
- Test built-in arity/type coverage against semantic analyzer fixtures.

### 16. Function Parameters

- Lower function parameters to selector uniforms and generated dispatch.
- Preserve defaults: `fn1 = sin`, `fn2 = sqr`, `fn3 = sinh`, `fn4 = cosh`;
  other complex function params default to `sin`, and color function params
  default to `mergenormal`.
- Emit diagnostics for selected functions that are not shader-lowerable.
- Test default selectors and client overrides.

### 17. Plug-In Parameters

- Use eager plug-in selector resolution from binding state.
- For selected plug-ins with GLSL-supported procedural bodies, retain and lower
  only referenced formula/class ASTs.
- Emit diagnostics for object plug-ins until object lowering exists.
- Test default selector, client override, missing selector diagnostics, and
  nested plugin parameter binding metadata.

### 18. Builtin Image Class

- Treat `Image` as a builtin resource type, not an imported class.
- Map image parameters to texture/image bindings with explicit sampler and
  format metadata.
- Lower supported `Image` methods and fields to GLSL texture operations.
- Emit diagnostics for unsupported image mutation or format operations.
- Test default empty image metadata, client-bound image metadata, and method
  lowering.

### 19. Object/Class Descriptors

- Build descriptors from retained imported class ASTs for referenced classes.
- Record fields, methods, constructors, inheritance, visibility, and static
  members.
- Emit diagnostics for descriptors that cannot be represented in GLSL yet.
- Test retained-only descriptor construction and unsupported object diagnostics.

### 20. Object Execution

- Lower object allocation, field access, method calls, constructors, casts, and
  plugin object parameters where GLSL representation is viable.
- Choose object storage layout and method dispatch strategy.
- Test simple object construction, inherited fields, methods, casts, and
  unsupported dynamic dispatch.

### 21. Section Shader Emission

- Emit GLSL source for fractal, coloring, and transformation sections.
- Return source plus ABI metadata from `emit`.
- Keep global initialization explicit so clients can run it once.
- Test `init`, `loop`, `bailout`, `final`, and `transform` source emission.

### 22. GLSL Validation

- Add optional `glslangValidator` tests for representative extended shaders.
- Keep tests skipped when the validator is unavailable.
- Add generated-source snapshots only where they stabilize the ABI.
- Test that unsupported formulas do not produce source.

### 23. Interpreter Equivalence

- Add fixtures that run the extended interpreter and compare expected lowered
  GLSL operations or, when a harness is available, GPU numeric output.
- Cover scalar, complex, color, loops, functions, parameters, transforms, and
  images.
- Keep tolerances explicit for floating point comparisons.

### 24. Renderer Example Integration

- Extend or add an example that consumes `ExtendedGLSLEmitter` output and ABI
  metadata.
- Keep graphics object ownership in the example/client, not the facade.
- Run at least one generated extended shader in the smoke path when the local
  OpenGL context supports it.

### 25. Documentation And Cleanup

- Document supported and unsupported extended GLSL surface.
- Document client binding flow and ABI metadata.
- Cross-link `extended-interpreter.md`, `extended-compiler.md`, and
  `glsl-emitter.md`.
- Remove stale temporary diagnostics and example-only comments.

## Testing Strategy

- Unit-test facade construction and diagnostics before source emission.
- Unit-test binding state independently of GLSL text.
- Unit-test capability diagnostics for unsupported nodes.
- Unit-test typed lowering shapes before final source snapshots.
- Snapshot only stable shader helpers and ABI declarations.
- Use interpreter-equivalence fixtures for semantics.
- Use optional `glslangValidator` for source syntax.
- Use OpenGL smoke tests for end-to-end binding and dispatch.
- Run `cmake --workflow rt-default` after every implementation slice.

## Risks

- GLSL cannot naturally represent every extended runtime value.
- Object/class support may need a real object layout before it can generate
  useful shader code.
- Dynamic arrays and strings may be better rejected than partially supported.
- Plugin selector changes may require regenerating shader source.
- Keeping interpreter and GLSL semantics aligned requires shared fixtures.

## Assumptions

- Parser, resolver, and semantic analyzer remain the front-end gate.
- Inputs to codegen have no unresolved reference errors.
- Parameter defaults are complete before emission.
- Lazy plug-in resolution is not needed.
- Clients own rendering architecture and GPU resource lifetimes.
- Correct diagnostics are better than partial source for unsupported features.
