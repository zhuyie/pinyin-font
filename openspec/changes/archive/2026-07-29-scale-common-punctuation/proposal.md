## Why

Pinyin synthesis scales Han glyphs to 65% to make room for pronunciation, but leaves punctuation at its original size. Mixed text therefore shows oversized commas, periods, and other punctuation beside the reduced Han text.

## What Changes

- Scale a curated whitelist of common half-width, full-width, and Chinese punctuation with the same ratio and placement logic used for Han glyph bodies.
- Preserve each punctuation glyph's original advance width while replacing its cmap mapping with a scaled composite glyph.
- Leave technical, mathematical, and currency symbols—including `/`, `\`, and `_`—unchanged.
- Preserve the source mapping when a whitelisted punctuation glyph is missing, has no outline, or cannot be safely wrapped.
- Keep generated composite metadata and cmap-integrity diagnostics correct for intentional punctuation replacements.

## Capabilities

### New Capabilities

- `punctuation-scaling`: Defines the punctuation whitelist, scaling, placement, width preservation, fallback behavior, and horizontal-layout scope.

### Modified Capabilities

- `generated-font-integrity`: Treats successful punctuation scaling as an intentional cmap replacement and requires nested punctuation composites to be covered by generated-font metadata.

## Impact

- Affects punctuation selection and composite-glyph generation in `src/synthesis/`.
- Affects synthesis and integrity tests for cmap mappings, horizontal metrics, transformed bounds, and composite depth.
- Does not change the CLI, pinyin database format, source fonts, or public builder API.
