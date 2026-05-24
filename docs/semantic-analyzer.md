# Semantic Analyzer Implementation Plan

## Summary

This document tracks implementation work for the semantic analyzer only.
Semantic-analysis behavior and stage ownership are documented in
`extended-semantics.md`.

The analyzer is already implemented as a diagnostic-only pass for extended
formulas and resolved extended parameter sets. Remaining work is integration,
future model shape, and checks that become necessary as interpreter/compiler
support expands.

## Completed Baseline

- diagnostic type and stable diagnostic codes
- builtin registry
- formula contexts
- parameter-set contexts
- retained class graph checks
- formula symbol, type, call, member, section, class, and builtin checks
- parameter-set formula kind, saved value, function, plug-in, nested plug-in,
  and retained graph checks
- diagnostic-free formula parameter metadata collection

## Implementation Slices

1. Function-parameter alignment.
    - As interpreter function-parameter binding is implemented, compare runtime
      target validation with semantic checks.
    - Add any missing semantic checks for target shape, callable kind, and
      global-section restrictions.
    - Add analyzer tests for each semantic-only failure case.

2. Plug-in-default alignment.
    - As interpreter plug-in binding is implemented, compare runtime selector
      validation with semantic checks.
    - Add any missing semantic checks for omitted plug-in defaults, explicit
      default selectors, selected-class compatibility, and retained class
      graph completeness.
    - Add analyzer tests for semantic failures that should block construction.

3. Parameter-set binding model decision.
    - Wait until interpreter or compiler parameter-set execution needs derived
      binding data.
    - If needed, design a separate model that carries converted saved values,
      resolved function targets, and resolved plug-in selections.
    - Keep the existing diagnostic-only API and parsed parameter-set data
      unchanged.

4. Formula semantic model decision.
    - Wait until interpreter or compiler execution needs typed expressions,
      resolved calls, or member bindings from analysis.
    - If needed, design a separate model beside the AST.
    - Keep AST nodes source-preserving and unmodified.

5. Interpreter integration regression.
    - Keep construction rejecting semantic diagnostics before execution.
    - Keep post-construction parameter binding diagnostics owned by the
      interpreter.
    - Add regression tests when new interpreter entry points consume semantic
      analyzer output.

6. Compiler integration regression.
    - Keep compiler entry points rejecting semantic diagnostics before code
      generation.
    - Keep compiler diagnostics focused on unsupported lowering or codegen
      failures after semantic analysis succeeds.
    - Add regression tests when extended compiler entry points are introduced.

7. Warning support.
    - Add warning-producing checks only after callers have policy for display,
      suppression, and test expectations.
    - Keep current diagnostics as errors until then.

## Test Policy

Each implementation slice should add focused tests in `SemanticAnalyzer-test`.
Prefer diagnostic-code assertions and minimal message-content checks.
