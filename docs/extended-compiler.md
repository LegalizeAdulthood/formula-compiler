# Extended Formula Compiler: JIT Runtime Plan

## Summary
- Target full EXTENDED formula execution in the JIT compiler.
- Preserve existing BASIC compiled behavior and public `Complex` APIs.
- Share semantic rules with the extended interpreter wherever possible.
- Compile procedural semantics first, then object/class semantics.
- Imports are load metadata. The compiler consumes resolved references and
  retained imported ASTs; it does not execute import directives.

## Goals
- Compile the same extended formula surface accepted by the parser.
- Support typed values, declarations, arrays, params/constants, loops,
  returns, user functions, color values, `final`, and `transform`.
- Support object/class execution after the procedural core: fields,
  methods, constructors, inheritance, `new`, member access, casts, and
  plug-in parameters.
- Report unsupported or invalid compiled constructs with stable
  diagnostics instead of silently falling back to wrong code.
- Keep interpreter and compiler results equivalent for shared semantics.

## Non-Goals
- Do not rewrite parsing or import loading.
- Do not inline imported ASTs into the main entry AST.
- Do not require GLSL/shader generation to advance with the JIT work.
- Do not optimize aggressively until correctness and ABI shape are stable.

## Current Baseline
- The compiler currently emits `Complex` expressions and assignments.
- Values are stored as pairs of doubles in XMM registers or data labels.
- Sections compiled today are fractal-oriented: `global`, `init`, `loop`,
  `bailout`, `perturbinit`, and `perturbloop`.
- Extended AST nodes currently fail compilation or are unsupported.
- Public formula execution still exposes `Complex` wrappers around run
  results.

## Architecture
- Add a compiler semantic analysis pass before machine-code emission.
- Lower AST to a typed compiler IR. Avoid emitting directly from raw AST
  for extended features.
- Keep a shared semantic model with the interpreter:
  - value kinds
  - type conversions
  - function lookup
  - scope rules
  - parameter/default handling
  - import reference resolution
- Split JIT into clear phases:
  - load metadata validation
  - semantic analysis
  - typed IR build
  - layout planning
  - code emission
  - runtime binding
- Keep BASIC fast path available while extended JIT matures.

## Value Model
- Add a compiled value representation matching runtime semantics:
  - `void`
  - `bool`
  - `int`
  - `float`
  - `complex`
  - `color`
  - `string`
  - arrays
  - object references
- Preserve current `Complex` memory representation for complex values.
- Represent `bool` as byte or 32-bit integer in memory; normalize to 0/1.
- Represent `int` as signed 32-bit or 64-bit consistently across JIT and
  host helpers.
- Represent `float` as double to match existing numeric behavior.
- Represent `color` as four doubles: red, green, blue, alpha.
- Represent `string` as a managed runtime handle, not an inline value.
- Represent arrays as runtime handles with element type, rank, dimensions,
  and storage pointer.
- Represent objects as runtime handles with class metadata and field
  storage.
- Represent builtin `Image` values as runtime handles to host image data.
  `Image` is a builtin class/type and is not backed by an imported class
  AST.

## Public API
- Preserve:
  - `Formula::set_value(std::string_view, Complex)`
  - `Formula::get_value(std::string_view) const`
  - `Formula::interpret(Section)`
  - `Formula::run(Section)`
- Add value-aware compiled APIs parallel to interpreter APIs:
  - `Formula::set_value(std::string_view, Value)`
  - `Formula::get_runtime_value(std::string_view) const`
  - `Formula::run_value(Section)`
- Keep `run(Section)` as a compatibility wrapper that coerces the section
  result to complex when possible.
- Expose compile diagnostics separately from parse/load diagnostics.
- `load_formula` metadata is an input to compiler setup when imports are
  present.

## Semantic Analysis
- Build symbol tables before emission:
  - formula variables
  - section-local variables
  - function declarations
  - function parameters
  - constants
  - parameters
  - imported classes
- Normalize names using the same rules as parser/interpreter.
- Resolve user functions before built-ins.
- Resolve `@param` and `#constant` references explicitly.
- Resolve object/class references through `LoadedFormula::files` metadata:
  - `import_graph`
  - `resolved_references`
  - `retained_classes`
  - `class_index`
- Reject unresolved classes and missing retained ASTs before codegen.
- Attach source locations to semantic diagnostics.

## Type System
- Implement UF numeric promotions:
  - `bool -> int -> float -> complex`
- `color` does not auto-convert to numeric.
- `string` does not auto-convert except where UF docs define a specific
  built-in behavior.
- Assignment coerces to declared target type.
- Function calls coerce arguments to declared parameter types.
- Return statements coerce to declared return type.
- Section returns use section-specific expected types:
  - fractal bailout: bool/numeric truth
  - coloring final: float, color, or complex-compatible value as allowed
  - transformation transform: void or value ignored after state mutation
- Invalid conversions become compile diagnostics.

## Memory Layout
- Add a layout planner for compiled formula state:
  - global variables
  - section locals
  - parameters
  - constants
  - arrays
  - objects
  - temporaries that must survive helper calls
- Separate mutable formula state from immutable constants.
- Keep labels for scalar data where current JIT uses data labels.
- Add typed slots so emitter knows load/store width and representation.
- Add stack-frame layout for function locals and temporaries.
- Define calling convention for internal compiled functions before
  compiling user functions.

## Runtime Helpers
- Add host helper functions for operations that are hard or unsafe to
  inline initially:
  - array allocation and resizing
  - bounds checks
  - string operations
  - color conversion helpers
  - image parameter access and sampling
  - random number generation
  - object allocation
  - virtual/member method dispatch
  - diagnostic traps
- Keep helper ABI stable and documented.
- Generate direct machine code for hot scalar/complex arithmetic.
- Use helpers first for rare or complex behavior, then optimize later.

## Sections
- Compile all supported entry kinds:
  - fractal
  - coloring
  - transformation
- Compile and dispatch:
  - `global`
  - `init`
  - `loop`
  - `bailout`
  - `perturbinit`
  - `perturbloop`
  - `final`
  - `transform`
- Run `global` once and preserve formula-scope state.
- Enforce read-only global variables from non-global sections if that
  remains the chosen semantic model.
- Preserve current section entry points for BASIC formulas.

## Expressions
- Compile:
  - literals: bool, int, float, complex, string, color
  - identifiers
  - `@param`
  - `#constant`
  - unary `-` and `!`
  - binary arithmetic and comparisons
  - logical `&&` and `||` with short-circuiting
  - assignment expressions
  - member access
  - array indexing
  - `new`
  - function calls with multiple args
- Preserve UF precedence already parsed by AST shape.
- Generate short-circuit control flow for logical operations.
- Generate lvalue/rvalue paths for assignment targets.

## Statements
- Compile:
  - statement sequences
  - typed declarations
  - static array declarations
  - dynamic array declarations
  - `if/elseif/else/endif`
  - `while/endwhile`
  - `repeat/until`
  - `return`
  - function declarations as precompiled definitions, not runtime
    statements
- Add loop guards matching interpreter semantics.
- Top-level `return` exits the current section.
- Function `return` exits the current compiled function.

## Functions
- Build a function table before section emission.
- Support:
  - typed return values
  - void returns
  - const params
  - by-value params
  - by-reference params
  - recursion
  - declaration after call
  - functions inside allowed scopes
- Compile functions to separate JIT labels or functions.
- Define a value ABI for passing and returning non-complex values.
- By-ref arguments pass typed lvalue addresses or handles.
- Const arguments reject stores during semantic analysis.
- Global-section functions follow UF callability rules.

## Arrays
- Static arrays:
  - layout dimensions during semantic analysis
  - flatten indexes row-major
  - bounds-check indexes
  - copy only compatible arrays
- Dynamic arrays:
  - represent as runtime handles
  - `setLength` resizes first dimension
  - `length` returns first-dimension length
  - new elements default-initialize
  - whole-array assignment follows UF/interpreter rules
- Generate helper calls for allocation, resize, and bounds failures.

## Parameters And Constants
- Compile `default:` blocks into parameter metadata and default values.
- Support `@param` as a typed value read.
- Support parameter blocks that reference imported class types as object
  parameter metadata.
- Support `Image` parameter blocks as builtin image parameter metadata.
  They do not resolve through imports, do not require a retained AST, and
  default to an empty host image handle.
- Support `#pixel`, `#z`, `#index`, `#color`, `#solid`, and other
  environment constants as typed state reads/writes where UF permits.
- Preserve current symbol-label behavior for existing complex built-ins.

## Built-Ins
- Preserve current compiled math built-ins.
- Add typed built-ins:
  - `setLength`
  - `length`
  - `rgb`
  - `rgba`
  - `hsl`
  - `hsla`
  - `red`
  - `green`
  - `blue`
  - `alpha`
  - `hue`
  - `sat`
  - `lum`
  - `random`
  - `atan2`
  - `isNaN`
  - `isInf`
  - `print`
- Lower simple numeric built-ins directly when practical.
- Use helpers for value-polymorphic built-ins.
- Define deterministic behavior for random state in compiled formulas.

## Objects And Classes
- Use retained imported AST metadata for referenced class definitions.
- Treat `Image` as a builtin class outside the import graph. It needs a
  builtin descriptor so member access, method calls, and parameter
  binding can share the object/member lowering path without requiring a
  parsed class definition.
- Build class descriptors during semantic analysis:
  - class name
  - base class
  - fields
  - methods
  - constructors
  - visibility
  - static members and methods
- Support inheritance layout:
  - base fields first
  - derived fields after
  - method lookup through class descriptor
- Support `new` by allocating object storage and running constructor.
- Support member access for fields and methods.
- Support casts after object model type checks exist.
- Support plug-in object parameters as object references initialized from
  parameter metadata.
- Support builtin image parameters as image handles initialized from host
  parameter bindings. Add helpers or direct lowering for documented image
  operations. Unsupported image members must produce compile diagnostics
  or runtime traps with stable messages.
- Reject unsupported object features with precise diagnostics until each
  feature is implemented.

## Imports
- Import directives are not executable AST nodes.
- Compiler input is the main AST plus `LoadedFormula::files` metadata.
- Compile only referenced retained imported class ASTs.
- Do not compile unreferenced imported classes.
- Preserve UF class lookup order already resolved by load metadata.
- Diagnostics from missing imports, cycles, parse errors, and unresolved
  class names remain load diagnostics; compiler should not duplicate
  them unless metadata is inconsistent.

## Error Handling
- Replace boolean-only `compile()` failure with structured diagnostics.
- Keep boolean `compile()` for compatibility.
- Include:
  - diagnostic code
  - message/detail
  - source location
  - section/function/class context
- Distinguish:
  - unsupported feature
  - semantic error
  - codegen failure
  - runtime trap generated by compiled code
- Runtime traps should throw or report through a stable helper path.

## Testing
- Keep every existing compiled BASIC test passing.
- Add compiler/interpreter equivalence tests for every extended feature.
- Add compile-failure tests for unsupported object features during the
  staged rollout.
- Add tests for:
  - typed declarations and conversions
  - bool/int/float/complex/color/string literals
  - parameter and constant references
  - static arrays
  - dynamic arrays
  - while/repeat loops
  - returns
  - user functions
  - by-ref and const params
  - multi-arg built-ins
  - final and transform sections
  - import metadata and retained class use
  - object allocation and member access once enabled
- Run:
  `cmake --workflow rt-default`

## Implementation Slices
1. Diagnostics and feature gates.
   - Add compile diagnostics while preserving `compile()` bool.
   - Convert unsupported extended nodes to clear compile diagnostics.

2. Typed compiler IR.
   - Add value kinds, typed operands, lvalues, and conversions.
   - Keep BASIC complex expressions compiling through the new path.

3. Runtime state layout.
   - Add typed slots for formula variables, parameters, constants, and
     section locals.

4. Scalar and color values.
   - Compile bool, int, float, complex, and color literals and
     conversions.

5. Declarations and assignment.
   - Compile typed declarations, default initialization, and typed
     assignment.

6. Control flow.
   - Compile if, while, repeat/until, logical short-circuiting, and loop
     guards.

7. Section dispatch.
   - Add compiled `final` and `transform`; preserve fractal sections.

8. Parameters and constants.
   - Compile `@param`, `#constant`, defaults, and value-aware APIs.

9. User functions.
   - Compile function tables, calls, returns, recursion, const, and
     by-ref arguments.

10. Arrays.
    - Add array handles, static indexing, dynamic resize, bounds checks,
      and array built-ins.

11. Built-ins.
    - Add multi-arg/value built-ins and compiled helper ABI.

12. Import-aware semantic analysis.
    - Consume retained imported AST metadata and resolved references for
      class and plug-in parameter type checks.

13. Builtin Image class.
    - Add builtin image parameter metadata, runtime handles, host binding
      APIs, and helpers for documented image operations.
    - Keep `Image` out of import resolution and retained AST metadata.

14. Object/class descriptors.
    - Build descriptors, layouts, inheritance metadata, and visibility
      checks.

15. Object execution.
    - Compile `new`, constructors, field access, method calls, casts, and
      object plug-in params.

16. Optimization and cleanup.
    - Inline hot helpers, remove redundant conversions, and stabilize
      generated code organization.

## Risks
- Value ABI churn can make early codegen brittle.
- By-ref arguments and object references require a reliable lvalue model.
- Dynamic arrays and strings need ownership rules.
- Object method dispatch may require runtime metadata not present today.
- Keeping interpreter and compiler semantics aligned requires shared
  helpers or shared tests.

## Assumptions
- Parser and load metadata are the source of truth for imports.
- The compiler may initially call helpers for complex extended behavior.
- JIT correctness comes before performance.
- BASIC compiled formulas remain compatible throughout the rollout.
- GLSL generation is separate work.
