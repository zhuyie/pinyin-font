## ADDED Requirements

### Requirement: Polyphonic readings have stable one-based selectors
The generator SHALL interpret `@1` through `@4` immediately following an eligible polyphonic Han character as one-based selectors for that character's non-empty pinyin readings in database order, and SHALL continue to use the first reading when the character has no selector.

#### Scenario: Default reading remains first
- **WHEN** a polyphonic character is rendered without a selector
- **THEN** its generated annotated glyph uses the first pinyin reading stored for that character

#### Scenario: Alternate reading is selected
- **WHEN** `藏` has the ordered readings `cáng,zàng` and the shaped input is `藏@2`
- **THEN** the visible annotated Han glyph uses `zàng`

#### Scenario: First reading is selected explicitly
- **WHEN** a polyphonic character with a successfully generated default annotation is followed by `@1`
- **THEN** the visible result is the same annotated Han glyph as the unmarked character and the selector is consumed

### Requirement: Valid selectors are standard OpenType substitutions
The generated font SHALL encode supported reading selectors as GSUB Ligature Substitution rules in the standard `liga` feature, available through the `DFLT` and `hani` default language systems.

#### Scenario: Shaping consumes a valid selector
- **WHEN** an OpenType shaping engine applies standard ligatures to a supported `汉字@序号` sequence
- **THEN** it replaces the three input glyphs with the single annotated glyph for the selected reading

#### Scenario: Selected glyph remains internal
- **WHEN** an alternate annotated reading glyph is generated for a selector rule
- **THEN** that glyph is referenced by GSUB without receiving an additional Unicode `cmap` mapping

#### Scenario: Generated GSUB is structurally valid
- **WHEN** at least one selector rule is generated
- **THEN** the output font contains a valid GSUB table with script, feature, lookup, coverage, ligature-set, and ligature records whose glyph references and offsets are in range

### Requirement: Invalid and unsupported selectors remain literal
The generator SHALL NOT emit a substitution for an empty, out-of-range, unsynthesizable, or otherwise unsupported reading selector.

#### Scenario: Candidate index is out of range
- **WHEN** `藏` has two readings and the input is `藏@4`
- **THEN** no selector ligature applies and the literal `@4` remains visible

#### Scenario: Character is not polyphonic
- **WHEN** a character with only one stored reading is followed by `@1`
- **THEN** no selector ligature applies and the literal `@1` remains visible

#### Scenario: Alternate annotation cannot be synthesized
- **WHEN** the components required for a selected alternate reading cannot be synthesized safely
- **THEN** the generator omits only that selector rule, preserves the default character mapping, and continues the font build

#### Scenario: Selector input glyph is missing
- **WHEN** the output font does not map `@` or the ASCII digit required by a selector
- **THEN** the generator omits affected selector rules and continues the font build

### Requirement: Selector generation is auditable
The font-generation diagnostics SHALL report the number of alternate annotated glyphs and selector ligatures generated and SHALL distinguish selector omissions caused by missing input glyphs from alternate-reading synthesis failures.

#### Scenario: Successful selector generation is reported
- **WHEN** a polyphonic record produces one or more valid selector ligatures
- **THEN** diagnostics include those generated alternate glyphs and ligatures in the selector totals

#### Scenario: Selector omission is classified
- **WHEN** a selector rule is omitted
- **THEN** diagnostics identify whether the cause was a missing selector input glyph or a failed alternate-reading synthesis

### Requirement: Existing unmarked rendering remains compatible
Adding polyphonic selectors SHALL preserve the existing `cmap` behavior and annotated rendering of text that does not contain a valid selector sequence.

#### Scenario: Ordinary digits remain ordinary text
- **WHEN** `@` and an ASCII digit do not immediately follow an eligible polyphonic Han glyph as a valid selector
- **THEN** their original glyph mappings and visible rendering are unchanged

#### Scenario: Shaping engine does not apply standard ligatures
- **WHEN** a renderer performs `cmap` mapping but does not apply the generated `liga` feature
- **THEN** the annotated default Han glyph and literal `@序号` remain renderable without corrupting the font or text
