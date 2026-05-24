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

## Remaining Work

1. Warning support.
    - Add warning-producing checks only after callers have policy for display,
      suppression, and test expectations.
    - Keep current diagnostics as errors until then.

2. Semantic model API.
    - Add a separate model only when interpreter or compiler work needs typed
      expression data, resolved calls, member bindings, converted saved values,
      or runtime binding data.
    - Keep the parsed AST and parameter-set structures immutable.
    - Do not backfill derived data into parser-owned objects.

3. Interpreter integration checks.
    - Keep interpreter construction rejecting semantic diagnostics before
      execution.
    - Keep post-construction parameter binding diagnostics in the interpreter,
      not in the semantic analyzer.
    - Revisit function parameter and plug-in default validation as interpreter
      binding APIs are implemented, so semantic checks and runtime binding
      agree.

4. Compiler integration checks.
    - Keep compiler entry points rejecting semantic diagnostics before code
      generation.
    - Keep compiler diagnostics focused on unsupported lowering or codegen
      failures after semantic analysis succeeds.

5. Effective-default validation follow-up.
    - Verify semantic checks cover omitted function defaults, omitted plug-in
      defaults, and default class compatibility once interpreter default
      resolution is implemented.
    - Add semantic tests for any default case that currently only has runtime
      coverage.

6. Parameter-set model follow-up.
    - If interpreter/compiler parameter-set execution needs converted saved
      values or resolved function/plug-in bindings, add a model-returning API
      beside the diagnostic-only API.
    - Keep resolver diagnostics and parsed parameter-set data unchanged.

## Test Policy

Each implementation slice should add focused tests in `SemanticAnalyzer-test`.
Prefer diagnostic-code assertions and minimal message-content checks.
