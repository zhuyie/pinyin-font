## 1. Horizontal Composition Model

- [x] 1.1 Replace global-head-derived pinyin spacing with base-cluster cursor advancement from resolved glyph horizontal metrics
- [x] 1.2 Keep combining marks attached to their base advance cell without contributing a separate logical advance
- [x] 1.3 Calculate the translated ink union across all base-letter, precomposed, generated fallback, and tone-mark components
- [x] 1.4 Center the pinyin ink union on the source Han advance cell instead of the Han outline center

## 2. Width Fitting and Composite Encoding

- [x] 2.1 Resolve a per-reading X scale capped at the configured pinyin ratio and conservatively fit overwide ink to the Han advance
- [x] 2.2 Extend component transforms and bound calculations to support independent quantized X and Y scales
- [x] 2.3 Preserve uniform transforms for Han bodies and scaled punctuation and preserve the shared pinyin Y scale and baseline

## 3. Synthetic Regression Coverage

- [x] 3.1 Extend fixtures with asymmetric Han outlines, distinct Latin side bearings and advances, wide tone marks, and an unrelated extreme-X glyph
- [x] 3.2 Verify short readings retain the configured uniform scale and long readings use a common reduced X scale whose pinyin ink stays inside both advance edges
- [x] 3.3 Verify advance-cell centering is stable across asymmetric Han outlines and unrelated global X extrema
- [x] 3.4 Verify source, precomposed, generated dotless-i, and stacked-mark paths preserve their internal horizontal relationships after fitting
- [x] 3.5 Verify Han-body and punctuation horizontal transforms and metrics remain unchanged

## 4. Diagnostics and Real-Font Validation

- [x] 4.1 Update generated-font integrity diagnostics to report left-of-zero and right-of-advance overflow separately
- [x] 4.2 Regenerate Noto Sans CJK SC, Noto Serif CJK SC, and ZCOOL XiaoWei with the local complete test database and inspect `zhuang`, `shuang`, `chuang`, `qiang`, short readings, and punctuation
- [x] 4.3 Confirm the three fonts load in the browser preview without pinyin overlap and retain the corrected vertical placement
- [x] 4.4 Run the complete test suite, OpenSpec validation, and generated-font integrity diagnostics
