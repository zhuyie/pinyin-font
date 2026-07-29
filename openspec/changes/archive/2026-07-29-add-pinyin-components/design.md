## Context

The synthesis pipeline currently normalizes pinyin readings, substitutes source precomposed glyphs where configured, and builds TrueType composite glyphs from source Latin glyphs plus a scaled source Han glyph. A missing base Han glyph returns `kNotFound`; a missing Latin or mark glyph causes composition to fail while the source-cmap baseline preserves the original character.

On `ZCOOLXiaoWei-Regular.ttf`, 3,842 of 7,909 loaded records synthesize successfully, 1,271 lack a source Han glyph, and 2,796 have a source Han glyph but fail because pinyin components are unavailable. The component failures are dominated by a missing acute mark and by unconditional substitution to unavailable `ī`, `í`, `ǐ`, and `ì` glyphs.

The project already parses and writes TrueType simple and composite glyphs, exposes `OS/2.sxHeight`, updates appended-glyph metrics through `OpenType_Font::AddGlyph()`, preserves the full source cmap as a baseline, and provides `font_tool integrity` plus `tools/preview.html` for validation. The parser explicitly rejects fonts without `glyf` and `loca`, so this design does not need to generalize across CFF outline storage.

## Goals / Non-Goals

**Goals:**

- Synthesize pinyin when the source Han and Latin base glyphs exist but tone-mark glyphs are missing.
- Preserve source-font Latin style by preferring source glyphs and deriving dotless `i` from the source `i`.
- Generate a minimal reusable set of internal TrueType tone-mark glyphs with source-relative geometry.
- Keep cmap fallback and all generated-font metadata consistent.
- Make coverage gains and remaining failures explainable through developer diagnostics and tests.

**Non-Goals:**

- Parse or write CFF/CFF2 outlines.
- Supply Han glyphs absent from the source font.
- Select among multiple pinyin readings.
- Preserve hinting on geometrically derived glyphs.
- Build a general-purpose outline editor or public font-manipulation API.
- Guarantee dotless-`i` derivation for connected, composite, or decorative source `i` glyphs.

## Decisions

### Generate fallback marks procedurally in target font units

The generator will build simple TrueType contours for macron, acute, caron, grave, and diaeresis. Macron is a centered rectangle; acute and grave are mirrored slanted quadrilaterals; caron is two overlapping slanted contours; diaeresis is two symmetric polygonal or quadratic dots.

Dimensions will be calculated directly in the source font's units rather than copied from a bundled component font. This avoids an extra binary asset, font licensing questions, units-per-em conversion, and fixed visual proportions.

Alternative considered: bundle `pinyin-components.ttf`. Rejected because fixed component glyphs still require optical scaling and can introduce an unrelated Latin design style.

### Resolve glyphs through a source-first fallback registry

Composition will use a resolver with this order:

1. Use a source precomposed accented vowel when it exists.
2. Otherwise use a source combining mark or current source fallback when usable.
3. Otherwise lazily create and cache one internal glyph for the required mark.

Internal component glyphs receive stable internal names and glyph IDs but no cmap entries. Lazy creation avoids adding unused glyphs while ensuring repeated use shares one component.

The unconditional accented-`i` substitution rule will be replaced with source-availability-aware resolution. It must not substitute to an absent precomposed glyph.

Alternative considered: always replace source marks with generated marks for uniformity. Rejected because source glyphs better match the font's design.

### Derive component geometry from x-height and actual vowel bounds

The geometry context will resolve x-height in this order:

1. A positive, plausible `OS/2.sxHeight`.
2. The median upper bound or measured height of available representative lowercase glyphs without ascenders or descenders.
3. A bounded ratio of `head.UnitsPerEm`.

Mark width, height, stroke, and inter-mark gap will use per-mark ratios of the resolved x-height with defensive lower and upper bounds. These ratios are implementation constants covered by visual fixtures rather than public configuration in this change.

Placement remains glyph-specific:

- horizontal anchor: center of the actual vowel bbox;
- vertical anchor: actual vowel `YMax` plus the resolved gap;
- second mark: previous transformed mark `YMax` plus the resolved gap.

Alternative considered: use one global fixed Y position. Rejected because lowercase height and individual vowel bounds vary across fonts.

### Derive dotless i conservatively from a simple source glyph

When a source `ī`, `í`, `ǐ`, or `ì` glyph is unavailable, the generator will inspect the source `i` only if it is a simple TrueType glyph. It will:

1. Partition contours into visual groups using overlapping or containing contour bounds so a hollow dot remains one group.
2. Identify the main body from the group containing the lowest extent and dominant vertical mass.
3. Accept an upper group as the dot only when it is vertically separated, horizontally centered over the body, and bounded in size and area relative to x-height and the body.
4. Require one unambiguous removable dot group.
5. Copy all remaining contours into a reusable internal dotless-`i` glyph.

The derived glyph retains the source `i` advance width, recomputes bounds and contour endpoints, and clears instructions because point indexes change after contour removal.

Composite, connected, or ambiguous `i` glyphs fail conservatively. Synthesis then preserves the source Han mapping and records a specific diagnostic reason.

Alternative considered: use `i` plus a tone mark without removing the dot. Rejected because the result is typographically incorrect. Alternative considered: bundle a project dotless `i`; rejected because its stem and serif style may not match the source font.

### Extend normalization instead of bundling exceptional precomposed glyphs

`ḿ` and supported accented `n` forms will normalize to base ASCII letters plus combining tone marks. They then use the same source-first component resolver. This keeps the component set minimal and makes fallback behavior consistent.

### Keep outline creation behind internal synthesis/OpenType helpers

The synthesis layer owns decisions about which pinyin component is needed and how it is positioned. Internal OpenType helpers own safe simple-glyph construction, contour copying, bbox calculation, metric insertion, and `maxp` maintenance. No public API is introduced.

`OpenType_Font::AddGlyph()` will be extended so appended simple glyphs update `MaxPoints` and `MaxContours` in addition to existing glyph-count, bounds, and horizontal metrics. Composite maxima continue to be updated for generated Han glyphs.

### Separate coverage outcomes in diagnostics

Synthesis bookkeeping will distinguish at least:

- generated without fallback;
- generated with tone fallback;
- generated with derived dotless `i`;
- source Han glyph missing;
- component glyph unavailable;
- dotless-`i` derivation unsupported or ambiguous;
- other composition failure.

Normal product CLI output remains concise. Detailed categories belong in development diagnostics and focused tests.

## Risks / Trade-offs

- [Procedural marks may not perfectly match decorative fonts] → Prefer source glyphs, derive dimensions from source metrics, keep ratios bounded, and validate representative serif, sans, and display fonts visually.
- [Contour heuristics could remove part of an unusual `i`] → Require one high-confidence dot group and fail closed on composite, connected, or ambiguous outlines.
- [Dropping hinting can reduce small-size crispness] → Limit instruction removal to derived internal glyphs and verify at several browser sizes; correctness is preferred over stale point references.
- [Internal simple glyphs can expose stale `maxp` values] → Centralize simple-glyph metadata updates in `AddGlyph()` and extend integrity checks.
- [Lazy component creation makes glyph IDs input-dependent] → Treat glyph IDs as internal implementation details and test semantic mappings and component reuse rather than fixed numeric IDs.
- [Metric heuristics vary across sparse Latin subsets] → Use a documented resolution order and units-per-em fallback with bounds.
- [Coverage measurements can hide source-character gaps] → Report source Han absence separately from component failure and calculate coverage against both database total and source-supported targets.

## Migration Plan

1. Add internal geometry and simple-glyph metadata support with unit tests.
2. Add source-relative tone component generation and caching without changing substitution behavior.
3. Route missing mark resolution through the fallback registry and extend normalization.
4. Add conservative dotless-`i` derivation and replace unconditional accented-`i` substitution.
5. Extend diagnostics, integrity checks, and browser preview samples.
6. Run fresh build/CTest/OpenSpec validation and end-to-end generation against representative fonts.

Rollback is a normal branch revert: generated fonts are build artifacts, source cmap remains the baseline, and no persistent external data format is migrated.

## Open Questions

- Which representative tracked or generated font fixtures provide enough outline diversity without adding licensing-sensitive binaries?
- Should the first implementation use polygonal diaeresis dots for simplicity or quadratic circles for better visual fidelity?
- What confidence thresholds for dot grouping provide the best balance between coverage and fail-closed behavior across the selected fixture set?
