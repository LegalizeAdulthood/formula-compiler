# Extended Parameter Interpreter Plan

## Summary
- Add an `ExtendedParameterInterpreter` facade for evaluating an extended
  parameter set.
- The facade wires saved `.upr` values into the existing extended formula
  interpreter APIs.
- The client supplies parameter entries, formula entry resolution, and
  runtime environment values. The facade owns parsing, reference resolution,
  semantic analysis, interpreter construction, and parameter binding.
- Image traversal, tiling, threading, compositing policy, and output storage
  remain client responsibilities.

## Existing Pieces
- `parse_extended_parameters` returns an `ExtendedParameterEntry` with one
  `fractal` section and one or more complete `ParameterLayer` values.
- `collect_parameter_references` finds `formula`, `inside`, `outside`, and
  `transform` references with layer and transform context.
- `resolve_parameter_references` asks a caller-provided resolver for each
  referenced `FileEntry`.
- `analyze_parameter_set` validates resolved formula kinds, retained class
  graphs, saved scalar parameters, function parameters, plug-in selectors,
  nested plug-in assignments, and parameter forwards.
- `ExtendedInterpreter` constructs and analyzes one extended formula entry,
  exposes parameter metadata, and accepts parameter, function, plug-in, and
  nested plug-in bindings through explicit APIs.

## API Shape
Place the public API in:

```text
libs/interpreter/include/formula/interpreter/ExtendedParameterInterpreter.h
libs/interpreter/ExtendedParameterInterpreter.cpp
```

The API should use C++17 containers and views only:

```cpp
struct ExtendedParameterInterpreterOptions
{
    parameter::ParameterEntryResolver resolver;
    FormulaFileImporter importer;
    semantic::BuiltinRegistry const *builtins{};
};

struct LayerEvaluationContext
{
    Complex pixel{};
    Complex screen_pixel{};
    int x{};
    int y{};
    int width{};
    int height{};
    int max_iterations{};
};

struct LayerEvaluationResult
{
    bool escaped{};
    int iterations{};
    Complex z{};
    Color inside_color{};
    Color outside_color{};
};

class ExtendedParameterInterpreter
{
public:
    ExtendedParameterInterpreter(parser::FileEntry entry,
        ExtendedParameterInterpreterOptions options);

    const std::vector<Diagnostic> &diagnostics() const;
    bool ok() const;

    std::size_t layer_count() const;
    const parameter::ExtendedParameterEntry &parameters() const;

    void set_value(std::string_view name, Value value);
    void set_fractal_parameter(std::string_view name, Value value);
    void set_layer_parameter(
        std::size_t layer, ParameterReferenceKind site, std::string_view name, Value value);
    void set_layer_function_parameter(
        std::size_t layer, ParameterReferenceKind site, std::string_view name, std::string_view target);
    void set_layer_plugin_parameter(
        std::size_t layer, ParameterReferenceKind site, std::string_view name, std::string_view selector);
    void set_layer_plugin_parameter_value(std::size_t layer,
        ParameterReferenceKind site,
        std::string_view plugin_name,
        std::string_view nested_name,
        Value value);

    LayerEvaluationResult evaluate_layer(std::size_t layer, const LayerEvaluationContext &context);
};
```

This shape is intentionally minimal. It gives clients a facade that can be
used for tests and simple renderers without forcing a pixel scheduler or
image buffer model into the library.

## Construction Pipeline
The constructor performs the cold setup:

1. Parse the supplied `FileEntry` with `parse_extended_parameters`.
2. Collect references from the parsed parameter set.
3. Resolve references through the supplied `ParameterEntryResolver`.
4. Parse each resolved `FileEntry` with the correct formula `EntryKind`.
5. Run formula import resolution for each referenced formula entry.
6. Run `analyze_parameter_set`.
7. If diagnostics contain errors, stop before constructing interpreters.
8. Construct one fractal `ExtendedInterpreter` per layer.
9. Construct zero or more transform `ExtendedInterpreter` values per layer.
10. Construct inside and outside coloring `ExtendedInterpreter` values per
    layer.
11. Apply saved parameter-section assignments to each interpreter.
12. Retain diagnostics from parameter parsing, reference resolution,
    semantic analysis, interpreter construction, and binding.

The facade should not require clients to call parse, resolve, analyze, or
manual binding steps separately.

## Saved Assignment Binding
Bindings come from the section that selected a referenced formula:

- `formula:` binds the layer fractal interpreter.
- `transform:` binds that transform interpreter.
- `inside:` binds the inside coloring interpreter.
- `outside:` binds the outside coloring interpreter.

Saved assignment prefixes are file-format details:

- `p_name=value` binds parameter `name`.
- `f_fn1=sin` binds function parameter `fn1`.
- `f_fn2=ident`, `f_fn3=ident`, and `f_fn4=ident` use the same rule.
- `p_plugin=ClassName` selects a plug-in class parameter.
- `p_plugin.p_child=value` binds nested parameter `child` on selected
  plug-in parameter `plugin`.

The facade strips `p_` and `f_` before calling the existing
`ExtendedInterpreter` APIs. It should not expose `p_` or `f_` as runtime
parameter names.

## Value Conversion
Saved values are strings in the parser AST. The facade must convert them by
using formula parameter metadata and semantic type information:

- bool: `yes`, `no`, `true`, `false`, `on`, `off`
- int: decimal integer text
- float: decimal or exponent text
- complex: UF parameter pair form such as `-0.5/0.25`
- color: unsigned packed color text and any documented color literal forms
- string: unescaped parameter string
- enum: selected label or numeric index
- function: selected function name
- plug-in: selected class name plus nested values
- `Image`: client-provided image values only

Invalid conversions should be diagnostics during construction or binding,
not runtime surprises during pixel evaluation.

## Layer Runtime Flow
`evaluate_layer` should run the interpreters in UF order:

1. Bind client environment values such as pixel, screen pixel, dimensions,
   maximum iterations, and coordinates into every interpreter needed by the
   layer.
2. Execute transform `global` sections once per layer setup when the first
   pixel is evaluated, then execute `transform` for each pixel in declared
   order.
3. Execute fractal `global` once per layer setup when the first pixel is
   evaluated.
4. Execute fractal `init` for the transformed pixel.
5. Execute fractal `loop` and `bailout` until bailout or maximum iterations.
6. Execute coloring `global` once per coloring interpreter when first used.
7. Execute coloring `init`, `loop`, and `final` for the selected inside or
   outside coloring formula.
8. Return the final layer data to the client. The client composites layers.

The first implementation can keep one facade instance single-threaded. A
threaded renderer can create one facade or one cloned runtime per worker.

## Diagnostics
All errors should be reported through `diagnostics()`:

- parameter parse errors
- missing formula entries
- invalid formula kind for a section
- formula parse/import errors
- semantic errors
- invalid saved parameter name
- invalid saved function selector
- invalid plug-in selector
- invalid nested plug-in path
- invalid value conversion
- section/layer index errors in public binding APIs
- attempts to evaluate when setup failed

Diagnostics should include parameter set source location when the error comes
from saved assignments.

## Ownership
- The facade owns the parsed parameter set and constructed interpreters.
- The facade does not own the client file index or filesystem.
- The resolver returns `FileEntry` copies.
- Imported formula text is obtained through the existing formula importer.
- Runtime `Image` values are client-owned values copied or referenced through
  existing `Value` semantics.

## Implementation Slices
1. Add `ExtendedParameterInterpreter` header and source with constructor,
   diagnostics, `ok`, `layer_count`, and parsed-parameter accessors. Wire it
   into the interpreter library and add construction tests for parse errors.
2. Implement parameter reference resolution inside the facade. Add tests for
   missing entries, wrong formula kind, and successful one-layer reference
   collection.
3. Construct layer interpreters for `formula`, `inside`, `outside`, and
   `transform` sections. Add tests that verify each interpreter is created
   with the correct `EntryKind`.
4. Apply saved scalar `p_` assignments through `set_parameter`. Add tests for
   bool, int, float, complex, color, string, and enum conversion.
5. Apply saved `f_` assignments through `set_function_parameter`. Add tests
   for `fn1` through `fn4`, invalid selectors, and default preservation.
6. Apply saved plug-in selectors through `set_plugin_parameter`. Add tests for
   default plug-in selection, saved override selection, and invalid selectors.
7. Apply dotted nested plug-in assignments through
   `set_plugin_parameter_value`. Add tests for nested scalar values,
   parameter forwards, and invalid nested paths.
8. Add client override APIs on the facade and route overrides to the relevant
   underlying interpreter. Add tests that client overrides win over saved
   parameter values.
9. Add layer environment binding from `LayerEvaluationContext`. Add tests that
   predefined symbols reach fractal, transform, and coloring interpreters.
10. Implement single-layer evaluation order without transforms. Add tests for
    fractal iteration and inside/outside coloring dispatch.
11. Implement transform evaluation before fractal init. Add tests for one and
    multiple transforms.
12. Add one end-to-end parameter-set fixture test that resolves inline test
    formulas, binds saved values, and evaluates a layer deterministically.
13. Document thread-safety limits and add tests that separate facade instances
    keep independent runtime state.

## Non-Goals
- Do not implement rendering, tiling, or layer compositing here.
- Do not add extended compiler or GLSL support here.
- Do not expose raw `p_` or `f_` names as interpreter parameter names.
- Do not lazy-resolve plug-ins at runtime. Defaults and saved selectors are
  known during setup and are semantic/setup diagnostics if invalid.
