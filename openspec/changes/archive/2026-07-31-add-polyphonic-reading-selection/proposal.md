## Why

The generator currently annotates every Han character with only the first reading in the pinyin database, so users cannot correct polyphonic characters whose intended pronunciation is another candidate. A compact, explicit text convention is needed that works inside the generated font without requiring an application-specific text processor.

## What Changes

- Support `汉字@序号` as an explicit reading selector, where the one-based sequence number refers to that character's ordered readings in the pinyin database.
- Generate internal annotated glyphs for selectable non-default readings of polyphonic characters.
- Generate an OpenType GSUB standard-ligature feature that replaces a valid `汉字@序号` glyph sequence with the selected annotated Han glyph, consuming the visible selector.
- Preserve ordinary text rendering when a selector is invalid, unsupported, or shaping does not apply the feature.
- Add diagnostics and tests for the generated GSUB structure, valid selection, invalid selection, default readings, and source-font integrity.

## Capabilities

### New Capabilities

- `polyphonic-reading-selection`: Defines the `汉字@序号` syntax, candidate ordering, GSUB shaping behavior, fallback behavior, and generated-font requirements for explicit polyphonic reading selection.

### Modified Capabilities

- None.

## Impact

- Pinyin synthesis must process all stored readings instead of only `pinyin[0]` for polyphonic records.
- The OpenType model and writer must support unmapped composite glyphs and a generated GSUB table containing the required script, feature, lookup, coverage, and ligature data.
- Font diagnostics and synthesis tests must inspect and exercise OpenType substitution behavior.
- Generated fonts become dependent on an OpenType shaping engine for selector processing; environments that only perform `cmap` lookup continue to show the literal selector.
- No CLI option or pinyin database format change is required.
