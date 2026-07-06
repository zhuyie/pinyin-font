## Context

The pinyin synthesis pipeline appends composite glyphs to the parsed source font, then rebuilds cmap from an internal `char2index_` map before writing the output font. Today that map is seeded only with selected common ranges, so source-font mappings outside those ranges are discarded unless the character is successfully synthesized. The writer also serializes most metric tables from the parsed source font without recomputing values affected by newly appended composite glyphs.

The product CLI should stay simple: it generates a font from explicit inputs and should not emit routine warning text. Detailed integrity checks are a development concern and fit the existing `font_tool` utility.

## Goals / Non-Goals

**Goals:**
- Preserve source-font character coverage by treating the original cmap as the baseline for generated fonts.
- Override cmap entries only for characters that receive a generated pinyin composite glyph.
- Keep generated OpenType metadata internally consistent for appended composite glyphs, especially `maxp` fields describing component usage.
- Provide a development diagnostic path that can compare generated glyph bounds against advance widths and font metrics.
- Keep the change narrow enough to validate with existing sample fonts and focused tests.

**Non-Goals:**
- Redesign the visual pinyin layout algorithm or tune the fixed base/pinyin scaling ratios.
- Change normal `pinyinfont` CLI output into a validation report.
- Introduce a public C++ API for font inspection or synthesis.
- Guarantee correctness for every optional OpenType table not currently parsed or rewritten by the project.

## Decisions

1. Use source cmap as the baseline map.

   The builder should initialize its output cmap map from the parsed font's existing cmap groups, then replace entries only when a new synthesized glyph is successfully added. This matches the overlay semantics users expect: unsupported or failed characters continue to render exactly as they did in the source font.

   Alternative considered: add fallback entries only for failed pinyin DB records. That fixes one failure mode but still drops source mappings outside the retained common ranges.

2. Update metrics that are deterministically affected by added glyphs.

   The font model should maintain writer-visible values that can be computed from glyph headers, hmtx records, and composite component lists. At minimum, appended composite glyphs must update `maxp.MaxComponentElements` and `maxp.MaxComponentDepth`; horizontal derived metrics and global bbox values should either be recomputed or intentionally preserved only when generated glyphs are guaranteed to remain within the original values.

   Alternative considered: leave all global metrics unchanged to preserve line layout exactly. This keeps visual layout stable but produces inaccurate metadata when synthesized glyphs exceed original bounds or component maxima.

3. Keep line-height policy explicit.

   `hhea` ascender/descender and `OS/2` typo/win ascender/descender should not be expanded casually because doing so changes downstream layout. Integrity diagnostics should report when generated glyphs exceed these bounds; any policy to expand line metrics should be a deliberate follow-up rather than an incidental side effect.

4. Put integrity diagnostics in `font_tool`.

   A development command can inspect original and generated fonts, report cmap preservation, bbox overflow, advance overflow, and table inconsistencies, and be used by tests or manual validation. The product CLI remains focused on generation.

## Risks / Trade-offs

- Recomputing `head` or `hhea` derived metrics can make generated metadata more accurate while changing values that some renderers or layout engines may use. Mitigation: distinguish table consistency fields from line-height policy, and report line-bound overflow before expanding line metrics.
- Preserving the entire source cmap may keep mappings to glyphs that the writer otherwise preserves but does not visually modify. This is intended overlay behavior; synthesized glyphs override only successful pinyin targets.
- Existing sample fonts may not cover all OpenType edge cases. Mitigation: validate with multiple repository samples and add tests around deterministic cmap and `maxp` behavior.
