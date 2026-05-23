# Extended Formula Parser Plan

## Status

The extended formula parser and load-time reference coverage from the earlier
plan has been implemented. This document now records only the remaining
parser-facing policy so completed implementation steps do not linger as open
work.

Implemented coverage includes:

- Extended lexer tokens and expression precedence.
- Multi-argument calls.
- Typed declarations and array declarations.
- `if`, `while`, `repeat`, `return`, and nested statement blocks.
- Function declarations, arguments, `const`, and by-reference metadata.
- Formula kinds and section ordering for fractal, coloring, transformation,
  and class entries.
- `final`, `transform`, switch, perturb, and visibility sections.
- Parameter and predefined-symbol reference AST nodes.
- Bool, string, color, and typed literal forms.
- Member access, indexing, `new`, casts, class declarations, inheritance
  syntax, fields, methods, constructors, static methods, and plug-in parameter
  syntax.
- Import metadata, source filenames, import graph loading, class indexing,
  reference collection, reference resolution, and lazy retention of referenced
  imported class ASTs.

## Parser Boundary

The parser owns syntax and structural entry rules. It should continue to report:

- Invalid tokens or malformed expressions.
- Invalid statement forms.
- Invalid section order or section cardinality.
- Invalid entry-kind section use.
- Invalid parser-only setting syntax.
- Impossible AST shapes, such as assignment to a literal.

The parser should not validate semantic meaning that requires types, imported
class descriptors, callable descriptors, or section-specific symbol rules.
Those checks belong in `semantic-analyzer.md`.

## Predefined Symbols

The lexer may still tokenize `#` followed by identifier syntax, but the parser
must reject any predefined symbol that is not explicitly documented in
`docs/uf6/predefined-symbols.txt` and the files under `docs/uf6/predefined`.
Referencing `#foo` when `foo` is not in that documented list is a parse error,
not a semantic diagnostic.

The documented predefined-symbol set is:

- `#angle`
- `#calculationPurpose`
- `#center`
- `#color`
- `#dpixel`
- `#dz`
- `#e`
- `#height`
- `#index`
- `#magn`
- `#maxiter`
- `#numiter`
- `#pi`
- `#pixel`
- `#random`
- `#randomrange`
- `#screenmax`
- `#screenpixel`
- `#skew`
- `#stretch`
- `#whitesq`
- `#width`
- `#x`
- `#y`
- `#z`

The AST must preserve source spelling for accepted predefined symbols. Lookup
should be case-sensitive unless a specific UF document says otherwise for
predefined symbols.

Validation of predefined-symbol mutability, type, constant-expression
availability, and formula-kind or section availability belongs to semantic
analysis.

## Remaining Parser Work

1. Add a parser-visible predefined-symbol name table from the documented set
   above. Reject unknown `#` names during parse while preserving source spelling
   for accepted names.
2. Add a semantic predefined-symbol descriptor table using the same documented
   set. Store canonical name, type, mutability, constant-expression
   availability, allowed formula kinds, and allowed sections.
3. Validate assignments to predefined symbols. Reject writes to read-only
   symbols and allow writes only to documented writable symbols in allowed
   formula kinds and sections.
4. Validate use in constant-expression contexts, including array declarations
   and default settings, using the documented constant behavior.
5. Add tests for every documented predefined symbol and at least one unknown
   symbol parse error. Tests should cover source spelling preservation and any
   documented writable symbols.

Future parser work should be added here only when it is truly syntactic. If the
work needs symbol tables, type information, imported class metadata, formula
kind compatibility, or runtime/compiler support, record it in the semantic,
interpreter, or compiler plan instead.

## Verification

Keep coverage in the existing parser, loader, and reference-resolution tests.
The required project workflow remains:

```text
cmake --workflow rt-default
```
