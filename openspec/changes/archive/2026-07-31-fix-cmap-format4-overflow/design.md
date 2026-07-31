## Context

The in-memory cmap is stored as format-12-style sequential mapping groups. The writer currently emits every BMP group as one format 4 `idDelta` segment and always emits Windows 3/1 format 4 beside Windows 3/10 format 12. A complete pinyin database replaces thousands of mappings in a large CJK font, fragmenting the sequential glyph-ID groups even when the Unicode coverage remains dense.

Format 4 stores its total length in an unsigned 16-bit field. Noto Sans CJK SC and Noto Serif CJK SC produce about 8,700 segments with the current encoding, requiring roughly 69 KB. The writer calculates that size in 32 bits but narrows it while serializing, producing a corrupt subtable. The parser and existing integrity checks select format 12 as the preferred subtable and therefore do not detect the corrupt sibling format 4 table.

## Goals / Non-Goals

**Goals:**

- Serialize a complete BMP mapping into format 4 compactly when it fits.
- Enforce every format 4 field limit before allocating or writing the table.
- Preserve identical BMP character-to-glyph mappings in format 4 and format 12.
- Fail generation explicitly rather than emitting a partial or corrupt cmap.
- Add boundary and full-database regression coverage that exercises all emitted cmap subtables.

**Non-Goals:**

- Changing pinyin database contents, reading selection, glyph composition, or layout.
- Reassigning existing source glyph IDs to make cmap mappings more sequential.
- Supporting characters at or above `U+10000` in the pinyin database.
- Adding a new third-party font serialization dependency.

## Decisions

### Plan format 4 independently from format 12 groups

The writer will derive BMP codepoint-to-glyph mappings from the canonical cmap groups, then build format 4 segments specifically for format 4. It will not assume that each format 12 group must become one format 4 segment.

For each mapped BMP region, the planner can represent mappings as:

- an `idDelta` segment when glyph IDs advance with codepoints; or
- an `idRangeOffset` segment backed by `glyphIdArray` when mappings are dense but glyph IDs are non-linear.

The planner will compare encoded byte costs and merge mappings into glyph-array ranges when doing so is smaller than retaining fragmented delta segments. Unmapped entries included inside such a range use glyph ID zero. This keeps the strategy Unicode-block independent while allowing the dense CJK range in Noto fonts to fit below 64 KB.

Alternative considered: emit every format 12 group as a delta segment. This is the current behavior and cannot represent the full Noto result.

Alternative considered: omit format 4 unconditionally. Format 12 is sufficient for modern full-repertoire consumers, but retaining a valid Windows BMP subtable provides broader compatibility and follows the existing output contract.

Alternative considered: keep the source format 4 unchanged and place replacements only in format 12. This would make the two subtables disagree for BMP characters and could display different glyphs depending on consumer selection.

### Treat format 4 limits as checked serialization constraints

The planner will calculate the complete subtable size before writing and validate:

- total length fits the unsigned 16-bit `length` field;
- `segCountX2` and search parameters fit their fields;
- segment ranges are ordered and non-overlapping;
- the required `U+FFFF` sentinel is present;
- every glyph-array offset resolves within the subtable.

If a complete format 4 representation cannot fit, font generation will return an explicit failure. The writer will never narrow an unchecked length, truncate mappings, or emit only a subset of BMP replacements.

Alternative considered: fall back to format 12 only on overflow. This may be valid for some full-repertoire fonts, but introduces a font-dependent compatibility policy that the current API cannot report clearly. Explicit failure is deterministic and leaves a format-12-only policy as a separate future change.

### Validate every emitted Unicode cmap subtable

Tests and development diagnostics will enumerate cmap encoding records and parse each supported Unicode subtable within its declared boundary. Validation will compare the BMP mapping decoded from format 4 with the BMP portion decoded from format 12.

The normal font parser can continue selecting one preferred subtable for its canonical in-memory cmap. Structural validation is a separate concern and must not depend on that selection behavior.

### Cover the exact boundary and the reported full-font case

Unit tests will cover a format 4 table at the largest legal size, a mapping that requires compact glyph-array encoding, and an unrepresentable mapping that must fail. Integration coverage will synthesize a static CJK font with the complete local pinyin database when the fixture is available, then validate every cmap subtable.

## Risks / Trade-offs

- [Compact segment planning is more complex than direct group serialization] → Keep planning separate from byte emission and test round-trip mappings for both delta and glyph-array segments.
- [A compact representation may still exceed 65,535 bytes for an adversarial BMP map] → Return a clear error before writing any malformed table.
- [An integration test may depend on large font fixtures not present in every environment] → Keep deterministic synthetic boundary tests mandatory and make the real-font regression fixture-aware.
- [The project parser currently exposes only its preferred cmap] → Add targeted cmap-table validation without changing normal subtable selection semantics.

## Migration Plan

No data migration or CLI change is required. Existing generated fonts remain unchanged on disk. Regenerating affected Noto fonts after the change produces a structurally valid cmap; reverting the code restores the previous writer but must not be used to regenerate full-database Noto output.

## Open Questions

- Whether a future explicit compatibility option should permit format-12-only output when format 4 cannot fit.
