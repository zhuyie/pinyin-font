## ADDED Requirements

### Requirement: Pinyin synthesis prefers source-font glyphs
The generator SHALL prefer usable source-font glyphs over generated fallback components.

#### Scenario: Source precomposed vowel is available
- **WHEN** the source font contains the precomposed accented vowel required by a pinyin cluster
- **THEN** the generator uses that source glyph without generating an equivalent fallback glyph

#### Scenario: Source tone mark is available
- **WHEN** a pinyin cluster cannot use a precomposed vowel but the source font contains a usable tone-mark glyph
- **THEN** the generator uses the source tone-mark glyph

### Requirement: Missing tone marks have internal TrueType fallbacks
For a source font with TrueType `glyf`/`loca` outlines, the generator SHALL be able to create internal glyphs for macron, acute, caron, grave, and diaeresis when required source glyphs are unavailable.

#### Scenario: Required tone mark is absent
- **WHEN** synthesis requires one of the supported tone marks and neither a source combining mark nor its usable source fallback is available
- **THEN** the generator creates or reuses the corresponding internal tone-mark glyph and continues synthesis

#### Scenario: Internal tone mark is reused
- **WHEN** more than one synthesized character requires the same generated tone mark
- **THEN** the generated font contains one reusable internal glyph for that mark rather than one duplicate per synthesized character

#### Scenario: Internal glyph is not exposed through cmap
- **WHEN** an internal tone-mark glyph is appended to the generated font
- **THEN** it is referenced by generated composite glyphs without adding a new character mapping to the source-cmap baseline

### Requirement: Fallback components adapt to the source font
The generator SHALL size fallback tone marks from source-font metrics and position them from the actual bounds of the vowel that carries the mark.

#### Scenario: Source x-height is usable
- **WHEN** the source font provides a valid `OS/2.sxHeight`
- **THEN** generated tone-mark dimensions and spacing are derived from that x-height

#### Scenario: Source x-height is unavailable
- **WHEN** `OS/2.sxHeight` is absent or invalid
- **THEN** the generator estimates lowercase height from available representative source glyph bounds and uses a bounded units-per-em fallback only when measurement is insufficient

#### Scenario: Tone mark is positioned above a vowel
- **WHEN** a generated or source tone mark is appended to a vowel cluster
- **THEN** its horizontal center is aligned to the vowel bounds and its lower bound is placed above the vowel's actual `YMax` using font-relative spacing

#### Scenario: Two marks are stacked
- **WHEN** a pinyin cluster requires diaeresis and a tone mark as separate components
- **THEN** the marks are vertically stacked using their transformed bounds and font-relative spacing without overlap

### Requirement: Accented i can use a derived dotless i
The generator SHALL derive an internal dotless-`i` from a reliably separable simple source `i` glyph when a required precomposed accented `i` glyph is unavailable.

#### Scenario: Source accented i is available
- **WHEN** the source font contains the required `ī`, `í`, `ǐ`, or `ì` glyph
- **THEN** the generator uses the source precomposed glyph and does not derive dotless `i` for that cluster

#### Scenario: Source i has a separable dot
- **WHEN** the source `i` is a simple TrueType glyph whose upper dot contour group can be identified unambiguously
- **THEN** the generator copies the remaining contours into one reusable internal dotless-`i` glyph and combines it with the required tone mark

#### Scenario: Derived outline drops source hinting
- **WHEN** dot contours are removed from the source `i`
- **THEN** the derived glyph does not retain instructions that reference the source glyph's original point indexes

#### Scenario: Dot separation is ambiguous
- **WHEN** the source `i` is composite, connected, or has no uniquely identifiable dot contour group
- **THEN** the generator does not guess or remove contours and reports dotless-`i` derivation as unavailable

### Requirement: Exceptional accented forms are decomposed
The pinyin database normalization path SHALL decompose supported exceptional accented syllabic forms into base letters and tone marks that can use the component fallback path.

#### Scenario: Accented syllabic m is loaded
- **WHEN** a pinyin reading contains `ḿ`
- **THEN** normalization produces `m` followed by an acute tone mark

#### Scenario: Accented syllabic n is loaded
- **WHEN** a pinyin reading contains an accented `n` form such as `ń`
- **THEN** normalization produces `n` followed by the corresponding tone mark

### Requirement: Component fallback is limited to TrueType outlines
The generator SHALL keep CFF and CFF2 outline support outside this capability.

#### Scenario: Input font uses CFF outlines
- **WHEN** the input OpenType font contains CFF or CFF2 outlines instead of `glyf`/`loca`
- **THEN** the generator returns an explicit unsupported-outline result without attempting component generation
