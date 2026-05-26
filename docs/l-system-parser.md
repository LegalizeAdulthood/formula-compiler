# L-System Parser Plan

## Summary
- Add parse-only support for Iterated Dynamics L-system entries.
- Input is a single `FileEntry`; output is a parsed `LSystem`.
- Assume the `FileEntry` preparation plan is implemented first:
  `FileEntry.body` already has comments removed and strict line
  continuations joined.
- No expansion, drawing, turtle execution, rendering, or parameter handling in
  this pass.
- Source semantics come from `docs/id.txt`, section 2.26, "L-Systems".

## Documented Format
An L-system file is an entry file. The generic entry scanner supplies a
`FileEntry` whose name is the L-system name and whose body contains one
statement per line.

Example from `id.txt`:

```text
Dragon {
  Angle 8
  Axiom FX
  F=
  y=+FX--FY+
  x=-FX++FY-
}
```

Rules from `id.txt`:

- Each L-system entry contains an angle, an axiom, and transformation rules.
- Each item appears on its own line.
- Each line must be less than 160 characters long.
- `angle n` sets the drawing angle increment to `360 / n` degrees.
- `n` must be an integer greater than 2 and less than 50.
- `axiom string` defines the initial command string.
- A transformation rule is `a=string`.
- The rule left side is a single character.
- Multiple rules for the same character are concatenated in source order.
- Rule left sides may be any character except space, tab, or `}`.
- Text after `;` on a line is a comment.
- Drawing commands and other command-string characters are not interpreted by
  the parser.
- Other characters are legal in command strings and are ignored by the
  renderer if they are not drawing commands.
- Reserved symbols cannot be redefined:
  `+`, `-`, `<`, `>`, `[`, `]`, `|`, `!`, `@`, `/`, `\`, and `c`.

## Public API
Place the API in the parser library:

```text
libs/parser/include/formula/parser/LSystem.h
libs/parser/LSystem.cpp
```

Use a diagnostic-owning result, matching the parameter parser style:

```cpp
enum class LSystemParseErrorCode
{
    NONE = 0,
    EXPECTED_STATEMENT,
    EXPECTED_ANGLE_VALUE,
    INVALID_ANGLE_VALUE,
    EXPECTED_AXIOM_VALUE,
    EXPECTED_RULE_NAME,
    INVALID_RULE_NAME,
    RESERVED_RULE_NAME,
    LINE_TOO_LONG,
    DUPLICATE_ANGLE,
    DUPLICATE_AXIOM,
};

struct LSystemDiagnostic
{
    LSystemParseErrorCode code{};
    SourceLocation location;
};

struct LSystemRule
{
    char symbol{};
    std::string replacement;
    SourceLocation location;
};

struct LSystem
{
    std::string name;
    int angle_divisor{};
    std::string axiom;
    std::vector<LSystemRule> rules;
    std::vector<LSystemDiagnostic> diagnostics;
};

LSystem parse_l_system(const FileEntry &file_entry);
```

`LSystemRule` stores the accumulated replacement for each symbol. If a symbol
has multiple rule lines, the parser appends later replacements to the existing
rule while preserving first-rule order in `rules`.

No public raw-string parser entry point is planned. Parser-internal helpers may
accept `std::string_view`, but clients parse L-system entries by passing a
prepared `FileEntry`.

## Parsing Rules
1. Set `LSystem.name` from `FileEntry.name`.
2. Split prepared `FileEntry.body` into physical lines.
3. For each line, compute source location relative to `body_range`.
4. Diagnose `LINE_TOO_LONG` when the prepared physical line length is 160 characters
   or more.
5. Trim leading and trailing whitespace.
6. Skip blank lines.
7. Recognize `angle` and `axiom` keywords case-insensitively because the
   documentation uses lowercase prose and uppercase sample spelling.
8. Parse `angle n`:
   - exactly one integer value is required after `angle`;
   - trailing non-comment text after the integer is an error;
   - values must satisfy `2 < n < 50`;
   - a second `angle` line is diagnosed and ignored.
9. Parse `axiom string`:
    - the remainder of the line after `axiom` and following whitespace is the
      axiom string;
    - an empty remainder is diagnosed;
    - a second `axiom` line is diagnosed and ignored.
10. Parse rule lines:
    - find the first `=`;
    - the left side after trimming must be exactly one character;
    - space, tab, and `}` are invalid rule symbols;
    - reserved symbols are diagnosed and ignored;
    - the right side is preserved exactly after the `=`, except that comments
      have already been removed;
    - empty replacement strings are valid and mean delete the symbol.
11. Any nonblank line that is neither `angle`, `axiom`, nor a rule is an
    `EXPECTED_STATEMENT` diagnostic.

## Case And Spelling
- Preserve `FileEntry.name`.
- Preserve axiom spelling exactly from prepared `FileEntry.body`.
- Preserve rule replacement spelling exactly from prepared `FileEntry.body`.
- Preserve rule symbol case; `x` and `X` are distinct rule symbols.
- Match `angle` and `axiom` case-insensitively.

## Deliberate Non-Goals
- Do not expand the command string by order.
- Do not validate command syntax beyond reserved rule left sides.
- Do not validate numeric suffixes for drawing commands such as `Cnn`, `@nnn`,
  `\nn`, or `/nn`.
- Do not evaluate color, angle, stack, or line-size commands.
- Do not interpret unknown command characters.
- Do not parse `.par` parameters such as `lfile`, `lname`, or `order`.

## Implementation Slices
1. Add `LSystem.h` with AST structs, diagnostic enum, and parser function.
   Add the header/source to the parser library CMake file.
2. Implement prepared-body line splitting, keyword recognition, and diagnostics
   for empty or unknown statements.
3. Implement `angle` and `axiom` parsing with duplicate and value
   diagnostics.
4. Implement rule parsing, repeated-rule concatenation, invalid-symbol
   diagnostics, and reserved-symbol diagnostics.
5. Add focused parser tests for the documented `Dragon` sample, empty
   replacement rules, repeated rule concatenation, comments, line-length
   diagnostics, invalid angles, duplicate angle/axiom lines, invalid rule
   symbols, and reserved rule symbols.
6. Add a small corpus fixture if an `id.l` file is added to the repository.

## Test Matrix
- `Dragon` parses with name `Dragon`, angle divisor `8`, axiom `FX`, and
  rules for `F`, `y`, and `x`.
- `F=` parses as an empty replacement.
- `x=a` followed by `x=b` returns one `x` rule with replacement `ab`.
- `Angle 8` and `Axiom FX` are accepted.
- prepared body input is used directly; comments and continuations are covered
  by `FileEntry` tests.
- `angle 2`, `angle 50`, `angle text`, and missing `angle` values diagnose.
- duplicate `angle` and duplicate `axiom` diagnose and keep the first value.
- `+=F`, `<=F`, `/=F`, `\=F`, and `c=F` diagnose reserved symbols.
- ` =F`, `ab=F`, and `}=F` diagnose invalid rule names.
