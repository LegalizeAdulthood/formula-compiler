# FileEntry Parser Plan

## Summary
- Make `FileEntry` the shared preparation layer for entry-based parsers.
- `FileEntry.body` becomes the prepared body used by downstream parsers.
- Preparation removes comments and joins strict line continuations.
- Preparation preserves all non-continued source line endings exactly as they
  appeared.
- Raw source recovery is the caller's responsibility through `SourceRange` and
  the original caller-owned source text.

## Goals
- Centralize comment removal and line-continuation handling.
- Remove repeated comment/continuation logic from formula, parameter,
  gradient, and L-system parsers.
- Keep downstream parsers focused on their own grammar.
- Preserve enough source ranges for clients to recover raw text from their
  original buffer.

## FileEntry Shape
Use the existing fields, but define their contracts precisely:

```cpp
struct FileEntry
{
    std::string name;
    std::string paren_value;
    std::string bracket_value;
    std::string body;

    SourceRange source_range;
    SourceRange header_range;
    SourceRange name_range;
    SourceRange body_range;
};
```

Field contracts:

- `name`: trimmed entry name, excluding trailing `(paren)` and `[bracket]`
  metadata.
- `paren_value`: value from trailing parenthesized metadata, without
  parentheses.
- `bracket_value`: value from trailing bracket metadata, without brackets.
- `body`: prepared body with comments removed and strict continuations joined.
- `source_range`: half-open range from the first non-whitespace character of
  the entry header through the closing brace.
- `header_range`: trimmed raw header span, excluding leading and trailing
  whitespace before `{`.
- `name_range`: trimmed raw name span, excluding trailing metadata and
  surrounding whitespace.
- `body_range`: raw body slice between braces, excluding braces.

`source_range.end` should point just after the closing brace. `body_range`
should continue to identify the raw body slice in the original source, even
though `body` is prepared text.

## Prepared Body Rules
Preparation is performed while scanning the raw body:

1. Comments begin with `;` outside quoted strings and continue through, but do
   not include, the physical line ending.
2. Removed comments do not remove the line ending. This preserves statement
   boundaries.
3. Quoted strings protect semicolons from comment handling.
4. Inside quoted strings, backslash escapes the next character for comment and
   quote scanning.
5. A line continuation is recognized only when `\` is immediately followed by
   `\r\n`, `\n`, or `\r`.
6. A recognized continuation removes the `\` and the physical line ending.
7. Leading spaces and tabs on the next physical line are removed after a
   continuation.
8. A backslash followed by spaces or tabs before end of line is not a
   continuation.
9. Non-continued line endings are preserved exactly as `\r\n`, `\n`, or `\r`.
10. No other whitespace normalization is performed.

## API Direction
Keep the existing stream overload, and add a text overload:

```cpp
std::vector<FileEntry> load_file_entries(std::string_view text,
    std::string filename = {});
std::vector<FileEntry> load_file_entries(std::istream &in,
    std::string filename = {});
```

The text overload lets clients retain the original source buffer and use
`SourceRange` values to recover raw text. The stream overload remains a
convenience; callers using it get prepared bodies and source locations, but
cannot recover raw body text unless they separately retained the input.

## Downstream Simplification
After `FileEntry.body` is prepared:

- Formula lexer no longer skips comments or line continuations.
- Parameter parser no longer strips comments or joins continuations.
- Gradient parser no longer skips comments.
- L-system parser should consume the prepared body directly.

Downstream parsers may still split physical lines, trim grammar-specific
whitespace, and parse quoted values. They should not implement entry-format
comment or continuation rules.

Entry-based parser APIs should accept `const FileEntry &` only. Raw string
entry-body APIs should not be public entry points for parameter, gradient,
L-system, or future entry-based parsers. Parser-internal helpers may still
accept `std::string_view`, and formula snippet/test helpers may continue to
parse ad hoc source text when they are not modeling a file entry.

## Diagnostics
`FileEntry` scanning currently has no diagnostic result type. Keep that for
the first implementation slice and preserve permissive behavior.

Future diagnostics can be added later for:

- unterminated quoted strings in body preparation;
- unterminated entries;
- invalid metadata brackets;
- strict-continuation warnings if needed.

Do not add diagnostics in the first pass unless an existing caller requires
them.

## Implementation Slices
1. Add `load_file_entries(std::string_view, std::string)` and route the
   existing stream overload through it. Keep existing tests passing.
2. Adjust `FileEntry` ranges:
   - `source_range` starts at the first non-whitespace header character;
   - `source_range.end` is after the closing brace;
   - `header_range` excludes leading/trailing header whitespace;
   - `name_range` excludes trailing metadata and surrounding whitespace;
   - `body_range` remains the raw slice between braces.
3. Change body scanning so `FileEntry.body` is prepared text. Remove comments,
   join strict continuations, preserve non-continued line endings, and keep
   semicolons inside quoted strings.
4. Add `FileEntry` tests for prepared bodies:
   - comments outside strings removed;
   - semicolons inside strings preserved;
   - strict continuations joined;
   - leading spaces/tabs after continuation removed;
   - backslash followed by spaces before EOL is not continuation;
   - CRLF and LF preserved for non-continued lines.
5. Add `FileEntry` tests for ranges:
   - `source_range` excludes leading whitespace and ends after `}`;
   - `header_range` is trimmed;
   - `name_range` excludes `(paren)` and `[bracket]`;
   - `body_range` identifies the raw body between braces.
6. Simplify formula lexer by removing comment and continuation skipping from
   `Lexer`. Update tests that expected lexer-level continuation warnings.
7. Simplify parameter parser by deleting local comment stripping and
   continuation joining. Keep assignment parsing and compressed body handling.
8. Simplify gradient parser by deleting local comment skipping.
9. Update `docs/extended-parameters.md`, `docs/l-system-parser.md`, and any
   parser docs that still describe downstream parsers owning comment or
   continuation handling.
10. Standardize entry parser APIs so parameter, gradient, L-system, and future
    entry-based parsers accept `const FileEntry &` as their public input.
    Keep raw string parsing only for formula snippets, tests, and
    parser-internal helpers.
11. Run corpus-oriented tests and `cmake --workflow rt-default`.

## Non-Goals
- Do not normalize line endings.
- Do not preserve raw body text in `FileEntry.body`.
- Do not implement L-system parsing in this change.
- Do not change formula, parameter, or gradient grammar semantics beyond
  removing duplicated comment/continuation handling.
