## ADDED Requirements

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
