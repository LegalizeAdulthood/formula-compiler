# Parser Gaps

## Section Handling

Switch sections now parse multiple settings, including `type`,
`destination = #pixel`, and `destination = source_parameter`.

Default parameter blocks now parse multiple inner settings, including
`caption`, `default`, `hint`, `min`, `max`, `enum`, `enabled`, `visible`,
`expanded`, `exponential`, and `selectable`.

Default function blocks now parse metadata settings, including `caption`,
`default`, `hint`, `enabled`, and `visible`.

Default heading blocks now parse metadata settings, including `caption`,
`text`, `hint`, `enabled`, `visible`, `expanded`, and corpus legacy
`show`/`extended` booleans.

Default settings now apply per-entry-kind rules. Fractal-only settings
are rejected for coloring, transformation, and class entries; class
defaults accept only class-valid metadata.

Default and switch sections now stop on the full section-token set so
misplaced extended sections report section errors.

Entry headers now capture coloring `INSIDE`/`OUTSIDE` flags. Other
parenthesized entry options remain accepted as raw header text without
setting coloring flags.
