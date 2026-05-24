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
- no parameter-set binding model; runtime binding data stays owned by the
  interpreter or compiler until a shared derived model is needed
- no formula semantic model; typed expressions, resolved calls, and member
  bindings stay out of the AST until a shared derived model is needed

## Implementation Slices

1. Interpreter integration regression.
    - Keep construction rejecting semantic diagnostics before execution.
    - Keep post-construction parameter binding diagnostics owned by the
      interpreter.
    - Add regression tests when new interpreter entry points consume semantic
      analyzer output.

2. Compiler integration regression.
    - Keep compiler entry points rejecting semantic diagnostics before code
      generation.
    - Keep compiler diagnostics focused on unsupported lowering or codegen
      failures after semantic analysis succeeds.
    - Add regression tests when extended compiler entry points are introduced.

3. Warning support.
    - Add warning-producing checks only after callers have policy for display,
      suppression, and test expectations.
    - Keep current diagnostics as errors until then.

## Test Policy

Each implementation slice should add focused tests in `SemanticAnalyzer-test`.
Prefer diagnostic-code assertions and minimal message-content checks.
