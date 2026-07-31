## 1. Vertical Metric Selection

- [x] 1.1 Add a focused helper that validates and selects the OS/2 typo, hhea, or head vertical band in priority order
- [x] 1.2 Calculate the shared Han-body offset and pinyin baseline from the selected band while preserving the existing scale ratios
- [x] 1.3 Keep scaled punctuation wrappers on the same resolved Han-body vertical offset

## 2. Synthetic Regression Coverage

- [x] 2.1 Extend test fixtures to create an unrelated unmapped glyph with extreme vertical bounds without changing typographic metrics
- [x] 2.2 Verify equivalent fonts with and without the outlier produce identical Han, pinyin, and punctuation component Y offsets
- [x] 2.3 Add coverage for OS/2 typo metric priority and deterministic hhea/head fallback behavior
- [x] 2.4 Verify adjacent Han glyphs retain a shared pinyin baseline and stacked marks preserve their internal spacing
- [x] 2.5 Update existing punctuation placement expectations to use the resolved typographic body offset

## 3. Real-Font Validation

- [x] 3.1 Generate static Noto Sans CJK SC and Noto Serif CJK SC with the complete local pinyin database and inspect representative composite offsets
- [x] 3.2 Confirm Noto pinyin-to-Han spacing no longer reflects global head extrema and both fonts still load in the browser preview
- [x] 3.3 Check ZCOOL XiaoWei and the existing synthesis fixtures for compatible vertical placement
- [x] 3.4 Run the complete test suite and generated-font integrity diagnostics
