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
   - Parse class entries, inheritance, imports, visibility, fields,
     methods, constructors, static methods, member access, `new`, casts,
     plug-in params.
   - No semantic validation beyond parse shape in this pass.

## Tests
- Lexer: all new tokens, BASIC rejects extended-only syntax, EXTENDED
  accepts it.
- Expressions: precedence/associativity for `%`, `!`, `.`, `[]`,
  multi-arg calls, `new`.
- Statements: typed vars, arrays, loops, returns, nested blocks.
- Functions: typed/void funcs, const args, by-ref args, recursion syntax.
- Sections: fractal/coloring/transformation/class section order and
  invalid-order errors.
- Regression: all existing parse tests, ID formulas, and workflow command:
  `cmake --workflow rt-default`

## Assumptions
- This pass builds syntax coverage and durable AST only.
- Type checking, object model, imports, runtime behavior, interpreter, and
  shader generation come after parse coverage.
- `Dialect.h` remains shared so lexer does not depend on parser header.
