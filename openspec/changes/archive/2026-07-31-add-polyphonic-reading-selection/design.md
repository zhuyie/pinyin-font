## Context

`PinyinDB` already retains as many as four normalized readings per character, but `PinyinFontBuilder::__addPinyinGlyphs` passes only the first reading to synthesis. Each successful default annotation replaces the source Han character's `cmap` entry with a generated TrueType composite glyph. The writer rebuilds a TrueType-flavored OpenType font from known tables and currently has no OpenType Layout model or GSUB writer.

The selected-reading syntax must be expressible in plain text, disappear visually when supported, and degrade without corrupting the input when OpenType substitution is unavailable. The feature must therefore operate after `cmap` mapping, while continuing to use the project's existing composite-glyph synthesis.

## Goals / Non-Goals

**Goals:**

- Select any stored reading of a polyphonic character with the one-based syntax `汉字@序号`.
- Keep the first stored reading as the unmarked default.
- Consume a valid selector visually through standard OpenType shaping.
- Reuse the existing annotated TrueType composite construction for every selected reading.
- Emit a structurally valid, inspectable GSUB table without adding a runtime dependency to the generated font.
- Preserve literal selector text when no valid substitution exists.

**Non-Goals:**

- Inferring pronunciation from words, phrases, or language models.
- Interpreting the suffix number as a Mandarin tone number.
- Removing selector characters from the underlying text, clipboard, search index, or accessibility representation.
- Supporting more readings than the four entries currently retained by `PinyinRecord`.
- Guaranteeing substitution in renderers that do not run OpenType Layout or that disable standard ligatures.

## Decisions

### Use a one-based database candidate index

`@1` through `@4` refer directly to the non-empty `PinyinRecord::pinyin` entries in their stored order. The first reading remains the default, and `@1` is also accepted so generated or edited text can use one uniform explicit form.

This is preferred over tone-number selection because multiple distinct readings can share a tone and tone alone cannot always identify a candidate. It also avoids adding pronunciation parsing after database normalization. Candidate ordering consequently becomes observable behavior and must remain stable when the database is updated.

### Implement selectors as standard ligatures

For each eligible reading, synthesis creates or reuses an annotated composite glyph and records a ligature rule:

```text
default-annotated-Han + at-sign + ASCII-index -> selected-annotated-Han
```

The rule is encoded as GSUB Lookup Type 4 (Ligature Substitution) under the standard `liga` feature. The generated table exposes the feature through both `DFLT` and `hani` script systems with a default language system. `liga` is chosen because the complete three-glyph sequence is replaced atomically, consuming `@序号`; contextual substitution plus zero-width selector glyphs would leave more fragile layout and caret behavior.

The selected annotated glyphs are internal glyphs and do not receive Unicode `cmap` mappings. The default annotated glyph remains the `cmap` target for the Han character and is the first component used by GSUB coverage.

### Generate only valid, supported rules

A record is polyphonic when it contains at least two non-empty normalized readings. A ligature is emitted only when:

- the default annotated Han glyph was synthesized successfully;
- the selected annotated glyph was synthesized successfully; and
- the source/output `cmap` maps `@` and the relevant ASCII digit.

Failure to synthesize one alternate reading omits only that selector rule. Missing selector glyphs do not fail the whole font build. Diagnostics count and explain omitted rules so a generated font never contains a ligature that targets `.notdef` or a missing glyph.

### Add focused GSUB data to the font model and writer

The in-memory font representation will retain a compact list of ligature substitutions rather than a general-purpose mutable OpenType Layout object model. The writer will serialize that list into the minimum conforming GSUB Version 1.0 structure: ScriptList, FeatureList, LookupList, Type 4 subtable, coverage, ligature sets, and ligature records.

Rules are grouped by first glyph, sorted deterministically, and emitted with the longest applicable component sequence first if future syntax introduces overlaps. All offsets and table lengths are checked before narrowing to 16-bit GSUB offsets. The writer includes `GSUB` in the sorted SFNT table directory and global checksum calculation only when at least one rule exists.

This targeted representation is preferred over importing a font-shaping library because the project currently owns its TrueType parser/writer and needs only one well-defined lookup form. It does not attempt to preserve arbitrary source GSUB data, consistent with the current writer's table reconstruction behavior.

### Validate structure and behavior separately

Unit tests cover ligature-table serialization, offsets, coverage grouping, and directory/checksum integration. Synthesis tests use records such as `藏 cáng,zàng` to confirm that the default glyph and selected glyph differ and that the `藏 @ 2` glyph sequence resolves to the alternate. Diagnostics inspect the emitted rule set so correctness does not depend solely on a browser screenshot; the preview page remains useful for manual shaping-engine validation.

## Risks / Trade-offs

- [Some applications disable `liga` or do not run GSUB] → Preserve literal `@序号` as the fallback and document the shaping requirement.
- [Underlying text still contains the selector] → Treat the syntax as author-visible markup and explicitly document clipboard, search, and accessibility behavior.
- [Database candidate reordering changes existing documents] → Define candidate order as a compatibility contract and add fixture tests for representative records.
- [Generating alternate composites increases glyph count and font size] → Generate alternates only for polyphonic records and only when their selector rule can be emitted.
- [A source font lacks `@` or an index digit] → Skip affected rules, continue the build, and report the omission.
- [GSUB uses 16-bit relative offsets] → Precompute and validate serialized sizes; fail GSUB generation safely rather than writing malformed offsets.
- [A source font's own GSUB behavior is not retained by the existing writer] → Keep this change scoped to the generated selector GSUB and do not claim general GSUB preservation.

## Migration Plan

No input database or CLI migration is needed. Existing text without selectors continues to use the first reading exactly as before. Rollback consists of generating the font with the prior version; selector-bearing text then renders literally as `汉字@序号`.

## Open Questions

- Whether a future CLI option should disable selector glyph generation for users prioritizing minimum font size.
- Whether later work should preserve and merge arbitrary source GSUB tables instead of emitting only the generated selector feature.
