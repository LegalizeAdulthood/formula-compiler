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
- Parser validation of documented predefined-symbol names. Unknown `#` names
  are parse errors.

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

## Remaining Parser Work

No remaining extended formula parser implementation slices are planned in this
file.

## Known Runtime Divergence

`docs/uf6/conditionals.txt` explicitly documents short-circuit evaluation for
`&&` and `||`: expressions are evaluated left-to-right and evaluation stops
once the result is known. Current GLSL translator work emits helper calls for
logical operators, which forces both operands to be evaluated. That divergence
must be corrected in translator/runtime work, not parser work.

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
