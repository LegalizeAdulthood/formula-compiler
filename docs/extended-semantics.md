# Extended Formula Semantics

## Summary

Extended semantic analysis is implemented as a diagnostic-only pass for
extended formulas and resolved extended parameter sets.

It runs after parsing and, for parameter sets, after reference resolution:

```text
parse -> resolve references -> analyze semantics -> interpret or compile
```

The analyzer validates extended-language meaning that cannot be known from
syntax alone: names, types, calls, members, classes, builtin use,
predefined-symbol context, formula kind compatibility, saved parameter
bindings, plug-in bindings, and retained reference graph completeness.

The analyzer does not mutate ASTs or parameter-set data. It does not execute
formulas, generate code, convert saved values into runtime values, or return a
typed semantic model.

## Scope

BASIC formulas do not use semantic analysis. BASIC parser success is their
complete static validation boundary. Unknown BASIC identifiers remain valid
runtime-zero variables.

BASIC parameter sets do not use semantic analysis. They are flat parser-owned
assignment data with no formula reference graph, layer structure, imported
classes, or typed parameter metadata.

Extended formulas and extended parameter sets use semantic analysis.

## Stage Ownership

- Parser: malformed text, invalid token sequences, invalid section order,
  invalid section cardinality, impossible AST shapes, unknown predefined
  symbols, and assignments to globally read-only predefined symbols.
- Resolver: files, entries, imported classes, retained referenced entities,
  and missing-reference diagnostics.
- Semantic analyzer: valid syntax and resolved references that still have
  invalid meaning.
- Interpreter/compiler: runtime behavior and unsupported execution or lowering.

When a diagnostic could belong to more than one stage, report it from the
earliest stage that has enough information.

## Public API

Formula analysis:

```cpp
std::vector<SemanticDiagnostic> analyze_formula(
    const FormulaSections &formula,
    const FormulaSemanticContext &context);
```

Parameter-set analysis:

```cpp
std::vector<SemanticDiagnostic> analyze_parameter_set(
    const ExtendedParameterEntry &parameters,
    const ParameterReferenceSet &references,
    const ParameterSetSemanticContext &context);
```

Formula parameter metadata:

```cpp
std::vector<FormulaParameterInfo> collect_formula_parameters(
    const FormulaSections &formula,
    const FormulaSemanticContext &context);
```

`collect_formula_parameters` returns each `default:` parameter type, source
name, whether it has an explicit default, and whether it is a plug-in
parameter. It is diagnostic-free and does not validate the formula. Direct
clients may call it, but the extended interpreter facade calls it during
construction and exposes the result through `parameters()`.

## Context

`FormulaSemanticContext` supplies:

- entry kind
- source name
- retained imported classes
- optional builtin registry

`ParameterSetSemanticContext` supplies:

- optional builtin registry
- referenced formula entries
- retained imported classes

The default builtin registry is used when no registry is supplied.

The analyzer does not load imports or resolve entries. The resolver owns file
and entry lookup, retained imported classes, and missing-reference diagnostics.

## Diagnostics

Diagnostics are currently errors only, though `WARNING` exists in the public
severity enum for future use.

Implemented diagnostic codes:

- `UNKNOWN_SYMBOL`
- `DUPLICATE_SYMBOL`
- `UNKNOWN_TYPE`
- `INVALID_TYPE_CONVERSION`
- `INVALID_ASSIGNMENT_TARGET`
- `INVALID_CALL_TARGET`
- `INVALID_CALL_ARITY`
- `INVALID_ARGUMENT_TYPE`
- `INVALID_MEMBER_ACCESS`
- `INVALID_ARRAY_ACCESS`
- `INVALID_RETURN`
- `INVALID_SECTION_RESULT`
- `INVALID_PARAMETER_BINDING`
- `INVALID_FORMULA_KIND`
- `INVALID_SECTION`
- `INVALID_BUILTIN_USAGE`
- `INCOMPLETE_REFERENCE_GRAPH`
- `INHERITANCE_CYCLE`
- `UNSUPPORTED_SEMANTIC_FEATURE`

Diagnostics carry source location, message, entry name, and section name when
available. Tests should assert diagnostic codes and focused message content,
not full prose.

## Builtin Registry

The builtin registry describes:

- scalar types: `void`, `bool`, `int`, `float`, `complex`, `color`, `string`
- builtin functions
- documented predefined symbols
- builtin classes

`Image` is a builtin class. It is not resolved from the import graph and is
valid as a formula type and parameter type without a defining file. The
analyzer checks its constructor, documented methods, documented fields, method
arity, and argument types.

## Formula Analysis

Formula analysis currently checks:

- duplicate declarations in formula, function, block, and class scopes
- unknown types, including retained imported class base types
- retained class graph completeness
- inheritance cycles
- unknown variables, parameters, functions, classes, and members
- builtin and user function call arity
- argument conversions
- assignment conversions
- assignment target validity not already rejected by the parser
- return placement and return value compatibility
- `const` argument mutation
- by-reference argument targets
- static and dynamic array access
- `setLength` and `length` usage for dynamic arrays
- member access, method calls, constructors, casts, and `new`
- visibility and class/builtin member lookup
- section-specific result rules
- formula-kind and section availability for predefined symbols
- constant-expression availability for predefined symbols

The analyzer validates object and class semantics, but the interpreter still
does not execute user objects/classes. Runtime support is tracked in
`extended-interpreter.md`.

## Parameter Set Analysis

Parameter-set analysis assumes:

- the extended parameter set has parsed successfully
- reference resolution has already run
- resolver diagnostics may be forwarded as incomplete-reference diagnostics

It currently checks:

- fractal reference kind
- layer coloring reference kind
- layer transform reference kind
- resolver diagnostics as incomplete reference graph diagnostics
- complete retained import graphs for referenced formulas and classes
- saved parameter names against referenced formula defaults
- saved value type compatibility
- enum labels and numeric enum indexes
- function parameter targets and target shape
- plug-in selectors
- missing retained plug-in classes
- nested plug-in assignments against selected class defaults
- required forwarded plug-in parameters
- builtin object parameters, including `Image`

Section cardinality and ordering are parser responsibilities. Invalid
parameter-set layer structure should not reach semantic analysis.

## Effective Defaults

Every formula parameter has an effective value before interpretation. Omitted
defaults do not create undefined runtime parameters:

- undeclared and untyped parameters default to complex `(0, 0)`
- typed parameters use type defaults
- `Image` parameters use an empty image
- plug-in parameters use their explicit default class or base class
- function parameters use documented fallback functions

Semantic analysis owns validation of effective defaults that depend on names,
imports, function targets, or class compatibility. Missing host bindings are
not runtime errors.

## Non-Goals

The current analyzer does not:

- analyze BASIC formulas
- analyze BASIC parameter sets
- load or resolve imports
- mutate parsed AST or parameter-set data
- return symbol tables or typed AST annotations
- convert saved parameter values into runtime values
- execute formulas
- generate code

When downstream stages need resolved symbols, member bindings, expression
types, converted parameter values, or runtime binding data, add a separate
semantic model beside the parsed data. Do not write derived data back into the
parsed AST or parameter-set structures.

## Tests

Existing semantic analyzer tests cover:

- no mutation of parsed formula and parameter-set data
- duplicate symbols
- unknown symbols, functions, members, classes, and types
- predefined-symbol rules
- assignment, call, argument, return, and conversion errors
- static and dynamic array errors
- `setLength` and `length`
- `new`, casts, member access, visibility, inheritance, and cycles
- builtin `Image` constructor, fields, and methods
- formula kind mismatches in parameter sets
- unknown saved parameter names
- saved parameter type mismatches
- enum, function, and plug-in parameter values
- nested plug-in assignments
- incomplete retained import graphs

New semantic work should add focused tests beside the implementation slice
that needs it.
