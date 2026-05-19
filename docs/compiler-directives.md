**Compiler Directives Status**

**Summary**
- Sources: [compiler directives](https://www.ultrafractal.com/help/writing/reference/directives/compilerdirectives.html), [`$define`](https://www.ultrafractal.com/help/writing/reference/directives/define.html), [`$undef`](https://www.ultrafractal.com/help/writing/reference/directives/undef.html), [`$ifdef`](https://www.ultrafractal.com/help/writing/reference/directives/ifdef.html), [`$else`](https://www.ultrafractal.com/help/writing/reference/directives/else.html), [`$endif`](https://www.ultrafractal.com/help/writing/reference/directives/endif.html).
- Implemented as a standalone `formula::preprocessor::Preprocessor`; clients preprocess source text before handing it to the parser.
- The preprocessor returns the processed text as `std::string`.
- It is not wired into the lexer or parser.

**Implemented**
- Recognizes line directives after leading spaces or tabs:
  - `$define NAME`
  - `$undef NAME`
  - `$ifdef NAME`
  - `$else`
  - `$endif`
- Directive names and symbols are case-insensitive.
- Directive lines are omitted from processed output.
- Inactive branch lines are omitted from processed output.
- Active ordinary source lines are emitted unchanged.
- Directive errors still report original source line numbers.
- Macro names do not expand into source text.
- Macro state is exposed as `MacroList`.
- Internal macro lists are normalized to lowercase, unique, and sorted so lookup uses binary search.

**Predefined Macros**
- `UltraFractalMacros` selects common Ultra Fractal predefined macro sets:
  - `NONE`
  - `ULTRAFRACTAL3`
  - `ULTRAFRACTAL4`
  - `ULTRAFRACTAL5`
  - `ULTRAFRACTAL6`
- `predefined_macros(UltraFractalMacros)` returns a `MacroList` so callers can append custom macros.
- `Preprocessor(UltraFractalMacros)` delegates through `predefined_macros`.
- `Preprocessor(const MacroList &predefined)` accepts custom or combined macro lists.
- UF macro sets are cumulative:
  - `ULTRAFRACTAL3`: `ULTRAFRACTAL`, `VER20`, `VER30`
  - `ULTRAFRACTAL4`: adds `VER40`
  - `ULTRAFRACTAL5`: adds `VER50`
  - `ULTRAFRACTAL6`: adds `VER60`

**Conditional State**
- `$define` adds a symbol only in an active branch.
- `$undef` removes a symbol only in an active branch.
- `$ifdef` pushes separate preprocessor state with parent activity.
- `$else` is allowed once per `$ifdef`.
- `$endif` pops the preprocessor stack.
- Compiler directive hierarchy is separate from formula sections, loops, and conditionals.

**Diagnostics**
- Implemented preprocessor diagnostics:
  - `EXPECTED_DIRECTIVE_ENDIF`
  - `UNEXPECTED_DIRECTIVE_ELSE`
  - `UNEXPECTED_DIRECTIVE_ENDIF`
  - `DUPLICATE_DIRECTIVE_ELSE`
  - `EXPECTED_DIRECTIVE_SYMBOL`
  - `INVALID_COMPILER_DIRECTIVE`

**Tests**
- Unit tests cover:
  - custom predefined macros
  - sorted macro state
  - Ultra Fractal predefined macro sets
  - combining Ultra Fractal and custom macros
  - `$define`, `$undef`, `$ifdef`, `$else`, `$endif`
  - nested inactive conditionals
  - case-insensitive directives and symbols
  - CRLF preservation for active retained lines
  - active source text unchanged
  - directive error cases

**Known Gaps**
- `UltraFractalMacros` stops at UF6; UF docs mention UF7 will define `VER70`.
- `DEBUG` is only an ordinary macro for preprocessing. UF runtime behavior tied to `DEBUG`, such as runtime messages and some runtime errors, belongs outside this preprocessor.
