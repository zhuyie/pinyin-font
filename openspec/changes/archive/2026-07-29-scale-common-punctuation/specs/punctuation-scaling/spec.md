## ADDED Requirements

### Requirement: Common punctuation uses the Han body scale
The pinyin font generator SHALL replace mapped whitelist punctuation with composite glyphs that use the same scale and placement logic as synthesized Han glyph bodies.

#### Scenario: Half-width punctuation is scaled
- **WHEN** the source font maps a whitelisted half-width punctuation character with an outline
- **THEN** the generated cmap maps that character to a composite containing the source glyph at the Han body scale and offsets

#### Scenario: Full-width punctuation is scaled
- **WHEN** the source font maps a whitelisted full-width or Chinese punctuation character with an outline
- **THEN** the generated cmap maps that character to a composite containing the source glyph at the Han body scale and offsets

#### Scenario: Multi-character punctuation is composed naturally
- **WHEN** text contains repeated em-dash or ellipsis characters
- **THEN** each mapped character uses its individually scaled glyph without requiring a dedicated multi-character glyph

### Requirement: Punctuation scaling uses an explicit whitelist
The pinyin font generator SHALL scale only the agreed common punctuation code points and SHALL NOT infer eligibility from an entire Unicode block.

#### Scenario: Common punctuation is eligible
- **WHEN** a mapped character is one of `! " ' ( ) , - . : ; ? [ ] { }`, its agreed full-width counterpart, or one of `、 。 〈 〉 《 》 「 」 『 』 【 】 〔 〕 “ ” ‘ ’ – — … · ・`
- **THEN** the generator treats the character as eligible for punctuation scaling

#### Scenario: Technical symbols remain unchanged
- **WHEN** the source font maps `/`, `\`, `_`, or another symbol outside the whitelist
- **THEN** the generated cmap preserves that character's source glyph mapping

### Requirement: Scaled punctuation preserves horizontal layout
The pinyin font generator SHALL preserve the source punctuation glyph's advance width while updating its side bearing and bounds to describe the transformed outline.

#### Scenario: Advance width is preserved
- **WHEN** a punctuation wrapper is generated successfully
- **THEN** its advance width equals the source punctuation glyph's advance width

#### Scenario: Placement matches the Han body
- **WHEN** a punctuation wrapper is generated successfully
- **THEN** its horizontal and vertical component offsets are calculated with the same formula and values used for a synthesized Han body

### Requirement: Punctuation scaling fails closed
The pinyin font generator SHALL keep the source cmap mapping when eligible punctuation cannot be safely wrapped.

#### Scenario: Source punctuation is absent
- **WHEN** a whitelist code point has no source cmap mapping
- **THEN** the generator does not create a mapping for it

#### Scenario: Source punctuation has no outline
- **WHEN** a whitelist code point maps to a glyph without an outline
- **THEN** the generated cmap retains the source glyph mapping

#### Scenario: Wrapper creation fails
- **WHEN** a scaled punctuation composite cannot be appended safely
- **THEN** the generated cmap retains the source glyph mapping and the font build continues

### Requirement: Punctuation scaling is scoped to horizontal layout
The pinyin font generator SHALL apply punctuation wrappers to the horizontal cmap behavior without claiming vertical-layout substitutions.

#### Scenario: Horizontal punctuation is generated
- **WHEN** a whitelisted punctuation character is rendered through its cmap mapping
- **THEN** the generated horizontal glyph uses the scaled wrapper

