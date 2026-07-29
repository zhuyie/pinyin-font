## Why

Pinyin synthesis currently fails for thousands of otherwise-supported Han characters when the source font lacks tone marks or precomposed accented `i` glyphs. The generator should preserve the source font's Latin style while supplying the minimal missing TrueType components needed to compose complete pinyin coverage.

## What Changes

- Generate an internal set of TrueType tone-mark glyphs for macron, acute, caron, grave, and diaeresis when the source font does not provide usable equivalents.
- Size generated tone marks from the source font's measured lowercase metrics and position them relative to the actual vowel glyph bounds.
- Derive an internal dotless-`i` glyph from a reliably separable source `i` outline, then combine it with generated tone marks when `ī`, `í`, `ǐ`, or `ì` is unavailable.
- Normalize exceptional accented syllabic forms such as `ḿ` and `ń` into base letters plus tone components.
- Preserve source glyph preference, source-cmap fallback semantics, and generated-font metric consistency.
- Report component fallback usage and distinguish unsupported source glyphs from component-generation failures in development diagnostics.
- Keep CFF and CFF2 fonts explicitly out of scope; this change applies only to OpenType fonts with TrueType `glyf`/`loca` outlines.

## Capabilities

### New Capabilities

- `pinyin-component-fallback`: Defines source-aware generation, selection, sizing, and placement of internal tone marks and a derived dotless `i` for TrueType pinyin synthesis.

### Modified Capabilities

- `generated-font-integrity`: Extends generated-font integrity and diagnostics to cover appended internal simple glyphs, component fallback outcomes, and continued source-cmap preservation when fallback synthesis fails.

## Impact

- Affects `src/synthesis/` composition, substitution, fallback, glyph positioning, and synthesis statistics.
- Affects `src/opentype/` simple-glyph insertion and any outline utilities needed to copy selected contours safely without retaining invalid hinting instructions.
- Affects `src/pinyin/` normalization for exceptional accented forms.
- Affects `tools/font_tool.cpp` development diagnostics and `tools/preview.html` validation samples.
- Adds focused automated tests and representative TrueType font fixtures or fixture-generation support.
- Does not add CFF/CFF2 parsing or writing, change the CLI-first product shape, or add external runtime font dependencies.
