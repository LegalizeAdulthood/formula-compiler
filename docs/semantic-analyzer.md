# Semantic Analyzer Plan

## Summary

Semantic analysis runs after parsing and, for parameter sets, after reference
resolution. The parser and resolver should keep returning source-preserving
syntax data. The semantic analyzer should report errors in names, types,
bindings, class references, builtin usage, and render input consistency.

The analyzer should not change parsed data structures. Parsed formulas and
parameter sets remain immutable syntax artifacts. If later compiler or
interpreter stages need resolved bindings or inferred types, add a separate
semantic model built alongside the parsed data.

## Pipeline Boundary

Keep the pipeline split into four explicit stages:

```text
parse -> resolve references -> analyze semantics -> interpret or compile
```

The parser owns syntax and structural constraints. It reports malformed
formula text, malformed parameter-set text, invalid section order, invalid
section cardinality, and malformed expressions or statements.

The resolver owns file and entry lookup. It finds formulas, coloring entries,
transforms, classes, and retained imported classes that are referenced by a
formula or parameter set. It reports missing files and missing entries where a
reference cannot be resolved.

The semantic analyzer owns meaning. It validates whether parsed and resolved
data can be used together: names, types, calls, member access, class use,
parameter bindings, formula kind compatibility, builtin use, and complete
reference graphs.

The interpreter and compiler own execution. They should consume only data that
has passed semantic analysis. They may still reject unsupported runtime or
codegen features, but they should not duplicate semantic validation.

## Proposed Entry Points

Use two global functions:

```cpp
std::vector<SemanticDiagnostic> analyze_formula(
    const FormulaSections &formula,
    const FormulaSemanticContext &context);

std::vector<SemanticDiagnostic> analyze_parameter_set(
    const ExtendedParameterEntry &parameters,
    const ParameterReferenceSet &references,
    const ParameterSetSemanticContext &context);
```

Both functions return diagnostics only. A future overload may return a
`SemanticModel` when downstream stages need resolved symbols, member bindings,
or expression types.

## Analyzer Inputs

The formula analyzer input is one parsed formula plus a context containing
entry kind, retained imports, builtins, and host rules.

The parameter-set analyzer input is one parsed extended parameter set plus its
resolved reference set. The parameter set itself is not enough, because saved
values are meaningful only when checked against the defaults and types of the
referenced formulas and classes.

Parameter-set semantic analysis is needed for more than reference existence.
It also checks whether saved values bind to real parameters, whether those
values have compatible types, whether formula kinds are used in valid layer
positions, and whether nested plug-in assignments match the selected class.

## Diagnostics

Add a semantic diagnostic type separate from parser diagnostics. Parser
diagnostics describe invalid syntax. Semantic diagnostics describe syntactically
valid input that cannot be used correctly.

Suggested shape:

```cpp
enum class SemanticDiagnosticSeverity
{
    ERROR,
    WARNING,
};

enum class SemanticDiagnosticCode
{
    UNKNOWN_SYMBOL,
    DUPLICATE_SYMBOL,
    UNKNOWN_TYPE,
    INVALID_TYPE_CONVERSION,
    INVALID_ASSIGNMENT_TARGET,
    INVALID_CALL_TARGET,
    INVALID_CALL_ARITY,
    INVALID_ARGUMENT_TYPE,
    INVALID_MEMBER_ACCESS,
    INVALID_ARRAY_ACCESS,
    INVALID_RETURN,
    INVALID_SECTION_RESULT,
    INVALID_PARAMETER_BINDING,
    INVALID_FORMULA_KIND,
    INVALID_BUILTIN_USAGE,
    INCOMPLETE_REFERENCE_GRAPH,
    UNSUPPORTED_SEMANTIC_FEATURE,
};

struct SemanticDiagnostic
{
    SemanticDiagnosticSeverity severity;
    SemanticDiagnosticCode code;
    SourceLocation location;
    std::string message;
    std::string entry_name;
    std::string section_name;
};
```

Start with errors only. Add warnings after the analyzer is integrated and the
call sites have a policy for displaying or suppressing them.

Initial diagnostic categories:

- Unknown symbol.
- Duplicate symbol.
- Unknown type.
- Invalid type conversion.
- Invalid assignment target.
- Invalid call target.
- Invalid call arity.
- Invalid argument type.
- Invalid member access.
- Invalid array access.
- Invalid return.
- Invalid section result.
- Invalid parameter binding.
- Invalid formula kind.
- Invalid builtin usage.
- Incomplete reference graph.
- Unsupported semantic feature.

Diagnostics should carry source location when available. For parameter-set
errors, prefer the parameter assignment location. For formula errors, use the
formula source location.

Messages should name the invalid symbol, type, member, parameter, or formula
kind. The diagnostic code should stay stable enough for tests to assert against
it without depending on exact prose.

## Formula Context

`FormulaSemanticContext` should provide the analyzer with data that is not
stored directly in one parsed formula:

- Entry kind.
- Source identity.
- Resolved imported classes retained for this formula.
- Builtin scalar, color, string, array, and object type descriptors.
- Builtin function descriptors.
- Builtin class descriptors, including `Image`.
- Optional host feature flags.

The context should not cause imports to be loaded. Import resolution remains a
separate parser/resolver responsibility.

Suggested shape:

```cpp
struct FormulaSemanticContext
{
    EntryKind entry_kind;
    std::string source_name;
    std::span<const FormulaEntry> retained_classes;
    const BuiltinRegistry *builtins;
};
```

Keep ownership outside the analyzer. The context should reference data produced
by the loader, parser, resolver, or host.

## Formula Responsibilities

The formula analyzer should validate:

- Local variable, parameter, constant, field, function, and method names.
- Duplicate declarations in each scope.
- References to variables, constants, functions, parameters, classes, and
  members.
- Assignment targets and assignment value compatibility.
- Function call arity and argument compatibility.
- User function return paths and return value compatibility.
- `const` argument mutation.
- By-reference argument targets.
- Array indexing and array element assignments.
- Boolean condition expressions in `if`, `elseif`, `while`, and `repeat`.
- `switch` expression and case value compatibility.
- Section-specific result types and allowed statements.
- Class inheritance, constructors, visibility, overrides, and member access.
- `new` expressions, casts, and object method calls.
- Builtin object types, especially `Image` fields and methods.

This pass should not execute defaults, functions, or formula sections.

## Formula Analysis Order

Analyze one formula in deterministic phases:

1. Build the builtin environment.
2. Add retained imported class descriptors.
3. Add current formula or class declarations.
4. Collect default metadata from the `default` section.
5. Collect user function signatures before checking bodies.
6. Collect class members before checking methods.
7. Check default metadata values and parameter settings.
8. Check function and method bodies.
9. Check section bodies.
10. Check section-specific result rules.

Each phase should keep going after errors where possible. Later phases should
see error symbols and error types for invalid earlier declarations so one bad
declaration does not hide unrelated diagnostics.

For BASIC formulas, most phases are trivial. BASIC still benefits from the same
pipeline because constants, builtin functions, assignment targets, and section
result checks can share implementation with extended formulas.

## Formula Result Contract

The first formula analyzer should return diagnostics only. Later interpreter or
compiler work may require an analyzed result:

```cpp
struct FormulaSemanticModel
{
    std::vector<SemanticDiagnostic> diagnostics;
    // entry symbols
    // class descriptors
    // default parameter descriptors
    // expression types
    // resolved calls and members
};
```

Do not add this model until a downstream caller needs one. When it is added,
keep it separate from the parsed AST.

## Parameter Set Context

`ParameterSetSemanticContext` should provide data needed to validate a resolved
parameter set:

- Builtin type and class descriptors.
- Formula entries referenced by the parameter set.
- Retained imported class entries for every referenced formula.
- Default parameter metadata extracted from referenced formulas.
- Host render policy, if needed for warnings.

The resolved parameter set should already contain the references needed to
render: fractal settings, layers, referenced formulas, and retained imported
formula/class ASTs. Interpreter and compiler stages remain responsible for
execution behavior.

Suggested shape:

```cpp
struct ParameterSetSemanticContext
{
    const BuiltinRegistry *builtins;
    std::span<const FormulaEntry> referenced_formulas;
    std::span<const FormulaEntry> retained_classes;
};
```

Do not duplicate resolved references in the context when they already exist in
the `ParameterReferenceSet`. The context should supply only shared lookup data
that is not stored in the resolved reference set.

## Builtin Registry

Use one builtin registry for formula and parameter-set analysis. It should
describe language-provided types, functions, constants, classes, methods, and
fields.

Initial contents:

- Scalar types: `void`, `bool`, `int`, `float`, `complex`, `color`, `string`.
- Array type construction.
- Predefined constants by entry kind and section.
- Builtin functions and operators.
- Builtin object classes, including `Image`.

The registry should be read-only during analysis. Tests can build small
registries when a full host registry is not needed.

`Image` must be a builtin class descriptor, not an imported class. It should be
valid as a parameter type even when no file defines it.

## Type Model

Represent semantic types independently from parser spelling. Source identifiers
must still be preserved in the AST exactly as written.

Initial type categories:

- Error.
- Void.
- Bool.
- Int.
- Float.
- Complex.
- Color.
- String.
- Array.
- Class object.
- Builtin object.
- Function.

Use an error type after emitting a diagnostic. This lets analysis continue
without creating a cascade of secondary errors from one bad expression.

Suggested shape:

```cpp
enum class SemanticTypeKind
{
    ERROR,
    VOID,
    BOOL,
    INT,
    FLOAT,
    COMPLEX,
    COLOR,
    STRING,
    ARRAY,
    CLASS_OBJECT,
    BUILTIN_OBJECT,
    FUNCTION,
};

struct SemanticType
{
    SemanticTypeKind kind;
    std::string name;
    std::shared_ptr<const SemanticType> element_type;
};
```

The final implementation may use indexes or handles instead of owned strings
and shared pointers. The important boundary is that semantic types are not
stored back into the parsed AST.

## Conversion Rules

Define conversion rules once and share them between formula analysis and
parameter-set binding analysis.

Initial allowed numeric promotion chain:

```text
bool -> int -> float -> complex
```

Rules:

- Assignment may convert from value type to declared target type when allowed.
- Function arguments may convert to declared parameter types when allowed.
- Return values may convert to declared return type when allowed.
- Parameter-set saved values may convert to default parameter type when allowed.
- `color` does not implicitly convert to numeric values.
- `string` does not implicitly convert except where a documented setting
  requires string parsing.
- Object references may convert through inheritance where valid.
- Builtin object references convert only by explicit descriptor rule.
- Invalid conversions emit diagnostics and return error type.

## Symbols And Scopes

Build symbol tables before type-checking expression bodies. This supports
forward calls, recursion, class references, and parameter metadata lookup.

Initial symbol categories:

- Local variable.
- Function parameter.
- Formula parameter.
- Formula constant.
- User function.
- Builtin function.
- Class.
- Field.
- Method.
- Constructor.
- Builtin constant.

Suggested shape:

```cpp
enum class SemanticSymbolKind
{
    LOCAL,
    FUNCTION_PARAMETER,
    FORMULA_PARAMETER,
    FORMULA_CONSTANT,
    USER_FUNCTION,
    BUILTIN_FUNCTION,
    CLASS,
    FIELD,
    METHOD,
    CONSTRUCTOR,
    BUILTIN_CONSTANT,
};

struct SemanticSymbol
{
    SemanticSymbolKind kind;
    std::string name;
    SemanticType type;
    SourceLocation location;
};
```

Scope rules:

- Entry scope contains formula parameters, formula constants, user functions,
  classes, and builtins visible to the entry.
- Function scope contains function parameters and local declarations.
- Block scope contains local declarations from nested statements.
- Class scope contains fields, methods, constructors, inherited members, and
  visible builtin members if the class is builtin.
- Parameter-set binding scope is built from default parameter metadata for each
  referenced formula or selected plug-in class.

Duplicate checks should run during symbol collection where possible. Unknown
name checks should run during expression and binding analysis.

Identifier lookup should preserve source spelling in diagnostics. Any
case-insensitive lookup rule must keep the original identifier text in the AST
and in diagnostics.

## Parameter Set Responsibilities

The parameter-set analyzer should validate:

- The fractal formula reference exists and has fractal kind.
- Each layer coloring reference exists and has coloring kind.
- Each layer transform reference exists and has transformation kind.
- Referenced formulas have complete retained import graphs.
- Saved parameter names exist in referenced formula defaults.
- Saved parameter value types match referenced parameter metadata.
- Enum parameter values match allowed labels or indexes.
- Function parameter values target callable entries with compatible shape.
- Plug-in parameter values target compatible class entries.
- Nested plug-in assignments target parameters on the selected class.
- Builtin classes used by parameters, including `Image`, are available.
- Gradient and opacity values are structurally valid after parsing.

Section cardinality and ordering are parser responsibilities. Semantic analysis
should assume parsed parameter sets already satisfy those syntax constraints.

## Parameter Set Analysis Order

Analyze one resolved extended parameter set in deterministic phases:

1. Check that resolver diagnostics are empty or convert them into semantic
   diagnostics for the caller.
2. Check that the fractal reference exists and points to a fractal entry.
3. Check that each layer coloring reference exists and points to a coloring
   entry.
4. Check that each layer transform reference exists and points to a
   transformation entry.
5. Check that every referenced formula has a complete retained import graph.
6. Build parameter metadata scopes from the referenced formula defaults.
7. Validate saved fractal parameter values against fractal metadata.
8. Validate saved layer coloring values against coloring metadata.
9. Validate saved layer transform values against transform metadata.
10. Resolve selected plug-in classes for plug-in parameter values.
11. Validate nested plug-in assignments against the selected class defaults.
12. Validate function parameter values against callable metadata.
13. Validate builtin object parameter values, including `Image`.

The analyzer should continue after one bad layer or parameter where possible.
Bad bindings should create error entries in the binding scope so later nested
assignments can report precise errors without crashing or silently disappearing.

## Parameter Set Result Contract

The first parameter-set analyzer should return diagnostics only. Later stages
may need a binding model:

```cpp
struct ParameterSetSemanticModel
{
    std::vector<SemanticDiagnostic> diagnostics;
    // resolved formula kinds
    // resolved parameter descriptors
    // converted saved values
    // resolved plug-in class bindings
    // resolved function parameter targets
};
```

Do not write converted values or resolved bindings back into the parsed
parameter set. Keep the parsed parameter set as the source artifact and keep
derived semantic data separate.

## Semantic Model

Do not mutate AST or parameter-set objects. If downstream stages need resolved
data, introduce a separate model:

```cpp
struct SemanticModel
{
    std::vector<SemanticDiagnostic> diagnostics;
    // symbol tables
    // expression types
    // resolved calls
    // resolved members
    // resolved parameter bindings
};
```

Keep the diagnostic-only entry points first. Add model-returning APIs only when
the interpreter or compiler needs them.

## Implementation Slices

1. Add semantic diagnostic types, context structs, and no-op analyzer entry
   points.
2. Collect formula symbols and report duplicate names.
3. Resolve formula names and report unknown variables, functions, constants,
   classes, and parameters.
4. Add expression type checking for scalar, bool, string, color, complex, and
   array values.
5. Add function, return, by-reference, and `const` argument checks.
6. Add class, member, constructor, `new`, cast, visibility, and builtin `Image`
   checks.
7. Add section-specific formula validation.
8. Add parameter-set binding validation over resolved references.
9. Add nested plug-in/class parameter validation.
10. Integrate interpreter and compiler entry points so unsupported or invalid
    semantic inputs are rejected before execution/code generation.

## Tests

Add tests for:

- Duplicate symbols.
- Unknown variables, functions, constants, types, classes, and members.
- Bad assignment targets.
- Bad call arity and argument types.
- Bad return statements.
- Bad array indexing.
- Bad `new`, cast, and member access expressions.
- Builtin `Image` fields and methods.
- Formula-kind mismatch in parameter sets.
- Unknown saved parameter names.
- Saved parameter type mismatches.
- Invalid enum, function, and plug-in parameter values.
- Invalid nested plug-in parameter assignments.
- Incomplete retained import graphs.

## Documentation

This document defines the semantic-analysis boundary for both formulas and
resolved extended parameter sets. Keep `extended-semantics.md` as background
until this plan is implemented, then merge or retire overlapping material.
