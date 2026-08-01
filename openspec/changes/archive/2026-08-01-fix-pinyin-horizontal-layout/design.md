## Context

The composer scales pinyin uniformly to 35% and positions each base letter by its outline width plus `10%` of the source font's global `head` X span. That span is the union of every source outline, not a spacing metric: it is about 3930 units in the tested Noto CJK fonts and about 1042 units in ZCOOL XiaoWei. Consequently Noto receives 393 units of unscaled tracking per letter.

The completed annotation is centered on the Han outline bounds and is not fitted to the Han advance. HarfBuzz reports the generated Noto Sans glyph for `chuang` at approximately `-307..1307` in a 1000-unit advance and Noto Serif at `-395..1409`. Replacing the global span with `UnitsPerEm` alone does not solve the problem: the natural Latin widths of long readings can still exceed one Han cell at the current vertical scale.

The vertical layout contract is already stable: pinyin uses a shared font-wide baseline and a 35% Y scale. Horizontal fitting must not modify those decisions.

## Goals / Non-Goals

**Goals:**

- Use source Latin horizontal metrics as the intended spacing between pinyin base letters.
- Keep every pinyin annotation's ink within its Han advance cell.
- Preserve the 35% vertical scale and shared pinyin baseline.
- Center annotations consistently in the advance cell, independent of Han outline asymmetry.
- Include source, generated fallback, precomposed, and combining-mark components in the same fitting calculation.
- Make horizontal layout independent of unrelated global source-font outline extrema.

**Non-Goals:**

- Changing Han-body or punctuation scaling and placement.
- Changing generated glyph advance widths or source horizontal metrics.
- Applying OpenType kerning or shaping features inside synthesized pinyin.
- Selecting a new global visual tracking value.
- Changing vertical layout, cmap behavior, polyphonic selection, or component fallback policy.

## Decisions

### Lay out base-letter clusters with source advances

Each resolved base letter or precomposed pinyin glyph starts at the current logical cursor. The cursor advances by that component's `hmtx.AdvanceWidth`; combining tone marks do not advance it. Marks are centered over the base letter's advance cell and retain their existing vertical stacking relationship.

This uses the source typeface's side bearings rather than reconstructing spacing from outline widths. No extra tracking is added. The current `pinyinCharSpace_` value and its dependency on global `head.XMin/XMax` are removed.

Alternative considered: derive tracking from `UnitsPerEm`. This removes the global-extrema dependency but introduces a new arbitrary spacing constant and still leaves long Serif readings wider than a Han cell.

Alternative considered: keep outline-edge spacing and only change its constant. Outline widths discard the source typeface's intended side bearings and produce inconsistent spacing across Latin designs.

### Fit the complete pinyin ink union with a per-reading X scale

After composing all base letters and marks in unscaled logical coordinates, calculate the union of every component's translated X bounds. Let this be `inkMinX..inkMaxX`.

The pinyin Y scale remains `pinyinRatio` (`0.35`). Its X scale is:

```text
pinyinScaleX = min(
    pinyinRatio,
    baseAdvanceWidth / (inkMaxX - inkMinX))
```

The encoded F2DOT14 scale must be rounded conservatively so transformed ink does not exceed the advance because of quantization. Short readings retain the existing 35% aspect ratio; only annotations that would overflow are horizontally condensed.

All pinyin components in one reading use the same X scale, including marks, so horizontal relationships remain intact. Their Y transforms and Y offsets continue using `pinyinRatio`.

Alternative considered: reduce the global pinyin ratio until every reading fits. This unnecessarily shrinks common short readings and also changes their vertical size.

Alternative considered: reduce or make tracking negative before scaling. Some Serif readings exceed the cell even with zero added tracking, and negative tracking risks letter collisions.

### Center the fitted ink on the Han advance cell

Use `baseAdvanceWidth / 2` as the target center. Translate every pinyin component so the scaled center of `inkMinX..inkMaxX` lands on that point. The Han outline bounds do not participate.

This matches the cell in which the scaled Han body is already centered and gives adjacent characters a stable annotation center even when their source outlines have asymmetric side bearings.

### Support non-uniform composite transforms without changing body consumers

The component-append path will accept independent X and Y scales. When they differ, it emits `WE_HAVE_AN_X_AND_Y_SCALE`; when they match, it may retain the existing uniform-scale encoding. Bounds calculation must use the same quantized transform values that are serialized.

Han-body and punctuation calls continue passing the same value for both axes, so their geometry remains unchanged.

### Audit both sides of the advance cell

The generated-font integrity report will distinguish `XMin < 0` from `XMax > AdvanceWidth` instead of reporting only right-side overflow. Synthesis tests will additionally inspect the pinyin component union separately from the Han body, so a legitimate source outline overhang cannot hide or misclassify annotation fitting.

## Risks / Trade-offs

- [Long readings become horizontally condensed, especially in wide Serif Latin designs] → Condense only when required and retain the full 35% X scale for readings that already fit.
- [F2DOT14 quantization or integer offset rounding can leave a one-unit overflow] → Calculate bounds from encoded transforms and use a conservative scale cap.
- [A combining mark can be wider than its base letter] → Include every mark in the fitted ink union rather than fitting only base letters.
- [Source Latin advances can be zero or unusable] → Treat a missing or zero component advance as a synthesis failure, preserving the original Han mapping under the existing failure contract.
- [Whole composite diagnostics may include legitimate source-Han overhang] → Add pinyin-specific assertions and report left/right counts rather than treating every whole-glyph overhang as corruption.

## Migration Plan

No CLI or data migration is required. Regenerate fonts to receive corrected horizontal placement. Reverting restores the old global-extrema tracking and overflow behavior.

## Open Questions

- Whether a later visual-only change should reserve optional inner padding inside the advance cell; this correctness change uses the full advance to minimize condensation.
