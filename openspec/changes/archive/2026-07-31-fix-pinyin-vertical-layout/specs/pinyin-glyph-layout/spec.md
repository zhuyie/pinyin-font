## ADDED Requirements

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
