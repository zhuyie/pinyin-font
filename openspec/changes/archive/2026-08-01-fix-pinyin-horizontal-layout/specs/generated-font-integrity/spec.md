## MODIFIED Requirements

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
