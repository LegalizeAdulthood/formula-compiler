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
    runtime state today. Plug-in defaults are validated by semantic analysis
    and implemented by later plug-in slices.
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

1. Function parameter binding and dispatch.
    - Implement `set_function_parameter` for raw parameter names.
    - Validate function targets at binding time and record binding diagnostics.
    - Compare binding-time target validation with parameter-set analysis so
      host overrides and saved `f_` bindings reject the same invalid shapes.
    - Keep target-shape, callable-kind, and global-section restriction checks
      aligned between construction diagnostics and binding diagnostics.
    - Dispatch calls through function parameters to builtin or user functions
      as allowed by semantic rules.
    - Preserve global-section `const` function restrictions.
    - Tests: default fallback functions, host function override, invalid
      target diagnostics, builtin target call, user target call, global
      restriction enforcement, and analyzer-only failure coverage for any
      construction-time cases discovered while implementing the interpreter.

2. Enum parameter runtime values.
    - Represent enum parameters as values that preserve selected label and
      numeric index.
    - Accept host binding by label or valid index.
    - Support documented comparisons of enum parameters to strings.
    - Tests: default enum selection, label binding, index binding, invalid
      label/index diagnostics, and string comparison.

3. Plug-in runtime value shell.
    - Add a runtime `PluginValue` or equivalent handle that stores the
      selected class reference, retained class AST pointer, nested saved
      parameter bindings, and optional initialized object state.
    - Do not model an omitted plug-in value as unresolved. Every plug-in
      parameter has an effective selected class from its explicit default or
      base class.
    - Keep object state empty until `new @pluginParam` is implemented.
    - Tests: effective plug-in defaults create `PluginValue`, host overrides
      replace selected class, and runtime messages describe missing object
      state clearly.

4. Resolve standalone plug-in selectors.
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

5. Translate parameter-set bindings.
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

6. Parameter forwards.
    - Apply parameter forwards when translating old saved parameter names from
      parameter sets into current parameter bindings.
    - Do not expose forward syntax through the standalone interpreter API.
      Standalone clients bind current parameter names directly.
    - Tests: old saved scalar value forwards into nested plug-in parameter,
      conflicting forwards remain diagnostics, and direct current-name binding
      bypasses forwards.

7. Non-selectable plug-in binding.
    - Treat `selectable=false` as a UI/binding flattening rule: standalone
      clients bind the visible child parameter names directly, while
      parameter-set translation maps saved child values into the non-selectable
      plug-in's nested parameters.
    - Keep the runtime model as a plug-in object with nested parameters.
    - Tests: non-selectable plug-in defaults instantiate the base/default
      class, direct child bindings update nested parameters, and parameter-set
      saved values translate correctly.

8. Nested plug-in binding defaults.
    - When a selected class has plug-in parameters of its own, create nested
      plug-in binding slots from its `default:` metadata.
    - Apply nested saved values from `set_plugin_parameter_value` and
      translated parameter-set `p_plugin.p_x=value` assignments to those slots.
    - Leave unresolved nested selectors as diagnostics before interpretation.
    - Tests: nested plug-in selector binds to the child parameter, nested
      scalar/image/function saved values are applied to the selected class,
      and missing nested selectors produce diagnostics.

9. Construct plug-in instances.
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

10. User class field access and assignment.
    - Implement lvalues for object fields, including visibility rules already
      validated by semantic analysis.
    - Allow member reads/writes on plug-in and user class instances.
    - Preserve object identity for by-reference use and assignments.
    - Define null-reference behavior for field and method access using clear
      runtime errors.
    - Tests: field read/write, copied object references, private member
      access remains a semantic error, null access fails clearly, and
      assignment through fields works.

11. User class method dispatch.
    - Implement method calls on plug-in and user class instances, including
      `this`, local scope, return conversion, by-ref/const args, and access
      to object fields.
    - Support method lookup through base classes in the same order the
      semantic analyzer validates.
    - Tests: public method call, inherited method call, method mutating
      object state, by-ref args, const args, and return conversion.

12. Static class methods and constants.
    - Implement `Class.method(...)` dispatch for static methods.
    - Implement class constant lookup, including inherited constants.
    - Tests: direct static method call, inherited static method lookup,
      direct class constant, inherited class constant, and invalid static
      member diagnostics/runtime backstops.

13. User constructors and casts.
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
