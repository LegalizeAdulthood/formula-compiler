# Extended Formula Semantic Analysis

## Summary

Extended semantic analysis now exists as a diagnostic-only pass after parsing
and, for parameter sets, after reference resolution.

The analyzer validates extended-language meaning that the parser cannot know
from syntax alone: names, types, calls, members, class use, builtin use,
predefined-symbol rules that depend on context, formula kind compatibility,
parameter bindings, plug-in bindings, and complete retained reference graphs.

The analyzer does not execute formulas, generate code, mutate parsed ASTs, or
produce a typed semantic model. Downstream interpreter and compiler integration
is still future work.

## Pipeline

```text
parse -> resolve references -> analyze semantics -> interpret or compile
```

Stage ownership:

- Parser: malformed text, invalid token sequences, invalid section order,
  invalid section cardinality, and impossible AST shapes.
- Resolver: files, entries, imported classes, and retained referenced entities.
- Semantic analyzer: valid syntax and resolved references that still have
  invalid meaning.
- Interpreter/compiler: runtime behavior and unsupported execution or lowering.

BASIC formulas and BASIC parameter sets are outside this pass. BASIC formulas
are statically validated by the parser, and unknown BASIC variables remain
runtime-zero variables. BASIC parameter sets are flat parser-owned data.

## Public Entry Points

The current public analysis surface is diagnostic-only:

```cpp
std::vector<SemanticDiagnostic> analyze_formula(
    const ast::FormulaSections &formula,
    const FormulaSemanticContext &context);

std::vector<SemanticDiagnostic> analyze_parameter_set(
    const parameter::ExtendedParameterEntry &parameters,
    const parameter::ParameterReferenceSet &references,
    const ParameterSetSemanticContext &context);
```

The context supplies entry kind, retained imported classes, and builtin
descriptors. The default builtin registry is used when no registry is supplied.

The semantic analyzer also exposes a diagnostic-free formula parameter metadata
query:

```cpp
std::vector<FormulaParameterInfo> collect_formula_parameters(
    const ast::FormulaSections &formula,
    const FormulaSemanticContext &context);
```

This returns each `default:` parameter type, source name, whether it has an
explicit default, and whether the parameter is a plug-in parameter. Plug-in
classification uses builtin descriptors and retained class context. Direct
clients may call this API, but the normal interpreter facade calls it during
construction and exposes the result through `ExtendedInterpreter::parameters`.

## Diagnostics

The implemented diagnostic codes cover:

- Unknown symbols and types.
- Duplicate symbols.
- Invalid type conversions.
- Invalid assignment targets.
- Invalid call targets, arity, and argument types.
- Invalid member access.
- Invalid array access.
- Invalid returns.
- Invalid section results and invalid section use.
- Invalid parameter bindings.
- Invalid formula kind usage.
- Invalid builtin usage.
- Incomplete retained reference graphs.
- Inheritance cycles.
- Unsupported semantic features.

Diagnostics are errors only for now.

## Builtin Registry

The builtin registry currently describes scalar types, builtin functions,
predefined symbols, and builtin classes.

`Image` is a builtin class. It is not resolved from the import graph and is
valid as a formula type and parameter type without a defining file. The analyzer
checks its constructor, documented methods, documented fields, method arity, and
argument types.

## Formula Analysis

Formula semantic analysis currently checks:

- Duplicate declarations in formula, function, and class scopes.
- Unknown types, including retained imported class base types.
- Retained class graph completeness.
- Inheritance cycles.
- Unknown variables, parameters, functions, classes, and members.
- Builtin and user function call arity.
- Argument conversions.
- Assignment conversions.
- Assignment target validity not already rejected by the parser.
- Return placement and return value compatibility.
- `const` argument mutation.
- By-reference argument targets.
- Static and dynamic array access.
- `setLength` and `length` usage for dynamic arrays.
- Member access, method calls, constructors, casts, and `new`.
- Visibility and class/builtin member lookup.
- Section-specific result rules.
- Predefined-symbol section and formula-kind availability.
- Predefined-symbol constant-context availability.

The parser already rejects unknown predefined-symbol names and assignments to
globally read-only predefined symbols.

## Parameter Set Analysis

Parameter-set semantic analysis assumes the extended parameter set has already
been parsed and reference resolution has already been attempted.

It currently checks:

- Fractal reference kind.
- Layer coloring reference kind.
- Layer transform reference kind.
- Resolver diagnostics as incomplete reference graph diagnostics.
- Complete retained import graphs for referenced formulas and classes.
- Saved parameter names against referenced formula defaults.
- Saved value type compatibility.
- Enum labels and numeric enum indexes.
- Function parameter targets and target shape.
- Plug-in selectors.
- Missing retained plug-in classes.
- Nested plug-in assignments against selected class defaults.
- Required forwarded plug-in parameters.
- Builtin object parameters, including `Image`.

`Image` parameters have an implicit empty default and do not require a saved
parameter assignment.

Section cardinality and ordering are parser responsibilities. Invalid
parameter-set layer structure should not reach semantic analysis.

## Non-Goals

The current analyzer does not:

- Analyze BASIC formulas.
- Analyze BASIC parameter sets.
- Load or resolve imports.
- Mutate parsed AST or parameter-set data.
- Return symbol tables or typed AST annotations.
- Convert saved parameter values into runtime values.
- Integrate with the interpreter.
- Integrate with the compiler.

When downstream stages need resolved symbols, member bindings, expression
types, or converted parameter values, add a separate semantic model beside the
parsed data. Do not write derived data back into the parsed AST.

## Remaining Work

Remaining semantic work is tracked in `semantic-analyzer.md`.

Key deferred areas:

- Complete any remaining formula semantic checks listed there.
- Complete any remaining parameter-set binding checks listed there.
- Add a semantic model only when interpreter or compiler work needs one.
- Integrate the extended interpreter after extended runtime support exists.
- Integrate the extended compiler after extended lowering support exists.
