# Ultra Fractal Parameter File Plan

## Summary
- Sources: `docs/uf6/*.txt` and the 39 `*.upr` files in
  `../uf-formulas`.
- A `.upr` file is a plain-text entry file. It contains any number of
  top-level entries.
- Most entries are parameter sets. A file can also contain comment
  entries and compressed entries.
- Parameter parsing should be lossless first, typed second. Keep entry
  order, section order, repeated keys, raw scalar text, and source
  locations.

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

## Entry Grammar
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

`entry_name` is intentionally permissive. Observed names include
digits, spaces, unicode bytes, `.`, `:`, `,`, `[`, `]`, `(`, `)`, `&`,
`+`, `/`, and `-`. Do not require formula identifier syntax here.
Comments are whitespace for parsing purposes and can appear anywhere
whitespace can appear, not only between entries.

## Plain Parameter Body

```text
parameter_body ::= body_item*
body_item      ::= section | space
section        ::= section_name ":" section_item*
section_item   ::= assignment | space
assignment     ::= key "=" value
key            ::= non_space_without_delimiters
value          ::= quoted_value | atom_value
quoted_value   ::= '"' quoted_char* '"'
atom_value     ::= non_space_without_delimiters
```

`section_item*` ends at the next section label or the closing `}`.
Whitespace separates assignments; newlines are otherwise insignificant.
Comments are whitespace. Outside quoted strings, `;` starts a comment.
Inside quoted strings it is data, as in `credits="Name;date"`.

Quoted strings can be continued with a trailing backslash before a
newline. Treat the continuation as part of one string value and remove
the physical line break plus indentation.

Atom values are stored raw. Later phases can interpret them as bools,
numbers, complex pairs, colors, enum labels, function names, or
animation encodings.

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

Unknown section names should be preserved and warned about, not dropped.

`fractal` describes the whole parameter set. `layer` starts a new layer.
Following `mapping`, `transform`, `formula`, `inside`, `outside`,
`gradient`, `opacity`, and `alpha` sections belong to the current layer
until the next `layer` or `}`.

`transform` can repeat within a layer. `gradient`, `opacity`, and
`alpha` use repeated `index` keys followed by repeated color/opacity
keys, so assignment order is semantic.

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
to a later semantic phase.

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

The parameter parser must not locate these files. It should expose the
references. A caller-level resolver supplies formula text and parsed
formula ASTs.

## Comment Entries
An entry named `comment` can contain raw prose until the closing `}`.
These bodies do not follow the section grammar. Preserve them as raw
text entries.

## Compressed Entries
A body that begins with `::` after optional comments/prose is compressed
or embedded data. Its payload is wrapped UF base64 text until `}`.

Compressed entries occur for large parameter sets and for embedded
formula/class entries saved with formulas. Examples have names like
`file.ufm:Entry`, `file.ucl:Entry`, and `file.ulb:class:Name(Base)`.

Decode compressed entries as follows:

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
8. Parse the inflated text as the body of the current entry. The
   inflated text does not include the outer entry name or braces.

Until decode support lands, classify these entries as opaque and emit a
diagnostic when a client asks for their typed contents.

## AST Shape

```text
ParameterFile
  entries: [ParameterEntry]

ParameterEntry
  name
  body: ParameterSet | CommentText | CompressedText | UnknownText
  source_range

ParameterSet
  fractal: Section
  layers: [Layer]

Layer
  layer: Section
  mapping: Section?
  transforms: [Section]
  formula: Section?
  inside: Section?
  outside: Section?
  gradient: Section?
  opacity: Section?
  alpha: Section?
  extras: [Section]

Section
  name
  assignments: [Assignment]

Assignment
  key
  value: RawValue
  source_range
```

Use ordered vectors everywhere. Build convenience lookup helpers on top
of the ordered representation.

## Semantic Resolution Plan
1. Parse the `.upr` file into the generic entry AST.
2. Classify plain entries that contain `fractal:` as `ParameterSet`.
3. Build typed layer objects while preserving all raw sections.
4. For each `formula`, `inside`, `outside`, and `transform` section,
   collect `(filename, entry, kind)` references.
5. Ask a caller-provided resolver for the referenced formula files.
   This follows the same rule as formula imports: the caller locates
   files; the parser only consumes supplied text.
6. Parse referenced entries with the extended formula parser and its
   import support, preserving imported filenames in source locations.
7. Validate `p_` and `f_` assignments against the referenced formula
   entry's parameter declarations, function parameters, plug-in defaults,
   imported classes, and parameter forwards.
8. Diagnose missing files, missing entries, unknown parameter names,
   missing required values, and type mismatches with `.upr` source
   locations.
9. Keep unknown assignments as raw data so older or newer UF files can
   still be inspected and round-tripped.

## Implementation Slices
1. Add a parameter-file lexer that produces entry braces, section labels,
   keys, raw atoms, quoted strings, comments, and compressed payloads.
2. Add a generic parser with recovery at `}` and the next top-level
   entry.
3. Add typed classification for parameter sets, comment entries, and
   compressed entries.
4. Add value conversion helpers for bools, numbers, complex pairs,
   strings, and repeated gradient stops.
5. Add reference extraction for formula/coloring/transform sections.
6. Add a resolver API and semantic checks against formula AST metadata.
7. Add compressed-entry decoding: UF base64, CRC32 validation, zlib
   inflate, then parse the inflated body text.

## Tests
- Unit parse plain one-layer and multi-layer parameter sets.
- Parse comment entries with unstructured prose.
- Parse compressed entries as opaque payloads.
- Decode compressed entries with known payloads.
- Reject compressed entries with bad line length, base64, CRC, or zlib.
- Parse `}NextEntry {` without losing either entry.
- Preserve repeated gradient, opacity, and alpha assignments in order.
- Parse quoted strings containing semicolons and continued strings.
- Parse `transform` sections and `numtransforms`/`transforms` counts.
- Validate `p_` and `f_` keys against fixture formula entries.
- Validate parameter forwards and plug-in sub-parameters.
- Corpus test every `*.upr` in `../uf-formulas` parses and classifies.
