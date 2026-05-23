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

## Diagnostics

Add a semantic diagnostic type separate from parser diagnostics. Parser
diagnostics describe invalid syntax. Semantic diagnostics describe syntactically
valid input that cannot be used correctly.

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

Diagnostics should carry source location when available. For parameter-set
errors, prefer the parameter assignment location. For formula errors, use the
formula source location.

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
