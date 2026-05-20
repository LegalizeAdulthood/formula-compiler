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
- Add a per-entry import scope. The parse/load phase uses it to resolve
  class names found in base classes, declarations, casts, `new`, and
  plug-in parameters against parsed imported class ASTs and the current
  file.
- Add `Options::file_importer`, a
  `std::function<std::string(std::string_view)>`, so callers supply
  imported file text. The parser never locates files itself.
- Add `Options::source_filename` and preserve it in diagnostics so
  imported source locations keep their original filename.
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

6. Import metadata.
   - Parse `import "file"` statements anywhere a formula or class can
     contain statements.
   - Record import directives in source order without returning them as
     executable section AST statements.
   - Store directives on the parsed entry, e.g. filename, source
     location, and whether the import was implicit.

7. File and class indexes.
   - Add a formula loading pass that calls `Options::file_importer` for
     imported file text, preprocesses and parses imported files, follows
     chained imports, and retains only referenced imported ASTs.
   - Set `Options::source_filename` while preprocessing and parsing
     imported text so every diagnostic keeps file, line, and column.
   - Report missing imported files, import cycles, and syntax errors in
     imported files as load/parse diagnostics attached to the importing
     formula.
   - Load imported files first into `FormulaEntry` records and build a
     file-level class header index. Do not retain every imported class
     AST.
   - Treat `class Derived(File.ufm:Base)` as an implicit import of
     `File.ufm` before explicit imports, matching UF class lookup rules.
   - Build a file-level class index for every loaded file. Each indexed
     class can parse or expose its AST when referenced. Each entry has an
     import scope from its implicit import, explicit imports in source
     order, and the current file.

8. Reference collection.
   - Fully parse the referencing entry first, then post-process its AST
     with a reference-collection visitor. Collect class references from
     base class specs, declarations, casts, `new`, and plug-in params.

9. Reference resolution.
   - Resolve collected class names by searching explicit imports from
     last to first, then the implicit ancestor import, then the current
     file. Store resolution results as references to parsed class ASTs.
     Report unresolved class names as diagnostics.

10. Lazy imported AST retention.
   - Parse and retain only resolved imported entities referenced by the
     current AST. Repeat reference collection and resolution for each
     retained imported AST until no new referenced entities are found.

11. Public load result.
   - Expose resolved imports as attached parse/load metadata: main entry
     AST, referenced imported ASTs, import graph, diagnostics, and a
     resolved class table. Do not retain unreferenced imported entry ASTs
     and do not inline imported class ASTs into the main entry's
     executable section AST.

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
  import order preservation, imported diagnostic filenames, implicit
  ancestor-file import, referenced imported AST retention, imported
  class-reference resolution, missing files, cycles, and imported-file
  syntax errors.
- Regression: all existing parse tests, ID formulas, and workflow command:
  `cmake --workflow rt-default`

## Assumptions
- This pass builds syntax coverage and durable AST only.
- Import loading parses imported files enough to build class indexes,
  retains only referenced imported ASTs, and resolves referenced class
  names to those ASTs in the entry import scope. Detailed type, member,
  and overload checks remain semantic work after parse coverage.
- Referenced imported entities are discovered by a post-parse AST walk.
  Resolution is iterative because retained imported ASTs can introduce
  more class references.
- Import directives are metadata and loader work, not executable AST
  nodes.
- Imported class ASTs are attached through parse/load metadata, not
  physically embedded into the importing entry AST.
- Type checking, object model, runtime behavior, interpreter, and shader
  generation come after parse coverage.
- `Dialect.h` remains shared so lexer does not depend on parser header.
