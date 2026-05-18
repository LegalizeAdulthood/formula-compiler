**Compiler Directives Plan**

**Summary**
- Sources: [compiler directives](https://www.ultrafractal.com/help/writing/reference/directives/compilerdirectives.html), [`$define`](https://www.ultrafractal.com/help/writing/reference/directives/define.html), [`$undef`](https://www.ultrafractal.com/help/writing/reference/directives/undef.html), [`$ifdef`](https://www.ultrafractal.com/help/writing/reference/directives/ifdef.html), [`$else`](https://www.ultrafractal.com/help/writing/reference/directives/else.html), [`$endif`](https://www.ultrafractal.com/help/writing/reference/directives/endif.html).
- Implement as a pre-lexing source filter used by `parser::create_parser()`, preserving line count for diagnostics.

**Key Changes**
- Add a directive preprocessing step before `Lexer`:
  - Recognize line directives after leading whitespace: `$define NAME`, `$undef NAME`, `$ifdef NAME`, `$else`, `$endif`.
  - Case-fold directive names and symbols.
  - Replace directive lines and inactive lines with blank lines, preserving CRLF/LF layout and line numbers.
  - Pass the filtered text to the existing lexer/parser unchanged.
- Track symbols with a case-insensitive set:
  - Predefine `ULTRAFRACTAL`, `VER20`, `VER30`, `VER40`, `VER50`, `VER60`.
  - `$define` adds a symbol only in an active branch.
  - `$undef` removes a symbol only in an active branch.
- Track conditionals with a separate directive stack:
  - `$ifdef` pushes `{parent_active, condition_matched, active}`.
  - `$else` is allowed once per `$ifdef` and flips active state to `parent_active && !condition_matched`.
  - `$endif` pops.
  - Directives operate across sections and parser `if/else/endif`; no AST interaction.
- Add parser diagnostics:
  - `EXPECTED_DIRECTIVE_ENDIF`
  - `UNEXPECTED_DIRECTIVE_ELSE`
  - `UNEXPECTED_DIRECTIVE_ENDIF`
  - `DUPLICATE_DIRECTIVE_ELSE`
  - `EXPECTED_DIRECTIVE_SYMBOL`
  - `INVALID_COMPILER_DIRECTIVE`
- Do not implement special `DEBUG` behavior beyond ordinary symbol support.

**Tests**
- Lexer/parser behavior:
  - `$define TEST` then `$ifdef TEST` includes code.
  - `$undef TEST` then `$ifdef TEST` excludes code.
  - `$else` selects inactive branch.
  - Nested `$ifdef` blocks obey parent activity.
  - Directives are case-insensitive.
  - Predefined `VER60` branch is active; unknown symbol branch inactive.
  - Directives work across `global:`, `init:`, `loop:`, `bailout:`, `default:`.
- Error cases:
  - `$ifdef` without `$endif`.
  - `$else` without `$ifdef`.
  - `$endif` without `$ifdef`.
  - duplicate `$else`.
  - missing symbol after `$define`, `$undef`, `$ifdef`.
  - unknown `$foo`.
- Diagnostics:
  - Error line numbers match original source after filtering.
  - Inactive invalid code does not produce lexer/parser errors.

**Assumptions**
- Directives are standalone line directives; `$define` embedded inside an expression is invalid.
- Inactive branches are never lexed.
- Existing parser options keep extension behavior unchanged; directives are always recognized before lexing.
- No public AST nodes are added; directives affect source inclusion only.
