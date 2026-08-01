# Pinyin Glyph Layout Specification

## Purpose

Define how synthesized Han, pinyin, and punctuation components are positioned so generated glyphs remain stable across source fonts.

## Requirements

### Requirement: Pinyin composition uses a typographic vertical band
The generator SHALL derive the shared Han-body offset and pinyin baseline from valid font-wide typographic ascender and descender metrics rather than global outline extrema when valid typographic metrics are available.

#### Scenario: Valid OS/2 typo metrics are available
- **WHEN** `sTypoAscender` is positive, `sTypoDescender` is non-positive, and the ascender is greater than the descender
- **THEN** the generator uses the OS/2 typo ascender and descender as the vertical composition band

#### Scenario: OS/2 typo metrics are invalid
- **WHEN** the OS/2 typo metric pair is absent or invalid and the hhea ascender and descender form a valid pair
- **THEN** the generator uses the hhea ascender and descender as the vertical composition band

#### Scenario: Typographic metric pairs are unusable
- **WHEN** neither the OS/2 typo metrics nor hhea metrics form a valid pair
- **THEN** the generator falls back to the global head vertical bounds

### Requirement: Unrelated outline extrema do not affect pinyin layout
The generator SHALL keep synthesized component placement independent of source glyphs that do not participate in the synthesized character or its pinyin components when valid typographic metrics are available.

#### Scenario: Source font contains an unrelated extreme glyph
- **WHEN** two otherwise equivalent fonts have the same typographic metrics but one contains an unrelated glyph with extreme vertical bounds
- **THEN** corresponding synthesized Han and pinyin component Y offsets are identical in both generated fonts

#### Scenario: Noto CJK global bounds exceed common Han bounds
- **WHEN** a Noto CJK font has valid typo metrics and global outline extrema far outside its ordinary Han glyph range
- **THEN** the generated pinyin baseline is derived from the typo metrics rather than the global extrema

### Requirement: Synthesized characters share a stable pinyin baseline
The generator SHALL use one resolved font-wide pinyin baseline for all successfully synthesized readings while preserving each pinyin component's internal mark offsets.

#### Scenario: Adjacent Han glyphs have different outline tops
- **WHEN** adjacent synthesized characters have different Han `YMax` values
- **THEN** their pinyin base letters use the same baseline instead of following each Han outline top

#### Scenario: Pinyin contains descenders or stacked marks
- **WHEN** a reading contains a descending Latin component or multiple combining marks
- **THEN** its components preserve their calculated internal vertical relationship relative to the shared baseline

### Requirement: Scaled body consumers share the resolved vertical offset
The generator SHALL apply the same resolved Han-body vertical offset to synthesized Han components and scaled punctuation wrappers.

#### Scenario: Han and punctuation are generated from the same font
- **WHEN** a pinyin Han glyph and a whitelisted punctuation wrapper are successfully generated
- **THEN** their body components use the same scale and vertical offset derived from the selected typographic band

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
