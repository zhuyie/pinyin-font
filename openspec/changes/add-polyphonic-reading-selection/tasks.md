## 1. OpenType Ligature Model

- [x] 1.1 Add a compact in-memory representation for GSUB ligature substitutions and APIs for registering deterministic glyph sequences and replacement glyphs
- [x] 1.2 Extend the font builder with unmapped composite-glyph support so alternate annotated readings can be referenced by GSUB without changing `cmap`
- [x] 1.3 Add unit tests for ligature rule grouping, ordering, duplicate handling, and unmapped composite glyph creation

## 2. GSUB Serialization

- [x] 2.1 Serialize a minimal GSUB Version 1.0 table with `DFLT` and `hani` default language systems, the `liga` feature, and a Type 4 Ligature Substitution lookup
- [x] 2.2 Integrate the optional `GSUB` table into sorted SFNT directory generation, table checksums, whole-font checksum adjustment, and offset/size validation
- [x] 2.3 Add focused writer tests that parse or inspect generated ScriptList, FeatureList, LookupList, coverage, ligature sets, glyph references, and relative offsets

## 3. Polyphonic Glyph Synthesis

- [x] 3.1 Enumerate non-empty `PinyinRecord` readings with stable one-based indices while preserving `pinyin[0]` as the default unmarked annotation
- [x] 3.2 Generate internal composite glyphs for synthesizable alternate readings of polyphonic records without adding Unicode mappings
- [x] 3.3 Register `default-Han + @ + ASCII-index -> selected-Han` ligatures only when every input and output glyph is valid, including explicit `@1`
- [x] 3.4 Isolate failures so an unsynthesizable alternate or missing selector glyph omits only affected rules and does not change the default mapping or abort the build

## 4. Diagnostics and Validation

- [x] 4.1 Extend synthesis statistics and CLI diagnostic output with generated alternate-glyph and selector-ligature totals
- [x] 4.2 Classify selector omissions as missing selector input glyphs or alternate-reading synthesis failures
- [x] 4.3 Extend the font integrity diagnostic to validate generated GSUB structure and distinguish valid selector references from malformed or out-of-range glyph references

## 5. Behavioral Tests and Preview

- [x] 5.1 Add a polyphonic fixture containing ordered readings such as `藏 cáng,zàng` and the `@`/digit input glyphs required for selector shaping
- [x] 5.2 Test default reading compatibility and valid `@1` and `@2` substitutions, including that alternate glyphs remain absent from `cmap`
- [x] 5.3 Test literal fallback for out-of-range indices, single-reading characters, missing selector glyphs, disabled/unapplied ligatures, and failed alternate synthesis
- [x] 5.4 Add representative selector samples such as `收藏@1 西藏@2` to the browser preview and document the OpenType shaping and underlying-text behavior
- [x] 5.5 Run the full test suite and generated-font integrity checks against an approved source-font fixture
