## 1. Preserve Cmap Coverage

- [x] 1.1 Add an internal way to copy the parsed source cmap mappings into the synthesis output map before adding generated glyphs.
- [x] 1.2 Change pinyin glyph synthesis so successful glyphs override only their target character mapping.
- [x] 1.3 Ensure synthesis failures keep the original character mapping instead of removing the character from cmap.
- [x] 1.4 Verify generated cmap preservation with a focused test or sample-font check.

## 2. Maintain Generated Font Metrics

- [x] 2.1 Update `maxp.MaxComponentElements` for appended composite glyphs.
- [x] 2.2 Update `maxp.MaxComponentDepth` to cover generated composite glyphs.
- [x] 2.3 Review `head` bbox and `hhea` horizontal derived metrics against generated glyph data, and update deterministic consistency fields where appropriate without silently changing line-height policy.
- [x] 2.4 Verify generated fonts no longer contain composite glyph data that exceeds the serialized `maxp` component maxima.

## 3. Add Development Integrity Diagnostics

- [x] 3.1 Add a `font_tool` command for generated-font integrity diagnostics.
- [x] 3.2 Report dropped or unexpected cmap changes by comparing original and generated fonts.
- [x] 3.3 Report glyph bbox overflow against advance widths and vertical metric bounds.
- [x] 3.4 Report `maxp` composite maxima inconsistencies.

## 4. Validate

- [x] 4.1 Build the project from the existing CMake configuration.
- [x] 4.2 Run existing CTest coverage.
- [x] 4.3 Generate sample pinyin fonts and run the integrity diagnostic against them.
- [x] 4.4 Run OpenSpec validation for this change.
