# Extended Formula Interpreter: Procedural Runtime

## Summary
- Target procedural UF6 runtime first: typed values, declarations,
  arrays, params/constants, loops, returns, user functions, color values,
  and `final`/`transform` sections.
- Defer user object/class execution: user `new`, member access, methods,
  inheritance, casts, and plug-in object instantiation remain
  unsupported, but imports are resolved before interpretation. The builtin
  `Image` object has runtime support.
- Treat the existing `Formula` interface as the BASIC formula execution
  surface. Extended execution needs a separate value-aware interpreter
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
  `Image`. The extended compiler does not exist yet. The current
  interpreter/compiler remain BASIC-oriented and reject unsupported
  extended AST nodes.

## Interpreter Work
- Add `formula::Value`: `monostate`, `bool`, `int`, `double`,
  `Complex`, `Color`, `std::string`, `ArrayValue`, `ImageValue`.
- Add a C++17 `ExtendedInterpreter` facade for interpreting one
  extended formula entry:

  ```cpp
  class ExtendedInterpreter
  {
  public:
      ExtendedInterpreter(FileEntry entry, ExtendedInterpreterOptions options);

      const std::vector<Diagnostic> &diagnostics() const;
      bool ok() const;
      const std::vector<FormulaParameterInfo> &parameters() const;

      void set_value(std::string_view name, Value value);
      Value value(std::string_view name) const;

      Value interpret(Section section);
  };
  ```

  The constructor should run the cold pipeline for one entry: parse,
  resolve referenced files/classes, analyze semantics, and collect parameter
  metadata. After construction, the client queries `parameters()` to discover
  settable scalar, image, function, and plug-in parameters, then binds runtime
  values with `set_value`. `interpret` runs one section and fails if
  diagnostics contain errors. The facade owns formula evaluation state, but
  not image rendering, pixel scheduling, tiling, threading, or layer
  orchestration.
- Add public value-aware APIs while preserving current `Complex`
  wrappers:
  - `Formula::set_value(std::string_view, Value)`
  - `Formula::get_runtime_value(std::string_view) const`
  - `Formula::interpret_value(Section)`
  - Existing `set_value(..., Complex)`, `get_value`, `interpret` stay
    compatible.
- Extend `Section` with `FINAL` and `TRANSFORM`; wire `get_section` and
  interpreter dispatch for coloring/transformation entries.
- Reuse the existing import-loading and reference-resolution layer before
  interpreter construction. Imported files are already parsed and indexed
  enough to collect class declarations and import metadata. Referenced
  imported class ASTs are retained and diagnosed by existing load and
  semantic passes.
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

1. Clean parameter binding API.
    - Add interpreter facade methods for binding formula parameters by source
      parameter name, without `@`, `p_`, or `f_` prefixes.
    - Keep `@name` as formula-language syntax only. Keep `p_` and `f_` as
      extended parameter-set file-format prefixes only.
    - Suggested surface:

      ```cpp
      void set_parameter(std::string_view name, Value value);
      void set_function_parameter(std::string_view name, std::string_view target);
      void set_plugin_parameter(std::string_view name, std::string_view selector);
      void set_plugin_parameter_value(
          std::string_view plugin_name, std::string_view nested_name, Value value);
      ```

    - Tests: scalar, image, function, and plug-in parameter APIs bind by raw
      parameter name; prefixed names are not required by the facade.

2. Plug-in runtime value shell.
    - Add a runtime `PluginValue` or equivalent handle that stores the
      selected class reference, retained class AST pointer, nested saved
      parameter bindings, and optional initialized object state.
    - Add default empty/null plug-in values for unbound plug-in parameters.
    - Keep object state empty until `new @pluginParam` is implemented.
    - Tests: default plug-in parameters are empty, host-provided descriptors
      convert to `PluginValue`, and runtime messages describe missing object
      state clearly.

3. Resolve standalone plug-in selectors.
    - `set_plugin_parameter(name, selector)` accepts the same selector text
      used by parameter sets, such as `File.ulb:Class`.
    - The interpreter resolves selector strings through its configured
      importer/resolver options. The client does not pass `FormulaClassReference`,
      retained AST pointers, or semantic analyzer internals.
    - Use `ExtendedInterpreter::parameters()` to let hosts discover which
      formula parameters are plug-in parameters before supplying those
      bindings.
    - Retain referenced class ASTs and run the same semantic checks against
      the selected class and nested saved values.
    - Tests: standalone formula with `Plugin param p` and `new @p` can resolve
      a host selector; repeated binding replaces the old selector; missing
      entry, parse error, kind mismatch, and nested parameter mismatch become
      diagnostics.

4. Translate parameter-set bindings.
    - Replace the current string preservation for `p_plugin=File.ulb:Class`
      and `p_plugin.p_x=value` with resolved `PluginValue` bindings.
    - Treat `p_` and `f_` only as parser/bridge input syntax. Translate them
      into the clean interpreter APIs before runtime state is populated.
    - Reuse the parameter-set reference resolver data so the bridge does not
      reparse entries that are already resolved and retained.
    - Preserve layer/transform site metadata and support independent plugin
      object instances per prepared formula.
    - Tests: parameter-set bridge binds plug-in selectors and nested saved
      values as resolved runtime objects; separate layers get separate
      object state.

5. Nested plug-in binding defaults.
    - When a selected class has plug-in parameters of its own, create nested
      plug-in binding slots from its `default:` metadata.
    - Apply nested saved values from `set_plugin_parameter_value` and
      translated parameter-set `p_plugin.p_x=value` assignments to those slots.
    - Leave unresolved nested selectors as diagnostics before interpretation.
    - Tests: nested plug-in selector binds to the child parameter, nested
      scalar/image/function saved values are applied to the selected class,
      and missing nested selectors produce diagnostics.

6. Construct plug-in instances.
    - Implement `new @pluginParam` for resolved plug-in parameters.
    - Allocate object state from the retained class AST: public/protected/
      private fields, default values, nested plug-in/image parameters, and
      function parameter bindings.
    - Apply nested saved parameter bindings before constructors run.
    - Keep user constructors unsupported until the method-dispatch slice if
      needed, but return a clear diagnostic/runtime error rather than a raw
      unsupported node.
    - Tests: `new @pluginParam` returns an object with initialized fields,
      missing plug-in binding fails clearly, and nested defaults are applied.

7. User class field access and assignment.
    - Implement lvalues for object fields, including visibility rules already
      validated by semantic analysis.
    - Allow member reads/writes on plug-in and user class instances.
    - Preserve object identity for by-reference use and assignments.
    - Tests: field read/write, copied object references, private member
      access remains a semantic error, and assignment through fields works.

8. User class method dispatch.
    - Implement method calls on plug-in and user class instances, including
      `this`, local scope, return conversion, by-ref/const args, and access
      to object fields.
    - Support method lookup through base classes in the same order the
      semantic analyzer validates.
    - Tests: public method call, inherited method call, method mutating
      object state, by-ref args, const args, and return conversion.

9. User constructors and casts.
    - Run class constructors during `new Class(...)` and `new @plugin(...)`
      once method dispatch exists.
    - Implement casts between object references according to the retained
      class inheritance graph; failed casts return an empty/null reference.
    - Tests: constructor arguments initialize fields, base-to-derived casts,
      failed casts, and constructor arity/type backstops.

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
- Objects/classes remain unsupported in interpreter and throw clear
  runtime errors if evaluated.
- Uninitialized array reads are deterministic zero/default in this
  interpreter, despite UF describing static array contents as
  indeterminate.
