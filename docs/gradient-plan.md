# 🎯 Parse UltraFractal Gradient Files (.ugr)
**Overview**: Parse UltraFractal Gradient Files (.ugr)

**Progress**: 25% [██░░░░░░░░]

**Last Updated**: 2026-05-12 14:19:33

## Format Summary (from research)
- File = zero or more named blocks: `BlockName { section section }` (both sections required, either order)
- Two section types: `gradient:` (BGR color control points) and `opacity:` (opacity control points)
- Tokens are whitespace-separated `key=value` pairs; line breaks are insignificant
- Comments: `;` to end-of-line
- Section-level keys: `title` (quoted string), `smooth` (yes/no), `rotation` (signed int), `linked` (yes/no)
- Control points: flat interleaved stream of `index=N color=M` or `index=N opacity=M`
- Index range: nominally 0-399; negative values wrap around; duplicates are valid (hard edge)
- Color encoding: packed BGR integer in file -- unpacked to `RGBColor` during parsing: `R=(c>>16)&0xFF  G=(c>>8)&0xFF  B=c&0xFF`
- Opacity: 0-255; independent of color curve unless `linked=yes`

## 📝 Plan Steps
- ✅ **Research UltraFractal .ugr file format specification**
  - Grammar documented in `docs/gradient-grammar.txt`
  - Semantics correlated against UF6 manual and real `.ugr` sample file
- ✅ **Define data model types** -- declared in `libs/include/formula/Gradient.h`, namespace `gradient`
  - `RGBColor`: `red`, `green`, `blue` (uint8_t) -- result of BGR unpacking
  - `ColorControlPoint`: `index` (int, may be negative), `color` (RGBColor)
  - `OpacityControlPoint`: `index` (int, may be negative), `opacity` (uint8_t, 0-255)
  - `GradientSection`: `title`, `rotation`, `smooth`, `linked`, `points` (vector of ColorControlPoint)
  - `OpacitySection`: `smooth`, `rotation`, `linked`, `points` (vector of OpacityControlPoint)
  - `GradientEntry`: `name` (string), `gradient` (GradientSection), `opacity` (OpacitySection)
  - `GradientFile`: `entries` (vector of GradientEntry)
  - Entry point: `GradientFile parse_gradient(std::string_view text)`
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
  - Each entry: reads `entry_name`, `{`, two sections (either order), `}`
  - Returns a `GradientFile` containing all `GradientEntry` instances
-  **Convert packed BGR color integers to RGBColor** during parsing
  - `red   = (packed >> 16) & 0xFF`
  - `green = (packed >>  8) & 0xFF`
  - `blue  = (packed >>  0) & 0xFF`
  - Applied in the section parser when reading `color=N` tokens; stored directly as `RGBColor` in `ColorControlPoint`
-  **Write unit tests**
  - Tokenizer: identifiers, integers (positive and negative), quoted strings, comments, braces, colons
  - Section parser: section-level keys, control point pairs, duplicate indices, negative indices
  - File parser: multi-block file, blocks with gradient: first, blocks with opacity: first
  - BGR conversion: `8546815` => `red=0x82 green=0x6A blue=0xFF`
-  **Validate parser against real sample .ugr files**
  - Parse `Rainbow gradients by Velvet--Glove.ugr` (20 gradients, varied complexity)
  - Verify block count, spot-check control point counts and color values against raw file