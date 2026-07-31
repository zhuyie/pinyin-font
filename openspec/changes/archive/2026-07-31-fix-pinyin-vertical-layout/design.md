## Context

The builder scales Han bodies to 65% and pinyin components to 35%. It currently derives a shared body offset and pinyin baseline from `head.YMin` and `head.YMax`, which are global extrema across every outline in the font rather than typographic layout metrics.

For Noto Sans CJK SC, the global vertical bounds are approximately `-1048..1808`, while the complete local pinyin database maps to Han glyphs whose upper bounds have a median near 840 and a maximum near 860. The current formula therefore leaves roughly 700 design units between ordinary pinyin and its Han body. Noto Serif exhibits the same behavior. Their `OS/2` typo band is `-120..880`, which closely encloses normal CJK outlines and is shared by the other preview fonts.

The global lower bound does not contribute to the relative pinyin/Han gap because the body and pinyin share `baseDY`; it does move the entire composite down. The global upper bound directly controls the gap.

## Goals / Non-Goals

**Goals:**

- Base vertical composition on intended typographic metrics rather than unrelated outline extrema.
- Preserve one shared pinyin baseline across synthesized characters.
- Keep Han-body and punctuation placement consistent.
- Define deterministic fallback behavior for absent or invalid typo metrics.
- Make layout invariant to unrelated extreme glyphs in the source font.

**Non-Goals:**

- Changing horizontal pinyin letter spacing or width fitting.
- Changing the 65% Han / 35% pinyin scale ratios.
- Aligning pinyin separately to each Han glyph's actual top edge.
- Changing cmap generation, polyphonic selection, or component fallback.
- Rewriting source font line metrics.

## Decisions

### Select one font-wide typographic layout band

The builder will resolve one `layoutYMax/layoutYMin` pair before synthesis:

1. Use `OS/2.sTypoAscender` and `OS/2.sTypoDescender` when the ascender is positive, the descender is non-positive, and the ascender is greater than the descender.
2. Otherwise use `hhea.Ascender` and `hhea.Descender` under the same validity conditions.
3. Otherwise fall back to `head.YMax` and `head.YMin` for compatibility with fonts whose typographic metrics are unusable.

`sTypoLineGap` and `hhea.LineGap` do not participate because they describe inter-line spacing, not the internal composition of one glyph.

Alternative considered: use each target Han glyph's bounds. This tightly packs each glyph but makes adjacent pinyin baselines jump with differences in Han outlines.

Alternative considered: calculate percentiles over database Han glyphs. This avoids outliers but makes output depend on database coverage and requires arbitrary percentile choices.

Alternative considered: use `hhea` first. Noto Serif's hhea band is substantially taller than its common CJK outlines, while its typo band is the stable `-120..880` used across the tested fonts.

### Preserve the current geometric formula with the selected band

The scale ratios and placement relationship remain unchanged; only the source of the vertical band changes:

```text
baseDY = min(0, layoutYMin) * (1 - baseRatio)

pinyinDY =
    baseDY
    + layoutYMax * baseRatio
    - pinyinCharYMin * pinyinRatio
```

For the deepest supported pinyin descender, the relative gap above a Han glyph becomes:

```text
(layoutYMax - hanGlyph.YMax) * baseRatio
```

This retains a common pinyin baseline and uses the source font's intended typographic headroom. Ordinary pinyin without descenders naturally has additional clearance.

### Share the resolved body offset with punctuation

Scaled punctuation wrappers will continue using the same `baseDY` as synthesized Han bodies. The existing punctuation contract remains conceptually unchanged, but test expectations must reflect a typographic descender rather than the previous global outline minimum.

### Test invariance, not only known font constants

The primary synthetic regression will compare two equivalent fixture fonts where one contains an unrelated, unmapped glyph with extreme vertical bounds. Their generated pinyin and punctuation component Y offsets must match. This captures the defect independently of any specific Noto release.

Static Noto Sans and Serif generated with the complete local database provide integration validation. Existing preview fonts provide compatibility coverage.

## Risks / Trade-offs

- [Some legacy fonts have missing or nonsensical typo metrics] → Validate the pair and fall back to hhea, then head.
- [Changing `baseDY` moves all scaled Han bodies and punctuation] → Assert shared offsets and bounds in synthesis tests and compare existing preview fonts visually.
- [Pinyin marks may extend above the selected typo ascender] → Continue deriving actual composite bounds from transformed components; do not treat the typo band as a clipping boundary.
- [This does not fix excessive horizontal spacing in fonts with extreme global X bounds] → Keep horizontal spacing explicitly out of scope for a separate change.

## Migration Plan

No CLI or data migration is required. Fonts must be regenerated to receive corrected component placement. Reverting restores the old layout formula but also restores the Noto CJK spacing defect.

## Open Questions

- Whether horizontal spacing should later use `UnitsPerEm` or per-glyph advances instead of global `head.XMin/XMax`.
