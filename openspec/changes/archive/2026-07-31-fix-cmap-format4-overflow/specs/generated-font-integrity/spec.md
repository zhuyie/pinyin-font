## ADDED Requirements

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
