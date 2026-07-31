## 1. Format 4 Planning

- [x] 1.1 Add a format 4 segment-plan representation covering delta segments, glyph-array segments, and the required `U+FFFF` sentinel
- [x] 1.2 Build BMP mappings from the canonical cmap groups and choose a compact mix of delta and glyph-array segments
- [x] 1.3 Calculate and validate format 4 length, segment count, search parameters, and glyph-array offsets before serialization

## 2. Cmap Serialization

- [x] 2.1 Refactor the cmap writer to serialize format 4 from the checked segment plan without narrowing unchecked values
- [x] 2.2 Return an explicit generation failure when the complete BMP mapping cannot fit in a valid format 4 subtable
- [x] 2.3 Keep the emitted format 12 mapping unchanged and ensure its BMP mappings match the planned format 4 mapping

## 3. Integrity Validation

- [x] 3.1 Add a diagnostic path that enumerates and bounds-checks every supported Unicode cmap subtable
- [x] 3.2 Compare shared BMP mappings across emitted format 4 and format 12 subtables and report disagreements
- [x] 3.3 Report malformed sibling subtables even when the preferred format 12 subtable parses successfully

## 4. Regression Coverage

- [x] 4.1 Add unit tests for delta and glyph-array format 4 round trips, including sparse mappings and zero-filled gaps
- [x] 4.2 Add boundary tests for the largest legal format 4 representation and explicit failure immediately beyond the limit
- [x] 4.3 Add a regression case whose uncompressed delta segments exceed the limit but whose compact representation fits
- [x] 4.4 Generate a static Noto CJK test font with the complete local pinyin database when fixtures are available and validate every cmap subtable
- [x] 4.5 Run the complete test suite and manually confirm the generated static Noto Sans and Serif fonts load correctly in the browser preview
