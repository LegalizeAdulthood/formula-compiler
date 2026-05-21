# Extended Formula Semantic Analysis Plan

## Summary

Add a `SemanticAnalyzer` pass between parse/load and
interpreter/compiler execution. The parser remains responsible only for
syntax and AST shape. Semantic analysis validates names, types, callable
signatures, member access, object/class use, section rules that require
type knowledge, and builtin class behavior.

The analyzer consumes:

- the main parsed AST
- parsed default metadata
- entry kind
- resolved import metadata
- retained imported class ASTs
- host-provided builtin type and function descriptors

The analyzer produces:

- semantic diagnostics
- symbol tables
- typed AST or typed IR annotations
- resolved function/member/class references
- parameter metadata
- section result type expectations

## Non-Goals

- Do not move grammar checks out of the parser.
- Do not execute formulas.
- Do not generate machine code.
- Do not require imported ASTs that were not referenced by the main AST.
- Do not type-check unreachable imported classes unless they are
  retained.

## Responsibilities

- Resolve identifiers:
  - locals
  - formula variables
  - function parameters
  - functions
  - parameters referenced with `@`
  - constants referenced with `#`
  - fields and methods
- Resolve types:
  - builtin scalar types
  - builtin object types such as `Image`
  - imported classes
  - current-file classes
  - array element types
- Validate expressions:
  - operator operand types
  - assignment target validity
  - assignment conversion
  - array indexing and rank
  - function calls
  - method calls
  - member access
  - casts
  - `new`
- Validate statements:
  - declaration types and initializers
  - return placement and return type
  - loop conditions
  - by-reference argument lvalues
  - const parameter assignment rejection
  - global read-only rules outside `global:`
- Validate metadata:
  - default settings per entry kind
  - parameter block settings per parameter type
  - function blocks in `default:`
  - headings in `default:`
  - switch target and parameter mappings when target metadata is
    available
- Validate imports:
  - unresolved classes
  - retained class metadata consistency
  - class inheritance references
  - duplicate or ambiguous lookup where UF rules require diagnostics

## Pipeline

1. Build builtin descriptors.
   - Register builtin scalar types.
   - Register builtin classes such as `Image`.
   - Register builtin functions and operators.
   - Register predefined constants such as `#pixel`, `#z`, `#index`,
     `#color`, and `#solid`.

2. Build entry symbol tables.
   - Collect declarations from formula sections.
   - Collect user functions before checking calls.
   - Collect default parameter blocks and defaults.
   - Collect class fields, methods, constructors, and static members.

3. Build import/class environment.
   - Read resolved references and retained imported ASTs from load
     metadata.
   - Build class descriptors for retained imported classes.
   - Add current-file class descriptors.
   - Add builtin class descriptors outside the import graph.

4. Analyze default metadata.
   - Validate default settings by entry kind.
   - Validate parameter blocks and function blocks.
   - Validate heading blocks.
   - Build typed parameter metadata for execution.

5. Analyze class descriptors.
   - Resolve base classes.
   - Validate inheritance cycles.
   - Validate field and method declarations.
   - Validate visibility and override rules.
   - Compute object layout metadata for later runtime/compiler use.

6. Analyze sections.
   - Analyze `global`, `init`, `loop`, `bailout`, `perturbinit`,
     `perturbloop`, `final`, and `transform`.
   - Apply section-specific expected result types.
   - Enforce global variable read-only rules outside `global:`.

7. Analyze expressions and statements.
   - Walk each AST node.
   - Produce typed results and resolved references.
   - Emit diagnostics for invalid constructs.

8. Emit analyzer result.
   - Return diagnostics.
   - Return symbol tables and typed annotations.
   - Return metadata needed by interpreter and compiler.

## Type Model

- `void`
- `bool`
- `int`
- `float`
- `complex`
- `color`
- `string`
- arrays
- class/object references
- builtin object references
- unknown/error type

The analyzer should use an error type after diagnostics so one invalid
expression does not cascade into many duplicate errors.

## Conversion Rules

- Numeric promotions:
  - `bool -> int -> float -> complex`
- Assignment converts to the declared target type when allowed.
- Function arguments convert to parameter types when allowed.
- Return values convert to declared return types when allowed.
- `color` does not convert to numeric values automatically.
- `string` only converts where UF semantics explicitly allow it.
- Object references convert through inheritance where valid.
- Invalid conversions produce semantic diagnostics.

## Function Calls

- Resolve user functions before builtin functions.
- Validate arity.
- Validate argument conversions.
- Validate `const` and `&` parameters.
- Reject non-lvalue arguments passed by reference.
- Support recursion by building declarations before bodies are checked.
- Detect duplicate function signatures when UF rules disallow them.
- Keep unresolved functions as semantic diagnostics, not syntax errors.

## Member Access And Methods

- Resolve receiver type first.
- For class receivers:
  - find fields
  - find methods
  - apply visibility rules
  - validate method arity and argument types
- For builtin class receivers:
  - use builtin descriptors
  - validate documented members and methods
  - reject unknown members with semantic diagnostics
- For non-object receivers:
  - reject field and method access.

## Builtin Image Class

`Image` is a builtin class/type. It is not parsed from an imported file
and must not be resolved through the import graph.

Semantic analyzer work:

- Add a builtin `Image` descriptor.
- Treat `Image param name` as builtin image parameter metadata.
- Do not emit unresolved-class diagnostics for `Image`.
- Validate documented `Image` methods and fields against the UF6 image
  parameter docs.
- Validate method arity and return types.
- Mark unsupported documented methods as unsupported semantic
  diagnostics until implemented.
- Reject unknown `Image` fields and methods as semantic diagnostics.
- Expose typed image metadata to interpreter and compiler.

## Parameters

- Parse metadata becomes typed parameter descriptors.
- Validate parameter block settings by parameter type.
- Validate defaults against parameter type.
- Validate `min`, `max`, `enum`, `visible`, `enabled`, `expanded`,
  `exponential`, and `selectable`.
- Validate plug-in parameter class types through resolved class metadata.
- Validate builtin image parameters through the builtin `Image`
  descriptor.
- Treat parameter reads as typed immutable values unless UF rules allow
  writes.

## Constants

- Validate predefined constants by entry kind and section.
- Examples:
  - `#pixel`
  - `#z`
  - `#index`
  - `#color`
  - `#solid`
- Reject writes to read-only constants.
- Allow writes only where UF semantics permit them, such as transform
  updates to `#pixel` and `#solid`.

## Sections

- Fractal:
  - `bailout` must produce a truthy value.
  - `switch` metadata is validated against available target metadata
    where possible.
- Coloring:
  - `final` must produce a valid coloring result.
  - unlabelled coloring entries are treated as `final` by parser/load
    setup, then checked here.
- Transformation:
  - `transform` may mutate `#pixel` and `#solid`.
  - return values are ignored or rejected according to final semantics.
- Class:
  - visibility sections define member visibility.
  - `default:` provides class parameter metadata when used as a plug-in.

## Diagnostics

Diagnostics should include:

- code
- message
- source location
- entry name
- section name
- function or class context where available

Initial diagnostic families:

- unknown identifier
- unknown function
- unknown type
- unknown member
- invalid call arity
- invalid argument type
- invalid assignment target
- invalid conversion
- invalid return value
- invalid section result
- invalid parameter default
- invalid parameter setting
- invalid constant write
- unresolved class
- unsupported semantic feature

## Interpreter Use

The interpreter should receive analyzed metadata and typed annotations.
It should not rediscover symbol tables independently.

- Use typed parameter defaults.
- Use resolved functions and members.
- Use builtin `Image` descriptors and image runtime handles.
- Throw only for runtime failures, not for static semantic errors that
  the analyzer can detect.

## Compiler Use

The compiler should consume the same analyzer result.

- Lower typed annotations to compiler IR.
- Use resolved symbol slots and member descriptors.
- Use builtin `Image` descriptors for helper calls and image handles.
- Refuse compilation when semantic diagnostics contain errors.
- Keep compile diagnostics for codegen failures and unsupported lowering,
  not for semantic errors already found by the analyzer.

## Tests

- Unknown identifier/function/member diagnostics.
- Invalid call arity and argument type diagnostics.
- Invalid assignment target and conversion diagnostics.
- Return type diagnostics.
- Section result diagnostics.
- Parameter default and setting diagnostics.
- `Image param` does not resolve through imports.
- Unknown `Image` method and field diagnostics.
- Supported `Image` method signature validation.
- Plug-in parameter class resolution through retained imports.
- Global read-only diagnostics.
- Constant write diagnostics.
- Interpreter and compiler both reject formulas with semantic errors
  before execution/codegen.

## Implementation Slices

1. Analyzer result and diagnostic types.
2. Builtin type/function/class descriptor registry.
3. Symbol table construction for entries and sections.
4. Type model and conversion checker.
5. Expression analyzer for identifiers, calls, operators, and assignment.
6. Statement analyzer for declarations, control flow, and returns.
7. Default metadata analyzer for settings, params, funcs, and headings.
8. Import/class descriptor integration.
9. Builtin `Image` descriptor and param metadata.
10. Member/method analyzer for classes and builtin classes.
11. Section-specific semantic checks.
12. Interpreter integration.
13. Compiler integration.
