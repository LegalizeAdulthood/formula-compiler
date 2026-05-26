# Section Parser Plan

## Summary
- Add a small core helper that identifies colon-labeled section boundaries in
  a prepared entry body.
- Keep it boundary-only: section name/body views plus their source ranges.
- Keep ordering, cardinality, and body grammar in the caller.
- Refactor gradient and extended parameter set parsing to consume the shared
  split.

## API
Add to the core library:

```cpp
struct Section
{
    std::string_view name;
    std::string_view body;
    SourceRange name_range;
    SourceRange body_range;
};

std::vector<Section> split_sections(std::string_view body,
    SourceLocation body_begin = {});
```

`split_sections` accepts a prepared body. It does not copy, trim, remove
comments, join continuations, normalize line endings, or parse assignments.
Each `Section` contains views into the caller-owned body string and ranges
relative to the supplied `body_begin` location.

## Section Syntax
A section label is recognized at the start of a physical line after optional
spaces or tabs:

```text
name:
```

Rules:

- Names use the same broad label spelling already accepted by gradient and
  parameter parsing: letters, digits, underscore, and hyphen.
- The colon terminates the label.
- Text before the first label is represented by no section. Callers that
  require a leading section should diagnose this from their own grammar.
- A section body starts immediately after the label line ending, so the
  leading section label line is not part of the body.
- A section body ends immediately before the next section label line.
- Non-section lines containing colons remain part of the current body.
- The section name is the view of the label text before the colon, excluding
  leading indentation and excluding the colon.
- The splitter returns views into the caller-owned body string.
- The result vector preserves input section order.
- The splitter performs no allocation except the result vector.

## Source Locations
`split_sections` records ranges for the returned views:

- `name_range` covers the label text only. It excludes leading indentation and
  excludes the colon.
- `body_range` covers the body view only. It starts after the section-label
  line ending and ends immediately before the next section label line or EOF.
- If `body_begin.filename` is set, it is propagated through all returned
  ranges.
- Line and column tracking preserves CRLF, LF, and CR line ending semantics.

Callers that have a `FileEntry` should pass `file_entry.body_range.begin` as
`body_begin`.

## Refactor Targets
Gradient parser:

- Replace the local header token loop with `split_sections(file_entry.body)`.
- Parse each section body with the existing key/value lexer.
- Keep the current required-section checks for `gradient` and
  `opacity`/`alpha`.
- Keep gradient-specific errors for unknown section names and body keys.

Extended parameter set parser:

- Use `split_sections` after optional decompression.
- Keep basic parameter parsing line-oriented and unchanged.
- Parse extended parameter sections in order from the returned list.
- Preserve existing diagnostics for missing `fractal`, missing `layer`,
  unexpected sections, out-of-order sections, and bad assignments.
- Keep assignment parsing inside the parameter parser.

## Implementation Slices
1. Refactor gradient parsing to use `split_sections`; keep existing behavior
   and tests green.
2. Refactor extended parameter set parsing to use `split_sections`; keep basic
   parameter parsing unchanged.
3. Add regression tests that comments and continuations are still handled only
   by `FileEntry` before section splitting.
4. Run `cmake --workflow rt-default`.

## Non-Goals
- Do not parse formulas with this helper in this pass.
- Do not add section ordering or required-section policy to core.
- Do not make `Section` own strings.
- Do not move assignment parsing into core.
