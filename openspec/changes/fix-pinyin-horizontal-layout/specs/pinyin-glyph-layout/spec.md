## ADDED Requirements

### Requirement: Pinyin clusters use source horizontal metrics
The generator SHALL position consecutive pinyin base-letter clusters using the resolved base glyphs' source horizontal advances rather than global font outline bounds.

#### Scenario: Consecutive base letters are composed
- **WHEN** a reading contains multiple pinyin base letters
- **THEN** each cluster origin advances according to the preceding resolved base glyph's horizontal advance

#### Scenario: Combining marks are attached
- **WHEN** a base letter has one or more combining marks
- **THEN** the marks are horizontally associated with that base cluster without adding a separate logical advance

### Requirement: Pinyin annotation ink fits its Han advance cell
The generator SHALL fit the complete pinyin component ink union within the generated Han glyph's horizontal advance while preserving the configured pinyin vertical scale.

#### Scenario: Annotation fits at the configured scale
- **WHEN** the complete pinyin ink union fits within the Han advance at the configured pinyin scale
- **THEN** the generator uses the configured scale on both axes

#### Scenario: Long annotation exceeds the advance
- **WHEN** the complete pinyin ink union would exceed the Han advance at the configured pinyin scale
- **THEN** the generator reduces the common X scale sufficiently to contain the annotation while retaining the configured Y scale

#### Scenario: Tone mark overhangs its base letter
- **WHEN** a source or generated tone-mark component extends beyond its base letter's horizontal ink bounds
- **THEN** the mark participates in the fitted ink union and remains within the Han advance cell

### Requirement: Pinyin is centered on the Han advance cell
The generator SHALL center the fitted pinyin ink union on the source Han glyph's horizontal advance cell rather than its outline bounding-box center.

#### Scenario: Han outlines have asymmetric side bearings
- **WHEN** two Han glyphs have the same advance but different outline centers
- **THEN** their pinyin ink unions have the same advance-cell center

### Requirement: Unrelated horizontal extrema do not affect pinyin layout
The generator SHALL keep pinyin horizontal spacing, scale, and placement independent of source glyphs that do not participate in the synthesized reading or Han body.

#### Scenario: Source font contains an unrelated wide glyph
- **WHEN** two otherwise equivalent fonts differ only by an unmapped glyph with extreme X bounds
- **THEN** corresponding synthesized pinyin components have identical X scales and offsets
