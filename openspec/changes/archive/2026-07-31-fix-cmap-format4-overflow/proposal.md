## Why

Generating a pinyin font from a full Chinese reading database can produce more BMP cmap groups than the current format 4 writer can represent in its 16-bit length field. The writer silently truncates that length, so otherwise valid Noto CJK output is rejected by browsers even though the format 12 subtable remains readable.

## What Changes

- Encode BMP mappings into a size-aware format 4 representation that can use both delta segments and glyph-ID array segments.
- Reject or deliberately omit a format 4 subtable when the complete, consistent BMP mapping cannot be represented within the format's limits; never emit a truncated subtable.
- Keep BMP mappings consistent across emitted format 4 and format 12 subtables.
- Extend generated-font diagnostics and tests to validate every emitted cmap subtable, including near-limit and overflow cases.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `generated-font-integrity`: Require structurally valid cmap serialization, consistent BMP mappings across emitted Unicode subtables, and explicit handling when format 4 cannot represent the complete mapping.

## Impact

- Affects OpenType cmap serialization in `src/opentype/ot_font_writer.cpp` and supporting cmap representation or helpers.
- Affects cmap parsing or integrity diagnostics so validation covers every emitted subtable rather than only the preferred subtable.
- Adds regression coverage for large static CJK fonts generated with the complete pinyin database.
- Does not change the command-line interface or pinyin database format.
