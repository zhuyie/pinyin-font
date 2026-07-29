## MODIFIED Requirements

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

## ADDED Requirements

### Requirement: Punctuation cmap replacements can be audited
The generated-font integrity diagnostic SHALL distinguish intentional scaled-punctuation mappings from unexplained source cmap changes.

#### Scenario: Scaled punctuation passes cmap audit
- **WHEN** a source punctuation mapping is replaced by a valid scaled wrapper
- **THEN** the integrity diagnostic does not report the replacement as an unexplained cmap change

#### Scenario: Non-whitelist mapping change is reported
- **WHEN** a non-target source mapping changes without a valid pinyin or punctuation replacement
- **THEN** the integrity diagnostic reports the changed mapping
