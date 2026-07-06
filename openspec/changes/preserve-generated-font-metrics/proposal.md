## Why

Generated pinyin fonts should behave like the source font with pinyin glyphs overlaid on supported Han characters, but the current synthesis path rebuilds cmap from a narrow retained set and leaves several OpenType metric fields inconsistent with newly added composite glyphs. This can drop source-font mappings on synthesis failure and produce generated fonts whose metadata no longer describes their glyph data accurately.

## What Changes

- Preserve the source font cmap as the baseline mapping for generated fonts, then override only successfully synthesized Han character mappings with new pinyin composite glyphs.
- Ensure characters that cannot be synthesized keep their original glyph mappings instead of disappearing from cmap.
- Update generated-font OpenType metadata that is directly affected by appended composite glyphs, including `maxp` composite component fields and other safe glyph-count/metric fields needed for table consistency.
- Add a developer-facing metrics integrity check that reports cmap preservation, glyph bbox overflow, advance overflow, and metric/table inconsistencies for generated fonts.
- Keep normal `pinyinfont` runtime output focused; integrity diagnostics belong in development tooling or tests, not as warning spam during ordinary font generation.

## Capabilities

### New Capabilities
- `generated-font-integrity`: Defines cmap preservation and metric consistency requirements for fonts produced by the pinyin synthesis workflow.

### Modified Capabilities
- `project-layout`: Development font diagnostics should include generated-font integrity checks in the unified `font_tool` utility.

## Impact

- Affects `src/synthesis/` cmap construction and glyph synthesis bookkeeping.
- Affects `src/opentype/` font metadata maintenance and writer-visible table values.
- Affects `tools/font_tool.cpp` by adding or extending development-only diagnostics for generated font integrity.
- May add focused tests or fixtures to verify cmap fallback and composite metrics metadata without expanding product CLI modes.
