# Extended Formula Interpreter: Procedural Runtime

## Summary
- Procedural UF6 runtime is implemented for typed values, declarations,
  arrays, params/constants, loops, returns, user functions, color values,
  builtin `Image`, and `final`/`transform` sections.
- Defer user object/class execution: user `new`, member access, methods,
  inheritance, casts, and plug-in object instantiation remain
  unsupported, but imports are resolved before interpretation. The builtin
  `Image` object has runtime support.
- Treat the existing `Formula` interface as the BASIC formula execution
  surface. Extended execution uses a separate value-aware interpreter
  facade.
- Semantics source: UF6 docs under `docs/uf6`, especially
  `variables.txt`, `types.txt`, `type-compatibility.txt`, `arrays.txt`,
  `dynamic-arrays.txt`, `loops.txt`, `functions.txt`,
  `function-arguments.txt`, `parameters.txt`, `parameter-blocks.txt`,
  section docs, and `importing-classes.txt`.

## Current Foundation
- The extended parser already accepts the syntax needed by this plan:
  typed declarations, arrays, dynamic array calls, statement blocks,
  loops, returns, functions, function arguments, parameter references,
  predefined-symbol references, literals, member access, calls, casts,
  `new`, class syntax, imports, switch, perturb, `final`, `transform`,
  visibility sections, and kind-specific section ordering.
- The parser already rejects malformed syntax, invalid section order and
  cardinality, invalid entry-kind section use, unknown predefined-symbol
  names, and assignment to globally read-only predefined symbols.
- The loader/reference layer already records import metadata, source
  filenames, class references, entry references, resolved references,
  import diagnostics, and retained imported class ASTs. Retention is lazy:
  only referenced imported classes are kept.
- The semantic analyzer already validates extended formulas and resolved
  extended parameter sets. It checks names, types, duplicate symbols,
  calls, conversions, returns, arrays, dynamic-array calls, class bases,
  inheritance cycles, member access, `new`, casts, builtin `Image`, section
  rules, predefined-symbol context, formula kind compatibility, saved
  parameters, enums, function parameters, plug-in parameters, nested
  plug-in assignments, and complete retained reference graphs.
- The semantic analyzer is diagnostic-only. It does not mutate ASTs, create
  a typed semantic model, execute formulas, or generate code.
- The semantic analyzer exposes `collect_formula_parameters`; the extended
  interpreter calls it during construction and exposes the result through
  `parameters()`.
- The extended interpreter supports procedural runtime features and builtin
  `Image`. The `Formula` interpreter/compiler remain BASIC-oriented, and
  the extended compiler does not exist yet.

## Current API
- `Value` supports `monostate`, `bool`, `int`, `double`, `Complex`,
  `Color`, `std::string`, `ArrayValue`, and `ImageValue`.
- `ExtendedInterpreter` is the C++17 facade for interpreting one extended
  formula entry:

  ```cpp
  class ExtendedInterpreter
  {
  public:
      ExtendedInterpreter(FileEntry entry, ExtendedInterpreterOptions options);

      const std::vector<Diagnostic> &diagnostics() const;
      bool ok() const;
      const std::vector<FormulaParameterInfo> &parameters() const;

      void set_value(std::string_view name, Value value);
      void set_parameter(std::string_view name, Value value);
      void set_function_parameter(std::string_view name, std::string_view target);
      void set_plugin_parameter(std::string_view name, std::string_view selector);
      void set_plugin_parameter_value(
          std::string_view plugin_name, std::string_view nested_name, Value value);
      Value value(std::string_view name) const;
      const std::vector<std::string> &messages() const;

      Value interpret(Section section);
  };
  ```

  The constructor runs the cold pipeline for one entry: parse, resolve
  referenced files/classes, analyze semantics, collect parameter metadata,
  and initialize default runtime state. `interpret` runs one section and
  fails if diagnostics contain errors. The facade owns formula evaluation
  state, but not image rendering, pixel scheduling, tiling, threading, or
  layer orchestration.
- Parameter binding uses clean parameter-specific APIs by source parameter
  name. `@name` remains formula-language syntax; `p_` and `f_` remain
  extended parameter-set file-format prefixes.
- `Section` already includes `FINAL` and `TRANSFORM`; extended interpreter
  dispatch supports coloring and transformation entries.
- The existing import-loading and reference-resolution layer runs before
  interpreter construction. Imported files are parsed and indexed enough to
  collect class declarations and import metadata. Referenced imported class
  ASTs are retained and diagnosed by existing load and semantic passes.
- Formula file import APIs return `std::optional<std::string>`; `std::nullopt`
  is a missing import, while importer exceptions propagate as exceptional
  failures.
- Keep compiler/JIT unchanged; only interpreter gains extended behavior.

## Runtime Semantics
- Symbols:
  - Identifiers resolve local scope, then formula scope.
  - `@param` resolves parameter values; defaults come from `default:`
    parameter blocks.
  - `#name` resolves predefined/environment symbols using the same
    normalized names as `set_value`.
  - Unknown scalar reads keep current compatibility: zero value.
- Parameters:
  - Every formula parameter has an effective value before evaluation. Missing
    host bindings use UF defaults. Scalar, `Image`, and function defaults are
    runtime state today. Function parameter dispatch is implemented for
    `fn1`..`fn4` compatibility calls and `@name(...)` custom calls. Enum
    parameters preserve selected label and numeric index, and hosts can bind
    them by label or valid index. Plug-in defaults are validated by semantic
    analysis and stored as selected class runtime handles with empty object
    state.
  - Missing defaults are not runtime errors. Invalid defaults are parser,
    resolver, or semantic diagnostics before execution.
  - Plug-in selector resolution is eager by design. Defaults are diagnosed
    during construction/analysis, host overrides will be resolved when bound,
    and parameter-set selectors are diagnosed while preparing interpreters.
    `interpret` never performs lazy selector resolution.
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
  - The documented image methods and properties from the UF6 image-parameter
    docs are implemented for the current image handle.
  - Host code binds image parameter data with `set_parameter("name", image)`.
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
  - Imported files are indexed before semantic checks. Only referenced
    imported class ASTs are retained and diagnosed.
  - Import order is preserved for later class lookup: last imported file
    wins; an ancestor file in `class X(File.ufm:Base)` is treated as an
    implicit import before explicit imports.
  - Import edges, resolved class references, retained imported ASTs, and
    diagnostics are exposed as load metadata.

## Implementation Slices
Each slice should leave BASIC behavior unchanged and should run the project
workflow before being considered complete.

1. Backfill interpreter runtime backstop tests for static class members.
    - Add tests for invalid static class method/member runtime failures.
    - Add tests for static method return conversion, by-ref arguments, and
      const arguments.
    - Add tests for class constants with omitted initializers and explicitly
      unsupported constant value shapes.

2. User constructors and casts.
    - Run class constructors during `new Class(...)` and `new @plugin(...)`
      once method dispatch exists.
    - Enforce constructor inheritance rules validated by semantic analysis,
      including matching derived/ancestor constructor arguments and explicit
      ancestor constructor calls.
    - Implement casts between object references according to the retained
      class inheritance graph; failed casts return an empty/null reference.
    - Tests: constructor arguments initialize fields, base-to-derived casts,
      failed casts, and constructor arity/type backstops.

## Tests
- Interpreter tests for typed declarations/defaults/coercions and
  invalid conversions.
- Tests for `@param`, `#pixel/#z/#index/#color/#solid`, and default
  param initialization.
- Tests for clean parameter binding APIs, binding diagnostics, effective
  defaults, function parameters, enum parameters, plug-in selectors, parameter
  forwards, and non-selectable plug-ins.
- Tests for `while`, `repeat/until`, loop guard, top-level and function
  `return`.
- Tests for user functions: declaration after call, recursion, local
  shadowing, by-ref mutation, const rejection.
- Tests for arrays: static multidimensional indexing, dynamic
  `setLength`, `length`, bounds failures, static copy, dynamic copy
  rejection.
- Tests for color construction, extraction, color arithmetic,
  final-section color return.
- Tests for section dispatch: fractal, coloring `final`, transformation
  `transform`, and global read-only behavior.
- Tests for import loading: chained imports, import order, missing file,
  public import metadata, referenced AST retention, and syntax errors in
  referenced imported classes.
- Tests that `ExtendedInterpreter` refuses to run when parse, resolution, or
  semantic diagnostics contain errors.
- Regression: existing basic interpreter tests still pass unchanged.

## Assumptions
- Scope is procedural first, per user choice.
- `import` handling, reference resolution, and retained imported class
  diagnostics are reused from the existing loader and semantic analyzer.
- Objects/classes remain unsupported until their implementation slices land;
  until then, evaluated object/class nodes throw clear runtime errors.
- Uninitialized array reads are deterministic zero/default in this
  interpreter, despite UF describing static array contents as
  indeterminate.
