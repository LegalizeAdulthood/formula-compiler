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

## Scope

Semantic analysis is only for extended formulas and extended parameter sets.

BASIC formulas do not need semantic analysis. They cannot reference user
functions, other formulas, classes, imports, extended parameters, or object
metadata. All static BASIC validation should be completed by the parser. Unknown
BASIC identifiers remain valid variables with runtime zero initialization.

BASIC parameter sets do not need semantic analysis. They are flat assignment
lists with no formula reference graph, layer structure, imported classes, or
typed parameter metadata to validate. Their syntax and value parsing are parser
responsibilities.

The remaining semantic analyzer work therefore starts at the extended language
boundary: typed formulas, user functions, classes, imports, builtin objects,
resolved extended parameter sets, and cross-entry bindings.

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

The semantic analyzer owns extended-language meaning. It validates whether
parsed and resolved extended data can be used together: names, types, calls,
member access, class use, parameter bindings, formula kind compatibility,
builtin use, and complete reference graphs.

The interpreter and compiler own execution. They should consume only data that
has passed semantic analysis. They may still reject unsupported runtime or
codegen features, but they should not duplicate semantic validation.

## Error Ownership

Avoid reporting the same problem from multiple stages. Each stage should own a
small class of diagnostics:

- Parser: malformed text, invalid token sequences, invalid section order,
  invalid section cardinality, and impossible AST shapes.
- Resolver: missing files, missing entries, unresolved imports, and reference
  graphs that cannot be loaded.
- Semantic analyzer: invalid meaning after parse and resolution, including
  names, types, calls, members, formula kinds, parameter bindings, and builtin
  use.
- Interpreter: runtime failures that require execution state.
- Compiler: unsupported lowering or code generation failures after semantic
  analysis succeeds.

When a later stage receives earlier diagnostics, it should stop or forward
them instead of trying to reinterpret the same input. For example, a missing
formula entry belongs to the resolver, while a coloring formula used as a
fractal formula belongs to semantic analysis.

If a diagnostic could belong to two stages, choose the earliest stage that has
all information needed to report it accurately.

## Proposed Entry Points

Use two global functions for extended inputs:

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

The formula analyzer input is one parsed extended formula plus a context
containing entry kind, retained imports, builtins, and host rules.

The parameter-set analyzer input is one parsed extended parameter set plus its
resolved reference set. The parameter set itself is not enough, because saved
values are meaningful only when checked against the defaults and types of the
referenced formulas and classes. BASIC parameter sets are excluded.

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
- Predefined-symbol rules described in the Predefined Symbols section.
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

BASIC formulas are not analyzed by this pipeline. Parser success is their
complete static validation boundary.

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
- Predefined symbol descriptors.
- Builtin functions and operators.
- Builtin object classes, including `Image`.

The registry should be read-only during analysis. Tests can build small
registries when a full host registry is not needed.

`Image` must be a builtin class descriptor, not an imported class. It should be
valid as a parameter type even when no file defines it.

## Predefined Symbols

The parser validates that `#` references name only documented predefined
symbols. Semantic analysis owns the remaining meaning for those accepted
symbols.

The semantic predefined-symbol descriptor table should use the same documented
set accepted by the parser:

- `#angle`
- `#calculationPurpose`
- `#center`
- `#color`
- `#dpixel`
- `#dz`
- `#e`
- `#height`
- `#index`
- `#magn`
- `#maxiter`
- `#numiter`
- `#pi`
- `#pixel`
- `#random`
- `#randomrange`
- `#screenmax`
- `#screenpixel`
- `#skew`
- `#stretch`
- `#whitesq`
- `#width`
- `#x`
- `#y`
- `#z`

Each descriptor should store canonical name, type, mutability,
constant-expression availability, allowed formula kinds, and allowed sections.
The analyzer should preserve source spelling in diagnostics.

Semantic checks should:

- Assume the parser has already rejected writes to globally read-only
  predefined symbols.
- Allow writes only to documented writable symbols in allowed formula kinds and
  sections.
- Validate use in constant-expression contexts, including array declarations
  and default settings.
- Validate formula-kind and section availability.

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

Before starting code implementation, present the completed plan for review and
wait for approval. Do not begin semantic analyzer source changes from this plan
until the review is complete.

Milestone 1: analyzer shell.

- Add public semantic analyzer header.
- Add diagnostic, context, type, and symbol declarations.
- Add no-op `analyze_formula` and `analyze_parameter_set`.
- Add tests proving syntactically valid empty inputs return no diagnostics.
- Add tests proving the API does not mutate parsed data.

Milestone 2: builtin registry.

- Add scalar type descriptors.
- Add builtin constants by entry kind and section.
- Add predefined-symbol descriptors by entry kind and section.
- Add builtin function descriptors needed by current parser tests.
- Add builtin class descriptor support.
- Add `Image` as a builtin class without import lookup.

Milestone 3: formula symbols.

- Collect entry, function, block, and class scopes.
- Report duplicate declarations.
- Preserve identifier spelling in diagnostics.
- Add error symbols for invalid declarations so analysis can continue.

Milestone 4: formula expressions.

- Resolve identifiers.
- Resolve constants and parameter references.
- Resolve calls, member access, array indexing, and assignments.
- Track static and dynamic array declarations separately from scalar values.
- Validate `setLength` and `length` calls for dynamic arrays.
- Apply conversion rules.
- Emit expression type diagnostics.

Milestone 5: extended formula statements and sections.

- Check declarations, returns, by-reference arguments, `const` arguments,
  loops, branches, and switch expressions.
- Check section-specific constants and result rules.
- Leave BASIC behavior covered by parser validation only.

Milestone 6: classes and builtin objects.

- Resolve imported and current classes.
- Check inheritance, constructors, methods, fields, visibility, `new`, and
  casts.
- Check `Image` members and methods through the builtin descriptor.

Milestone 7: parameter-set bindings.

- Validate formula kinds for fractal and layer references.
- Validate complete retained import graphs.
- Build parameter metadata scopes from referenced defaults.
- Check saved values, enums, functions, plug-ins, nested plug-in assignments,
  and builtin object parameters.

Milestone 8: downstream integration.

- Make interpreter entry points reject semantic errors before execution.
- Make compiler entry points reject semantic errors before code generation.
- Keep runtime and codegen diagnostics limited to unsupported or dynamic
  failures not knowable by semantic analysis.

1. Add remaining class checks.
   - Unknown base class.
   - Inheritance cycle.
   - Visibility.
   - Cast validity.

2. Add remaining section-specific formula validation.
   - Switch case value compatibility.
   - Formula-kind-specific section availability not already enforced by the
     parser.

3. Finish extended parameter-set binding validation.
   - Missing referenced entries converted from resolver diagnostics if needed.
   - Incomplete retained import graph coverage for all referenced entries.
   - Remaining enum, function, plug-in, and nested plug-in edge cases.

4. Integrate interpreter and compiler entry points so unsupported or invalid
   semantic inputs are rejected before execution/code generation.

## Tests

Add tests for:

- BASIC formulas produce no semantic analyzer coverage because they are fully
  parser-validated.
- BASIC parameter sets produce no semantic analyzer coverage because they are
  fully parser-validated.
- Duplicate symbols.
- Unknown variables, functions, constants, types, classes, and members.
- Predefined-symbol rule violations.
- Bad assignment targets.
- Bad call arity and argument types.
- Bad return statements.
- Bad array indexing.
- Bad dynamic array declarations.
- Bad `setLength` and `length` calls.
- Bad `new`, cast, and member access expressions.
- Builtin `Image` fields and methods.
- Formula-kind mismatch in parameter sets.
- Unknown saved parameter names.
- Saved parameter type mismatches.
- Invalid enum, function, and plug-in parameter values.
- Invalid nested plug-in parameter assignments.
- Incomplete retained import graphs.

## Test Slices

Keep tests aligned with the implementation milestones:

1. Classes and builtin objects.
   - Unknown base class.
   - Inheritance cycle.
   - Visibility violation.
   - Cast validity.

2. Extended formula statements and sections.
   - Bad switch case value.
   - Formula-kind-specific section availability not already enforced by the
     parser.

3. Extended parameter-set bindings.
   - Missing referenced entry.
   - Incomplete retained import graph.
   - Remaining enum, function, plug-in, and nested plug-in edge cases.

4. Downstream integration.
   - Interpreter rejects semantic errors before execution.
   - Compiler rejects semantic errors before code generation.
   - Interpreter still reports runtime-only failures.
   - Compiler still reports unsupported lowering failures.

## Documentation

This document defines the semantic-analysis boundary for both formulas and
resolved extended parameter sets. It explicitly excludes BASIC formulas and
BASIC parameter sets. Keep `extended-semantics.md` as background until this plan
is implemented, then merge or retire overlapping material.

## Review Questions

Resolve these before source implementation starts:

- Should formula analysis initially return only diagnostics, or should the
  first slice also include an empty semantic model type?
- Should resolver diagnostics be passed through unchanged, or translated into
  semantic diagnostics at the parameter-set analyzer boundary?
- Should parameter-set saved values be converted in the first analyzer pass, or
  only checked until an interpreter or compiler model needs converted values?
- Should builtin descriptors live in a new semantic analyzer module, or in a
  shared runtime-facing module used by analyzer, interpreter, and compiler?
- Should case-insensitive matching be implemented only where UF semantics
  require it, with source spelling preserved everywhere?
- Which documented `Image` methods and fields should be accepted before the
  interpreter and compiler implement image runtime behavior?
