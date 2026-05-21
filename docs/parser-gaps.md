# Parser Gaps

## Section Handling

- `default:` headings are not parsed yet. UF supports
  `heading`/`endheading` blocks with settings such as `caption`, `text`,
  `visible`, `enabled`, and `expanded`.
- Default settings are accepted from one shared table. The parser should
  apply per-entry-kind setting rules so fractal-only settings are rejected
  for coloring, transformation, and class entries.
- `default:` and `switch:` section stop lists are fractal-shaped. They
  should use the full section-token set so misplaced later sections report
  section errors instead of setting parse errors.
- Coloring entry flags such as `(INSIDE)` and `(OUTSIDE)` are not captured.
  Transformation entry options in parentheses should be accepted or ignored
  consistently when entry headers are parsed.

Switch sections now parse multiple settings, including `type`,
`destination = #pixel`, and `destination = source_parameter`.

Default parameter blocks now parse multiple inner settings, including
`caption`, `default`, `hint`, `min`, `max`, `enum`, `enabled`, `visible`,
`expanded`, `exponential`, and `selectable`.

Default function blocks now parse metadata settings, including `caption`,
`default`, `hint`, `enabled`, and `visible`.
