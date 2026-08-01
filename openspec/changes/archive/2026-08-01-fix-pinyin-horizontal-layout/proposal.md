## Why

The pinyin composer derives letter spacing from global `head.XMin/XMax` outline extrema and never constrains the completed annotation to its Han glyph's advance cell. Noto CJK therefore produces long readings such as `zhuang`, `shuang`, and `chuang` that extend hundreds of units into adjacent characters, while fonts with less extreme global bounds happen to look closer to normal.

## What Changes

- Lay out pinyin base letters with their source horizontal metrics instead of global font outline extrema.
- Include base letters and tone-mark components when calculating the annotation's actual horizontal ink bounds.
- Preserve the existing vertical scale while dynamically reducing only the horizontal scale when an annotation would exceed its available Han advance width.
- Center pinyin on the Han advance cell rather than the individual Han outline bounds.
- Extend generated-font diagnostics and synthesis regressions to detect both left and right advance-cell overflow.
- Validate representative short and long readings with Noto Sans CJK SC, Noto Serif CJK SC, and ZCOOL XiaoWei.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `pinyin-glyph-layout`: Define horizontal component spacing, ink-bound fitting, advance-cell centering, and invariance to unrelated global X extrema.
- `generated-font-integrity`: Report generated glyph overflow on both sides of the horizontal advance cell.

## Impact

- Affects horizontal composition and component transforms in `src/synthesis/pinyin_font_builder.cpp` and its header.
- Uses non-uniform composite transforms for pinyin components whose horizontal scale must be reduced; Han-body and punctuation transforms remain unchanged.
- Updates synthesis fixtures and generated-font integrity diagnostics.
- Changes generated glyph placement but does not change pinyin database selection, vertical baselines, cmap behavior, source advances, or CLI options.
