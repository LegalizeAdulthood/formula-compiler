# Extended Formula Interpreter: Procedural Runtime

## Summary
- Target procedural UF6 runtime first: typed values, declarations,
  arrays, params/constants, loops, returns, user functions, color values,
  and `final`/`transform` sections.
- Defer object/class execution: `new`, member access, methods,
  inheritance, casts, and plug-in object instantiation remain
  unsupported, but imports are resolved before interpretation.
- Semantics source: UF6 docs under `docs/uf6`, especially
  `variables.txt`, `types.txt`, `type-compatibility.txt`, `arrays.txt`,
  `dynamic-arrays.txt`, `loops.txt`, `functions.txt`,
  `function-arguments.txt`, `parameters.txt`, `parameter-blocks.txt`,
  section docs, and `importing-classes.txt`.

## Key Changes
- Add `formula::Value`: `monostate`, `bool`, `int`, `double`,
  `Complex`, `Color`, `std::string`, `ArrayValue`.
- Add public value-aware APIs while preserving current `Complex`
  wrappers:
  - `Formula::set_value(std::string_view, Value)`
  - `Formula::get_runtime_value(std::string_view) const`
  - `Formula::interpret_value(Section)`
  - Existing `set_value(..., Complex)`, `get_value`, `interpret` stay
    compatible.
- Extend `Section` with `FINAL` and `TRANSFORM`; wire `get_section` and
  interpreter dispatch for coloring/transformation entries.
- Add an import-loading pass before interpreter construction: call
  `Options::file_importer` for imported file text, preprocess and parse
  imported entries enough to collect class declarations and import
  metadata. Retain and diagnose referenced imported class ASTs only.
- Keep compiler/JIT unchanged; only interpreter gains extended behavior.

## Runtime Semantics
- Symbols:
  - Identifiers resolve local scope, then formula scope.
  - `@param` resolves parameter values; defaults come from `default:`
    parameter blocks.
  - `#name` resolves predefined/environment symbols using the same
    normalized names as `set_value`.
  - Unknown scalar reads keep current compatibility: zero value.
- Types:
  - Implement UF upward numeric conversion: `int -> float -> complex`.
  - Bool conversion to/from numeric works dynamically; color never
    auto-converts.
  - Invalid conversions throw `std::runtime_error`.
- Declarations:
  - Typed scalars default to zero/false/transparent color/null string
    equivalent.
  - Declaration initializer is coerced to declared type.
  - Untyped assignment keeps existing complex default unless target
    already has a declared type.
- Arrays:
  - Static arrays store dimensions and element type; multidimensional
    indexes flatten row-major.
  - Dynamic arrays start length 0; `setLength(array, n)` resizes and
    zero-initializes new elements.
  - `length(array)` returns current first-dimension length.
  - Dynamic array whole-array assignment throws; static array assignment
    requires same element type and dimensions.
- Statements:
  - `while` repeats while truthy; `repeat/until` executes once, then
    stops when truthy.
  - Add a hard loop guard of `1'000'000` iterations per loop; throw on
    overflow.
  - `return` exits the current function; at top level it exits the
    current section with its value.
  - Function declarations are collected before execution and skipped as
    statements.
- Functions:
  - Calls resolve user functions first, then built-ins.
  - User functions are order-independent and recursive.
  - Arguments are local variables; `&` arguments bind to caller lvalues.
  - `const` arguments reject assignment inside the function.
  - Functions can read/write formula variables; global-section functions
    follow UF rules: callable outside global only if declared `const`.
- Built-ins:
  - Preserve existing math built-ins.
  - Add multi-arg/value built-ins needed by UF6 procedural docs:
    `setLength`, `length`, `rgb`, `rgba`, `hsl`, `hsla`, `red`,
    `green`, `blue`, `alpha`, `hue`, `sat`, `lum`, `random`, `atan2`,
    `isNaN`, `isInf`.
  - `print(...)` formats values and records runtime messages in the
    interpreter state; returns no value.
- Builtin `Image` class:
  - Treat `Image` as a builtin class/type that is not defined in any
    imported file. Parameter blocks such as `Image param imageParam`
    must not produce unresolved-class diagnostics.
  - Add an `ImageValue` runtime handle for image parameters. The handle
    represents either an empty image or host-supplied image data.
  - Initialize image parameters from formula parameter state, not by
    constructing imported class ASTs.
  - Implement the documented image methods and properties from the UF6
    image-parameter docs. Until each operation exists, member access and
    calls on `Image` must throw clear unsupported-node or unsupported
    member errors.
  - Define host APIs for binding image parameter data before execution.
    The default image parameter value is empty.
- Sections:
  - `global:` executes as `PER_IMAGE`; variables written there persist
    and are read-only from other sections.
  - `init`, `loop`, `bailout`, `perturbinit`, `perturbloop`, `final`,
    and `transform` execute on request.
  - `bailout` returns truthiness; `final` may return float/color via
    `interpret_value`; `transform` mutates `#pixel`/`#solid`.
- Imports:
  - `import "file"` is not interpreted as a runtime statement.
  - Imported files are indexed before consistency/type checks. Only
    referenced imported class ASTs are retained and diagnosed.
  - Import order is preserved for later class lookup: last imported file
    wins; an ancestor file in `class X(File.ufm:Base)` is treated as an
    implicit import before explicit imports.
  - Import edges, resolved class references, retained imported ASTs, and
    diagnostics are exposed as load metadata.

## Tests
- Interpreter tests for typed declarations/defaults/coercions and
  invalid conversions.
- Tests for `@param`, `#pixel/#z/#index/#color/#solid`, and default
  param initialization.
- Tests for `while`, `repeat/until`, loop guard, top-level and function
  `return`.
- Tests for user functions: declaration after call, recursion, local
  shadowing, by-ref mutation, const rejection.
- Tests for arrays: static multidimensional indexing, dynamic
  `setLength`, `length`, bounds failures, static copy, dynamic copy
  rejection.
- Tests for builtin `Image`: image param blocks do not resolve through
  imports, default image values are empty, host-bound image data can be
  read, and unsupported image members fail clearly.
- Tests for color construction, extraction, color arithmetic,
  final-section color return.
- Tests for section dispatch: fractal, coloring `final`, transformation
  `transform`, and global read-only behavior.
- Tests for import loading: chained imports, import order, missing file,
  public import metadata, referenced AST retention, and syntax errors in
  referenced imported classes.
- Regression: existing basic interpreter tests still pass unchanged.

## Assumptions
- Scope is procedural first, per user choice.
- `import` is handled by a pre-interpreter load/parse pass; imported
  class bodies are retained and diagnosed only when referenced, and
  object execution remains out of scope.
- Objects/classes remain unsupported in interpreter and throw clear
  runtime errors if evaluated.
- Uninitialized array reads are deterministic zero/default in this
  interpreter, despite UF describing static array contents as
  indeterminate.
