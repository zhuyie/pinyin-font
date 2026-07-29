## Context

The builder currently retains the source cmap, then replaces supported Han mappings with newly appended composite glyphs. Each Han body is transformed by `baseRatio_` (0.65), horizontally offset within its original advance width, and vertically offset by `baseDY_`. Punctuation never enters this path and therefore remains at full size.

The font model can append a composite glyph and rebuild cmap mappings, so punctuation can reuse the same non-destructive mechanism. The source glyph remains available as the component, while only selected character mappings point to the scaled wrapper.

## Goals / Non-Goals

**Goals:**

- Make common Chinese, full-width, and half-width punctuation visually consistent with the reduced Han bodies.
- Reuse the Han body's scale, horizontal offset, and vertical offset calculations.
- Preserve source advance widths and source mappings on unsupported or failed cases.
- Keep cmap and nested composite metadata internally consistent.
- Keep punctuation generation separate from pinyin synthesis success/failure statistics.

**Non-Goals:**

- Scaling whitespace, technical symbols, mathematical symbols, currency symbols, `/`, `\`, or `_`.
- Preserving or adding vertical-layout substitutions.
- Adding or preserving kerning, GSUB, or GPOS tables; the current writer does not emit these tables.
- Adding a public configuration API for the punctuation ratio or whitelist.

## Decisions

### Use an explicit Unicode whitelist

The builder will recognize exact code points rather than broad Unicode blocks. The initial whitelist covers:

- Half-width: `! " ' ( ) , - . : ; ? [ ] { }`
- Full-width: `！ ＂ ＇ （ ） ， － ． ： ； ？ ［ ］ ｛ ｝`
- Chinese and typographic: `、 。 〈 〉 《 》 「 」 『 』 【 】 〔 〕 “ ” ‘ ’ – — … · ・`

An explicit list prevents unrelated symbols from being resized merely because they share a Unicode block. The list will be centralized in synthesis code so selection behavior is independently testable and easy to extend.

### Append scaled wrappers instead of mutating source glyphs

For each mapped whitelist character with an outline, synthesis will append a one-component composite that references the source glyph. Its transform and offsets will be calculated exactly like the Han body:

```text
scale = baseRatio_
dx    = advanceWidth * (1 - baseRatio_) / 2
dy    = baseDY_
```

The wrapper will copy the source horizontal metric, update its LSB to the transformed bounds, and retain the original advance width. The cmap entry will be replaced only after the wrapper is appended successfully.

Mutating source outlines was rejected because one source glyph can serve multiple mappings and components, and in-place edits would change unrelated uses. Scaling advance widths was rejected because it would alter line breaking and full-width layout rhythm.

### Process punctuation after retaining the source cmap

Punctuation wrappers will be generated after the complete source cmap has been copied into the builder's replacement map and before pinyin Han glyphs are added. Missing mappings, glyphs without outlines, or failed wrapper creation will leave the copied source mapping untouched.

Characters that share the same source glyph may share one generated wrapper, avoiding redundant glyphs while preserving equivalent metrics and transforms.

### Keep punctuation outcomes out of pinyin statistics

Existing glyph-add and synthesis-failure counters describe pinyin database records. Punctuation wrapping will not increment those counters. Correctness will instead be covered by cmap, glyph transform, metric, and integrity tests.

### Account for nested composite depth

A punctuation source glyph may itself be composite, making its scaled wrapper one level deeper. Appending generated composites must update `maxp.MaxComponentDepth` to the actual reachable depth, with cycle-safe traversal for malformed input. This metadata rule applies equally to existing generated Han composites.

## Risks / Trade-offs

- **[Whitelist omits a punctuation variant]** → Keep the list explicit and cover every agreed code point in table-driven tests; extend it deliberately when a real use case appears.
- **[A source punctuation glyph has no outline]** → Preserve its original cmap mapping rather than emitting an empty wrapper.
- **[A malformed composite graph contains a cycle]** → Traverse with a visiting set and fail conservatively without replacing the punctuation mapping.
- **[A font is at the TrueType glyph-count limit]** → Preserve the original punctuation mapping; existing pinyin synthesis behavior remains authoritative for later additions.
- **[Scaled punctuation has more surrounding whitespace]** → This is intentional because advance widths remain stable; it preserves Chinese grid rhythm and line wrapping.
- **[Vertical text uses horizontal punctuation wrappers]** → Vertical layout is explicitly outside this change and requires separate feature-table support.

