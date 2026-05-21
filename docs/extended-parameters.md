# Ultra Fractal Parameter File Plan

## Summary
- Sources: `docs/uf6/*.txt` and the 39 `*.upr` files in
  `../uf-formulas`.
- A `.upr` file is a plain-text entry file. The client scans the file
  with the generic entry scanner and gets a vector of `FileEntry`
  structures.
- Most entries are parameter sets. An entry named `comment` is not a
  parameter set; the client recognizes that name and does not pass the
  body to the parameter parser.
- Compressed entries can be parameter sets or inline formula/coloring/
  transform/class bodies. The client uses `FileEntry.name` metadata to
  route inline entries, such as `*.ufm:*`, `*.ucl:*`, `*.uxf:*`, and
  `*.ulb:class:*`, away from the parameter parser.
- The client knows whether each parameter body is basic or extended and
  passes that dialect to the body parser.
- Parameter parsing is string-preserving. It keeps assignment order,
  repeated keys, raw value text, and source locations. Value semantics
  belong to the client.

## UF6 Semantics
- UF6 docs describe parameter files as collections of parameter sets
  that describe fractals without storing calculated pixels.
- Parameter files are plain text and can be opened in a text editor.
- UF can save formulas with a parameter set. Those embedded formulas
  appear as top-level entries in the same `.upr` file.
- UF can compress parameter sets larger than 2 KB. Compressed entries
  use `::` followed by wrapped encoded data.
- Saved parameter names bind to formula parameter identifiers. Parameter
  forwards let newer formulas accept old saved parameter names.

## Entry Scanning
The entry scanner should treat `{` and `}` as tokens, not as
line-oriented delimiters. Corpus files include `}NextEntry {` on one
line.

```text
file        ::= item* eof
item        ::= entry | space
entry       ::= entry_name "{" entry_body "}"
entry_name  ::= text_until_lbrace_trimmed
entry_body  ::= compressed_body
              | parameter_body
              | raw_comment_body
space       ::= whitespace | comment
comment     ::= ";" text_to_end_of_line
```

The parameter parser does not scan files or classify top-level entries.
The client calls the generic entry scanner, receives `FileEntry`
structures, skips entries named `comment`, routes inline
formula/coloring/transform/class entries by `FileEntry.name`, and passes
only parameter-set bodies to the parameter parser.

`entry_name` is intentionally permissive. Observed names include
digits, spaces, unicode bytes, `.`, `:`, `,`, `[`, `]`, `(`, `)`, `&`,
`+`, `/`, and `-`. Do not require formula identifier syntax here.
Comments are whitespace for parsing purposes and can appear anywhere
whitespace can appear, not only between entries.

## Body Dialects
The client selects the body dialect. The parser never guesses.

Body parsing is line-oriented string processing:

1. Split the body into physical input lines.
2. Strip comments from each line. A `;` starts a comment except inside
   a quoted string.
3. Apply line continuation. A line ending in `\` is joined with the next
   physical line; the `\`, newline, and leading whitespace on the next
   physical line are removed. Repeat until no processed line ends with a
   continuation.
4. In the extended dialect, a line containing only `name:` starts a
   section.
5. Otherwise, a non-blank line is a whitespace-separated list of
   `name=value` pairs. Whitespace inside a quoted string is part of the
   value and is not a pair boundary.

## Basic Parameter Body

Basic parameter-set bodies are never compressed. They are a list of
name/value pairs. Names and values are both strings; the client performs
all further interpretation.

```text
basic_body ::= line*
line       ::= assignment_list | blank
assignment_list ::= assignment (space assignment)*
assignment ::= key "=" value
key        ::= text_until_equals
value      ::= quoted_value | atom_value
```

Basic bodies have no section names.

Quoted strings and atom values both become string values in the AST.
The parser preserves the string spelling after lexical quote handling;
it does not convert values to bools, numbers, complex pairs, colors, or
enums.

## Extended Parameter Body

Extended parameter-set bodies may be compressed. The client does not
need to know or care: if the body is compressed, the parser decompresses
it first, then invokes the extended body parser on the decompressed
payload.

Section names and their terminating colon always appear on a line by
themselves.

```text
extended_body ::= line*
line          ::= section_label | assignment_list | blank
section_label ::= section_name ":" line_end
assignment_list ::= assignment (space assignment)*
assignment    ::= key "=" value
key           ::= text_until_equals
value         ::= quoted_value | atom_value
quoted_value  ::= '"' quoted_char* '"'
atom_value    ::= non_space_without_delimiters
```

The extended dialect requires a named section before any `name=value`
pair. A section ends at the next section label or the end of the body.

All values are string values. Exact value semantics are not documented
well enough for the parser to impose types. The client interprets
strings by context, for example treating `title` as a string and parsing
`width` as a number.

## Sections
Observed parameter sections:

- `fractal`
- `layer`
- `mapping`
- `transform`
- `formula`
- `inside`
- `outside`
- `gradient`
- `opacity`
- `alpha`

Unknown section names should be preserved, not dropped.

`fractal` describes the whole parameter set. `layer` starts a new layer.
Following `mapping`, `transform`, `formula`, `inside`, `outside`,
`gradient`, `opacity`, and `alpha` sections belong to the current layer
until the next `layer` section or the end of the body.

`transform` can repeat within a layer. `gradient`, `opacity`, and
`alpha` use repeated `index` keys followed by repeated color/opacity
keys, so assignment order is semantic. The parser preserves order; the
client decides what the keys mean.

## Values
Observed scalar forms:

- strings: `"Background"`, `"Arithmetic mean"`
- bool-like atoms: `yes`, `no`, `on`, `off`
- enum atoms: `linear`, `multipass`, `normal`, `sqrt`
- integers and unsigned color values: `640`, `4280734340`
- floating point and exponent notation: `1.5`, `1E-16`
- complex pairs: `-0.5/0.25`
- function names: `ident`, `sin`, `atanh`
- animation strings: `"1@#0SS50@#47840E"`

Do not normalize value spelling during parse. Typed conversion belongs
to the client.

## Formula References
The sections `formula`, `inside`, `outside`, and `transform` can contain
`filename` and `entry` assignments. The file extension identifies the
referenced formula family:

- `.ufm`: fractal formula
- `.ucl`: coloring algorithm
- `.uxf`: transformation
- `.ulb`: class library or plug-in library

Saved formula parameters are regular assignments in those sections.
`p_` keys bind to formula parameters. `f_` keys bind to function
parameters such as `f_fn1`, `f_fn2`, `f_fn3`, and `f_fn4`.

The parameter parser must not locate these files or interpret these
references. The client reads the string assignments and resolves files
if needed.

## Comment Entries
An entry named `comment` can contain raw prose until the closing `}`.
These bodies do not follow the parameter grammar. The client recognizes
the entry name and does not pass the body to the parameter parser.

## Compressed Extended Bodies
A body that begins with `::` after optional comments and whitespace is
compressed. Its payload is wrapped UF base64 text.

Basic parameter bodies are never compressed. Extended parameter bodies
may be compressed. When a compressed body is passed to the parser, the
parser decompresses it and then parses the decompressed text as an
extended body.

Corpus note: compressed parameter-set entries decode to extended bodies
that begin with `fractal:`. Other compressed entries decode to inline
formula/coloring/class source and are distinguishable by entry names
such as `file.ufm:Entry`, `file.ucl:Entry`, and
`file.ulb:class:Name(Base)`. The client routes those inline entries
elsewhere before invoking the parameter parser.

Decode compressed bodies as follows:

1. Skip comments and blank lines before `::`, treating comments as
   whitespace.
2. The first data line must start with `::`; remove that prefix.
3. Trim each payload line, skipping comments and blank lines. Each
   payload line length must be a multiple of 4.
4. Decode with the normal base64 alphabet, but UF packs bits in little
   endian order within each 4-character group:
   ```text
   q0 q1 q2 q3 = 6-bit alphabet indexes
   b0          = q0 | ((q1 & 0x03) << 6)
   b1          = ((q1 & 0x3c) >> 2) | ((q2 & 0x0f) << 4)
   b2          = ((q2 & 0x30) >> 4) | (q3 << 2)
   ```
   `=` padding is allowed only in `q2` or `q3`; remove the matching number
   of trailing decoded bytes from that group.
5. Concatenate all decoded bytes.
6. The first four bytes are a little-endian CRC32 of the remaining
   bytes. Reject the entry if it does not match.
7. The remaining bytes are a zlib stream. Inflate them.
8. Parse the inflated text as the body. The inflated text does not
   include the outer entry name or braces.

## AST Shape

```text
BasicParameterBody
  assignments: [Assignment]

ExtendedParameterBody
  sections: [Section]

Section
  name
  assignments: [Assignment]

Assignment
  key
  value: string
  source_range
```

Use ordered vectors everywhere. Build convenience lookup helpers and
semantic layers on top of the ordered string representation.

## Semantic Resolution Plan
1. Client scans the file into `FileEntry` structures.
2. Client skips entries named `comment`.
3. Client routes inline formula/coloring/transform/class entries by
   `FileEntry.name` metadata.
4. Client chooses basic or extended body parsing for each remaining
   parameter-set entry.
5. Parser returns ordered string assignments, or ordered sections of
   string assignments.
6. Client builds typed layer objects while preserving all raw sections.
7. For each `formula`, `inside`, `outside`, and `transform` section,
   collect `(filename, entry, kind)` references.
8. Ask a caller-provided resolver for the referenced formula files.
   This follows the same rule as formula imports: the client locates
   files; the parser only consumes supplied text.
9. Parse referenced entries with the extended formula parser and its
   import support, preserving imported filenames in source locations.
10. Validate `p_` and `f_` assignments against the referenced formula
   entry's parameter declarations, function parameters, plug-in defaults,
   imported classes, and parameter forwards.
11. Diagnose missing files, missing entries, unknown parameter names,
   missing required values, and type mismatches with `.upr` source
   locations.
12. Keep unknown assignments as raw data so older or newer UF files can
   still be inspected and round-tripped.

## Implementation Slices
1. Reuse `FileEntry` scanning at the client boundary.
2. Add body line preprocessing: strip comments outside quoted strings
   and repeatedly join continued physical lines.
3. Add `name=value` splitting for a processed line, honoring quoted
   strings so whitespace inside quotes is not a pair boundary.
4. Add basic body parsing: ordered string name/value assignments with
   no sections.
5. Add extended body parsing: require section-label lines, then ordered
   string name/value assignments inside each section.
6. Add compressed extended-body decoding: UF base64, CRC32 validation,
   zlib inflate, then parse the inflated body text.
7. Add client-side helpers for interpreting string values as bools,
   numbers, complex pairs, colors, and repeated gradient stops.
8. Add client-side reference extraction for formula/coloring/transform
   sections.
9. Add a resolver API and semantic checks against formula AST metadata.

## Tests
- Unit parse basic name/value parameter bodies.
- Unit parse extended one-layer and multi-layer parameter bodies.
- Verify clients can skip `comment` `FileEntry` bodies.
- Verify clients can route compressed inline formula/coloring/class
  entries by `FileEntry.name` without invoking the parameter parser.
- Decode compressed parameter-set bodies that begin with `fractal:`.
- Decode compressed bodies with known payloads.
- Reject compressed bodies with bad line length, base64, CRC, or zlib.
- Parse `}NextEntry {` at the `FileEntry` layer without losing either
  entry.
- Preserve repeated gradient, opacity, and alpha assignments in order.
- Strip comments outside quoted strings, preserving semicolons inside
  quoted strings.
- Join continued physical lines repeatedly before parsing assignments.
- Split assignment lists on whitespace except inside quoted strings.
- Reject extended assignment lines before any section label.
- Parse `transform` sections and `numtransforms`/`transforms` counts.
- Validate `p_` and `f_` keys against fixture formula entries.
- Validate parameter forwards and plug-in sub-parameters.
- Corpus test every `*.upr` in `../uf-formulas` parses and classifies.
