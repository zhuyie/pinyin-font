## Purpose

Define cmap preservation and metric consistency requirements for fonts produced by the pinyin synthesis workflow.

## Requirements

### Requirement: Generated fonts preserve source character mappings
The pinyin font generator SHALL preserve the source font cmap as the baseline mapping for generated fonts.

#### Scenario: Non-target source mapping is preserved
- **WHEN** the source font maps a character that is not replaced by a synthesized pinyin glyph
- **THEN** the generated font maps that character to the same glyph as the source font

#### Scenario: Successful synthesis overrides source mapping
- **WHEN** a source character is successfully synthesized into a pinyin composite glyph
- **THEN** the generated font maps that character to the new pinyin composite glyph

#### Scenario: Failed synthesis keeps source mapping
- **WHEN** a source character is listed in the pinyin database but cannot be synthesized
- **THEN** the generated font keeps that character mapped to the original source glyph

### Requirement: Generated composite glyph metadata is consistent
The pinyin font generator SHALL update OpenType metadata that directly describes appended composite glyphs.

#### Scenario: Glyph count is updated
- **WHEN** pinyin composite glyphs are appended to the font
- **THEN** `maxp.NumGlyphs` matches the generated font glyph count

#### Scenario: Horizontal metric count is updated
- **WHEN** pinyin composite glyphs are appended to the font
- **THEN** `hhea.NumberOfHMetrics` matches the generated font horizontal metrics count

#### Scenario: Composite component maxima are updated
- **WHEN** generated pinyin composite glyphs contain more top-level components than the source font maximum
- **THEN** `maxp.MaxComponentElements` reflects the generated font maximum

#### Scenario: Composite depth is updated
- **WHEN** generated pinyin composite glyphs are present
- **THEN** `maxp.MaxComponentDepth` is at least sufficient for the generated composite glyphs

### Requirement: Generated font metrics can be audited
The project SHALL provide a development diagnostic path for checking generated font metric integrity.

#### Scenario: Cmap preservation can be checked
- **WHEN** a developer runs the integrity diagnostic on an original font and generated font
- **THEN** the diagnostic reports source cmap mappings that were dropped or changed without a generated pinyin replacement

#### Scenario: Glyph bounds can be checked against advance widths
- **WHEN** a developer runs the integrity diagnostic on a generated font
- **THEN** the diagnostic reports generated glyphs whose bounding boxes overflow their advance widths

#### Scenario: Glyph bounds can be checked against line metrics
- **WHEN** a developer runs the integrity diagnostic on a generated font
- **THEN** the diagnostic reports generated glyphs whose bounding boxes exceed `hhea` or `OS/2` vertical metric bounds

#### Scenario: Composite metadata can be checked
- **WHEN** a developer runs the integrity diagnostic on a generated font
- **THEN** the diagnostic reports `maxp` composite maxima that are lower than the generated glyph data requires
