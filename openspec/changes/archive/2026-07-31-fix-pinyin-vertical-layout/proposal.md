## Why

The pinyin layout algorithm uses the global `head.YMin` and `head.YMax` outline extrema as if they described a normal Han glyph. Fonts such as Noto CJK contain unrelated extreme outlines in those global bounds, causing synthesized Han bodies to shift far below the baseline and their pinyin to sit hundreds of units too high.

## What Changes

- Derive the shared vertical layout band from valid typographic metrics instead of global outline extrema.
- Prefer `OS/2` typo ascender and descender values, with deterministic fallbacks for fonts whose typo metrics are absent or invalid.
- Keep pinyin baselines consistent across characters while ensuring unrelated extreme glyphs do not alter synthesized layout.
- Apply the selected Han-body vertical offset consistently to scaled punctuation wrappers.
- Add synthetic outlier regression coverage and real-font validation for Noto CJK and existing preview fonts.

## Capabilities

### New Capabilities

- `pinyin-glyph-layout`: Defines vertical metric selection, pinyin/Han placement, fallback behavior, and layout invariance to unrelated outline extrema.

### Modified Capabilities

- None.

## Impact

- Affects vertical metric selection and component offsets in `src/synthesis/pinyin_font_builder.cpp`.
- Updates synthesis and punctuation tests whose expected vertical offsets currently reflect `head` bounds.
- Changes generated glyph placement but does not change cmap behavior, glyph selection, the CLI, or the pinyin database format.
