## Purpose

Define cmap preservation and metric consistency requirements for fonts produced by the pinyin synthesis workflow.

## Requirements

### Requirement: Generated fonts preserve source character mappings
The pinyin font generator SHALL preserve the source font cmap as the baseline mapping and SHALL replace mappings only for successfully synthesized pinyin glyphs or successfully scaled whitelist punctuation.

#### Scenario: Non-target source mapping is preserved
- **WHEN** the source font maps a character that is neither replaced by a synthesized pinyin glyph nor eligible for punctuation scaling
- **THEN** the generated font maps that character to the same glyph as the source font

#### Scenario: Successful pinyin synthesis overrides source mapping
- **WHEN** a source character is successfully synthesized into a pinyin composite glyph
- **THEN** the generated font maps that character to the new pinyin composite glyph

#### Scenario: Successful punctuation scaling overrides source mapping
- **WHEN** a whitelisted source punctuation glyph is successfully wrapped at the Han body scale
- **THEN** the generated font maps that character to the new punctuation composite glyph

#### Scenario: Failed synthesis keeps source mapping
- **WHEN** a character targeted for pinyin synthesis or punctuation scaling cannot be synthesized safely
- **THEN** the generated font keeps that character mapped to the original source glyph

### Requirement: Generated composite glyph metadata is consistent
The pinyin font generator SHALL update OpenType metadata that directly describes appended pinyin and punctuation composite glyphs.

#### Scenario: Glyph count is updated
- **WHEN** pinyin or punctuation composite glyphs are appended to the font
- **THEN** `maxp.NumGlyphs` matches the generated font glyph count

#### Scenario: Horizontal metric count is updated
- **WHEN** pinyin or punctuation composite glyphs are appended to the font
- **THEN** `hhea.NumberOfHMetrics` matches the generated font horizontal metrics count

#### Scenario: Composite component maxima are updated
- **WHEN** generated composite glyphs contain more top-level components than the source font maximum
- **THEN** `maxp.MaxComponentElements` reflects the generated font maximum

#### Scenario: Nested composite depth is updated
- **WHEN** a generated composite references a source glyph that is itself composite
- **THEN** `maxp.MaxComponentDepth` is sufficient for the complete reachable component depth

### Requirement: Punctuation cmap replacements can be audited
The generated-font integrity diagnostic SHALL distinguish intentional scaled-punctuation mappings from unexplained source cmap changes.

#### Scenario: Scaled punctuation passes cmap audit
- **WHEN** a source punctuation mapping is replaced by a valid scaled wrapper
- **THEN** the integrity diagnostic does not report the replacement as an unexplained cmap change

#### Scenario: Non-whitelist mapping change is reported
- **WHEN** a non-target source mapping changes without a valid pinyin or punctuation replacement
- **THEN** the integrity diagnostic reports the changed mapping

### Requirement: Generated font metrics can be audited
The project SHALL provide a development diagnostic path for checking generated font metric integrity.

#### Scenario: Cmap preservation can be checked
- **WHEN** a developer runs the integrity diagnostic on an original font and generated font
- **THEN** the diagnostic reports source cmap mappings that were dropped or changed without a generated pinyin replacement

#### Scenario: Glyph bounds can be checked against advance widths
- **WHEN** a developer runs the integrity diagnostic on a generated font
- **THEN** the diagnostic separately reports generated glyphs whose bounding-box minimum is left of zero or whose bounding-box maximum is right of the advance width

#### Scenario: Glyph bounds can be checked against line metrics
- **WHEN** a developer runs the integrity diagnostic on a generated font
- **THEN** the diagnostic reports generated glyphs whose bounding boxes exceed `hhea` or `OS/2` vertical metric bounds

#### Scenario: Composite metadata can be checked
- **WHEN** a developer runs the integrity diagnostic on a generated font
- **THEN** the diagnostic reports `maxp` composite maxima that are lower than the generated glyph data requires

### Requirement: Internal simple glyph metadata is consistent
The generator SHALL update OpenType metadata that describes appended internal simple glyphs used for pinyin fallback.

#### Scenario: Internal component glyph is appended
- **WHEN** a generated tone mark or derived dotless-`i` simple glyph is added
- **THEN** glyph count, horizontal metric count, global bounds, and applicable `maxp` simple-glyph maxima describe the appended glyph

#### Scenario: Derived glyph instructions are removed
- **WHEN** an internal simple glyph is derived by removing source contours
- **THEN** the generated glyph has no stale instructions and does not increase instruction metadata based on discarded source instructions

### Requirement: Component fallback outcomes can be audited
The project SHALL provide development diagnostics that distinguish source coverage, component fallback usage, and fallback failure reasons.

#### Scenario: Fallback components are used
- **WHEN** a developer audits a generated font that contains internal pinyin components
- **THEN** the diagnostic reports the generated component kinds and the number of synthesized characters that used component fallback

#### Scenario: Source Han glyph is absent
- **WHEN** a pinyin database entry cannot be synthesized because the source font has no mapped Han glyph
- **THEN** the diagnostic classifies it separately from pinyin-component failures

#### Scenario: Dotless i cannot be derived
- **WHEN** accented-`i` synthesis fails because source contours cannot be separated reliably
- **THEN** the diagnostic reports dotless-`i` derivation as the failure reason

#### Scenario: Fallback synthesis still fails
- **WHEN** any internal component cannot be generated or placed safely
- **THEN** the diagnostic reports the reason and cmap integrity confirms that the original source mapping was preserved

### Requirement: Generated cmap subtables are structurally valid
The generator SHALL serialize every emitted cmap subtable within the field and offset limits defined by its OpenType format.

#### Scenario: Format 4 mapping fits after compact encoding
- **WHEN** the complete BMP mapping can be represented by format 4 within its unsigned 16-bit length limit
- **THEN** the generator emits a complete format 4 subtable with valid segment arrays, offsets, search parameters, and declared length

#### Scenario: Format 4 mapping cannot fit
- **WHEN** the complete BMP mapping cannot be represented by format 4 within its field limits
- **THEN** generation fails explicitly without writing a truncated, partial, or length-wrapped format 4 subtable

#### Scenario: Format 4 reaches its legal boundary
- **WHEN** a format 4 representation reaches the largest legal declared length
- **THEN** the serialized length and all referenced data remain within the subtable boundary

### Requirement: Emitted Unicode cmap subtables agree on BMP mappings
When the generator emits multiple Unicode cmap subtables, it SHALL encode the same character-to-glyph mapping for every BMP character covered by more than one emitted subtable.

#### Scenario: Format 4 and format 12 are both emitted
- **WHEN** the generated font contains both format 4 and format 12 Unicode cmap subtables
- **THEN** decoding either subtable produces identical glyph IDs for their shared BMP characters

#### Scenario: Synthesized mapping replaces a source mapping
- **WHEN** a BMP character is remapped to a synthesized pinyin or scaled punctuation glyph
- **THEN** every emitted Unicode cmap subtable covering that character maps it to the replacement glyph

### Requirement: Cmap integrity diagnostics inspect every emitted subtable
The generated-font integrity diagnostic SHALL validate each emitted supported Unicode cmap subtable rather than validating only the preferred subtable.

#### Scenario: Preferred format 12 is valid but format 4 is corrupt
- **WHEN** format 12 can be parsed but a sibling format 4 subtable has an invalid declared length or out-of-range data
- **THEN** the integrity diagnostic reports the format 4 corruption

#### Scenario: Emitted subtables disagree
- **WHEN** two emitted Unicode cmap subtables map a shared BMP character to different glyph IDs
- **THEN** the integrity diagnostic reports the inconsistent character mapping
