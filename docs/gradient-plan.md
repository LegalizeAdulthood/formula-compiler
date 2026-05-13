# 🎯 Parse UltraFractal Gradient Files (.ugr)
**Overview**: Parse UltraFractal Gradient Files (.ugr)

**Progress**: 12% [█░░░░░░░░░]

**Last Updated**: 2026-05-12 14:19:33

## Format Summary (from research)
- File = zero or more named blocks: `BlockName { section section }` (both sections required, either order)
- Two section types: `gradient:` (BGR color control points) and `opacity:` (opacity control points)
- Tokens are whitespace-separated `key=value` pairs; line breaks are insignificant
- Comments: `;` to end-of-line
- Section-level keys: `title` (quoted string), `smooth` (yes/no), `rotation` (signed int), `linked` (yes/no)
- Control points: flat interleaved stream of `index=N color=M` or `index=N opacity=M`
- Index range: nominally 0-399; negative values wrap around; duplicates are valid (hard edge)
- Color encoding: packed BGR integer -- `R=(c>>16)&0xFF  G=(c>>8)&0xFF  B=c&0xFF`
- Opacity: 0-255; independent of color curve unless `linked=yes`

## 📝 Plan Steps
- ✅ **Research UltraFractal .ugr file format specification**
  - Grammar documented in `docs/gradient-grammar.txt`
  - Semantics correlated against UF6 manual and real `.ugr` sample file
-  **Define data model types**
  - `GradientFile`: list of `GradientEntry`
  - `GradientEntry`: `name` (string), `gradient_section`, `opacity_section`
  - `GradientSection`: `title`, `smooth`, `rotation`, `linked`, list of `ColorControlPoint`
  - `OpacitySection`: `smooth`, `rotation`, `linked`, list of `OpacityControlPoint`
  - `ColorControlPoint`: `index` (int, may be negative), `color` (uint32 BGR packed)
  - `OpacityControlPoint`: `index` (int, may be negative), `opacity` (0-255)
  - `RgbColor`: `r`, `g`, `b` (byte) -- result of BGR unpacking
-  **Implement tokenizer**
  - Reads a character stream; emits `IDENTIFIER`, `INTEGER`, `QUOTED_STRING`, `EQUALS`, `LEFT_BRACE`, `RIGHT_BRACE`, `COLON`, `END_OF_FILE`
  - Skips whitespace (including newlines) and `;` comments between tokens
  - Handles negative integers (leading `-`)
-  **Implement section parser**
  - Parses `gradient:` and `opacity:` sections from the flat token stream; accepts either order
  - Collects section-level keys (`title`, `smooth`, `rotation`, `linked`) until first `index=` token
  - Collects control points as interleaved `index=N` / `color=M` (or `opacity=M`) pairs
  - Preserves duplicate index entries (hard-edge semantics)
  - Handles both `gradient:` and `opacity:` section types
-  **Implement top-level file parser**
  - Parses zero or more blocks from the token stream
  - Each block: reads `block_name`, `{`, zero or more sections, `}`
  - Skips comments between sections
  - Returns a `GradientFile` containing all `GradientEntry` instances
-  **Convert packed BGR color integers to RgbColor**
  - `R = (color >> 16) & 0xFF`
  - `G = (color >>  8) & 0xFF`
  - `B = (color >>  0) & 0xFF`
  - Provide a helper/extension on `ColorControlPoint` or `GradientSection`
-  **Write unit tests**
  - Tokenizer: identifiers, integers (positive and negative), quoted strings, comments, braces, colons
  - Section parser: section-level keys, control point pairs, duplicate indices, negative indices
  - File parser: multi-block file, blocks with gradient: first, blocks with opacity: first
  - BGR conversion: known values (e.g. `8546815` => R=0x82 G=0x6A B=0xFF)
-  **Validate parser against real sample .ugr files**
  - Parse `Rainbow gradients by Velvet--Glove.ugr` (20 gradients, varied complexity)
  - Verify block count, spot-check control point counts and color values against raw file