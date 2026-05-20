# Ultra Fractal Parse + AST Coverage Plan

## Summary
- Target parse + AST coverage, not execution/codegen.
- Prioritize procedural core first: declarations, types, arrays, loops,
  functions, returns, params, constants.
- Keep BASIC behavior intact. Add features only under EXTENDED.
- Preserve CRLF in touched text files; source stays ASCII.

## Key Changes
- Add AST nodes for: typed variable declarations, array
  declarations/indexing, multi-arg calls, return, while, repeat/until,
  function declarations, parameter refs, constant refs, bool/string/color
  literals.
- Extend lexer with UF operators/tokens: `[ ]`, `.`, `%`, `!`, `&`,
  `public`, `protected`, `private`, `class`, and `final`; typed, color,
  bool, and string literal support is already implied by docs.
- Replace single-arg call shape with argument-list calls. Existing unary
  built-ins become one-arg calls.
- Add `EntryKind` to parse options: `FRACTAL`, `COLORING`,
  `TRANSFORMATION`, `CLASS`. Default stays `FRACTAL`.
- Add import metadata so formula loading can parse import chains before
  semantic checks. Imports are syntax in the source file, but their
  effect is loader-level class availability, not runtime execution.
- Add `Options::file_importer`, a
  `std::function<std::string(std::string_view)>`, so callers supply
  imported file text. The parser never locates files itself.
- Add section rules by kind:
  - fractal: existing order plus perturb sections.
  - coloring: `global`, `init`, `loop`, `final`, `default`.
  - transformation: `global`, `transform`, `default`.
  - class: visibility sections plus optional `default`.
- Do not evaluate new nodes yet. Visitors get stub methods; formatter
  prints stable shapes; interpreter/codegen reject unsupported nodes
  clearly.

## Implementation Order
1. Lexer + expression grammar.
   - Add missing tokens.
   - Parse operator precedence exactly from UF docs: `new`,
     member/index/call, unary `!`/`-`, power, `* / %`, `+ -`,
     comparisons, `&&`, `||`, assignment.
   - Add multi-arg calls and parenthesized arg lists.

2. Statements + declarations.
   - Parse typed declarations with optional initializer.
   - Parse static and dynamic arrays.
   - Parse `while/endwhile`, `repeat/until`, `return`.
   - Keep current `if/elseif/else/endif`.

3. Functions.
   - Parse return type optional, `func name(args)`, `endfunc`.
   - Parse `const` args and `&` by-ref args as metadata.
   - Allow declarations anywhere a statement can appear.

4. Formula kinds.
   - Add kind-specific section parsing.
   - Add `final` and `transform` sections.
   - Preserve legacy fractal splitting rules.

5. Objects/classes later.
   - Parse class entries, inheritance, visibility, fields, methods,
     constructors, static methods, member access, `new`, casts,
     plug-in params.
   - No semantic validation beyond parse shape in this pass.

6. Importing.
   - Parse `import "file"` statements anywhere a formula or class can
     contain statements.
   - Record import statements in source order without treating them as
     executable runtime statements.
   - Add a formula loading pass that calls `Options::file_importer` for
     imported file text, preprocesses and parses imported files, and
     follows chained imports.
   - Report missing imported files, import cycles, and syntax errors in
     imported files as load/parse diagnostics attached to the importing
     formula.
   - Treat `class Derived(File.ufm:Base)` as an implicit import of
     `File.ufm` before explicit imports, matching UF class lookup rules.
   - Preserve import order for later semantic class lookup: the last
     imported file is searched first, then earlier imports, then the
     current file.

## Tests
- Lexer: all new tokens, BASIC rejects extended-only syntax, EXTENDED
  accepts it.
- Expressions: precedence/associativity for `%`, `!`, `.`, `[]`,
  multi-arg calls, `new`.
- Statements: typed vars, arrays, loops, returns, nested blocks.
- Functions: typed/void funcs, const args, by-ref args, recursion syntax.
- Sections: fractal/coloring/transformation/class section order and
  invalid-order errors.
- Imports: quoted filename syntax, import anywhere, chained imports,
  import order preservation, implicit ancestor-file import, missing
  files, cycles, and imported-file syntax errors.
- Regression: all existing parse tests, ID formulas, and workflow command:
  `cmake --workflow rt-default`

## Assumptions
- This pass builds syntax coverage and durable AST only.
- Import loading parses imported entries and reports syntax/load errors,
  but class resolution remains semantic work after parse coverage.
- Type checking, object model, runtime behavior, interpreter, and shader
  generation come after parse coverage.
- `Dialect.h` remains shared so lexer does not depend on parser header.
