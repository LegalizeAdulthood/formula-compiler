# Extended Interpreter Test Gaps

## Summary
- Cover only gaps confirmed against the UF6 text files.
- Confirmed gaps:
  - color arithmetic
  - global-section read-only variables
- Not a confirmed gap:
  - public import metadata
- Keep these as test-first slices. If tests expose missing implementation,
  update the smallest affected runtime or semantic-analysis surface in the
  same slice.

## Source Semantics

Color arithmetic:
- Source files:
  - `docs/uf6/writing-direct-coloring-algorithms.txt`
  - `docs/uf6/operators/add.txt`
  - `docs/uf6/operators/subtract.txt`
  - `docs/uf6/operators/multiply.txt`
  - `docs/uf6/operators/divide.txt`
- Required operations:
  - `color + color` returns component-wise color sum.
  - `color - color` returns component-wise color difference.
  - `color * float` returns component-wise scaled color.
  - `color / float` returns component-wise divided color.
  - Alpha participates in the same arithmetic as red, green, and blue.
- Required rejection cases:
  - `float * color` is not documented as valid.
  - `color + float`, `color - float`, `color * color`, `color / color`,
    `float / color`, `%` with color, and `^` with color are invalid.

Global-section read-only variables:
- Source file:
  - `docs/uf6/global-section.txt`
- Required behavior:
  - The `global:` section executes once per image.
  - Variables declared in `global:` are readable in other sections.
  - Variables declared in `global:` are read-only in other sections.
  - Variables declared in `global:` remain writable within `global:`.

Import metadata:
- Source file:
  - `docs/uf6/importing-classes.txt`
- There is no documented "public import metadata" rule. The word `public`
  appears because an import statement is shown inside a class `public:`
  section. Existing import tests should remain focused on documented import
  behavior: explicit imports, implicit base imports, import order, position
  independence, missing imports, cycles, and retained referenced ASTs.

## Implementation Slices

1. Add interpreter tests for global-section state sharing.
   - Execute `global:` first, then another legal section.
   - Assert values declared and assigned in `global:` are visible when the
     later section runs.
   - Cover scalar and array readback.

2. Add interpreter or semantic tests for global-section write protection.
   - Prefer semantic analyzer coverage if writes are rejected before runtime.
   - Add interpreter coverage only if a valid AST can still reach the runtime
     write-protection path.
   - Assert writes outside `global:` fail clearly.

3. Reconcile `docs/extended-interpreter.md` test list.
    - Keep `color arithmetic` and `global read-only behavior` until the
      slices above are implemented.
    - Remove or reword `public import metadata`; it is not a documented UF6
      semantic gap.
    - Do not add implementation work for undocumented import metadata.

## Verification
- Run `cmake --workflow rt-default` after each implementation slice.
- Keep BASIC behavior unchanged.
- Keep source files ASCII.
- Normalize touched text files to CRLF on Windows.
