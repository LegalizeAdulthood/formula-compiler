# Separate Basic and Extended Parser Behavior

## Summary
- Keep one shared lexer/parser implementation.
- Replace caller-facing `recognize_extensions` with an explicit `formula::Dialect`.
- Use behavior names, not package names:
  - `Dialect::BASIC`
  - `Dialect::EXTENDED`
- The caller already knows the input kind; do not infer dialect from source text.

## Key Changes
- Add `enum class Dialect` in namespace `formula`.
- Update parser options to carry dialect:
  - `Dialect::BASIC`: legacy/basic grammar.
  - `Dialect::EXTENDED`: extended grammar.
- Replace internal checks of `recognize_extensions` with dialect predicates.
- Keep public parsing simple:
  - `Options{.dialect = Dialect::BASIC}`
  - `Options{.dialect = Dialect::EXTENDED}`
- Optionally keep `recognize_extensions` briefly only as a compatibility adapter, then remove it.

## Lexer Behavior
- `BASIC`:
  - No quoted strings.
  - No `#` constant identifiers.
  - No `@` parameter identifiers.
  - Extended section names and keywords remain ordinary identifiers where current behavior allows.
- `EXTENDED`:
  - Current extension behavior remains enabled.
  - Quoted strings, `#`, `@`, extended sections, type names, booleans, and extended control keywords are recognized.

## Parser Behavior
- `BASIC`:
  - Current legacy formula behavior remains.
  - Parse init/iterate/bailout using existing basic splitting behavior.
- `EXTENDED`:
  - Current section-based parsing remains.
  - Extended sections such as `global:`, `builtin:`, `init:`, `loop:`, `bailout:`, `default:`, and `switch:` remain enabled.
- Compiler directive preprocessing remains separate; callers preprocess before parsing if needed.

## Tests
- Rename or add tests around dialect behavior:
  - basic mode treats extended keywords as identifiers where currently allowed.
  - extended mode recognizes extended sections and keywords.
  - basic mode rejects strings, `#`, and `@` as today.
  - extended mode accepts strings, `#`, and `@` as today.
  - basic formula input parses in `Dialect::BASIC`.
  - extended formula input parses in `Dialect::EXTENDED`.
- Update tool/tests currently setting `recognize_extensions` to use `Dialect`.

## Assumptions
- This is an API clarity refactor, not a grammar rewrite.
- One parser implementation remains shared.
- Dialect is selected by the caller, not autodetected.
- `Dialect` lives in namespace `formula`, so callers use `formula::Dialect`.
