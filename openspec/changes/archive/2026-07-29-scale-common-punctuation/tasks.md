## 1. Punctuation Selection and Fixtures

- [x] 1.1 Add a centralized exact-code-point whitelist for the agreed half-width, full-width, Chinese, and typographic punctuation, explicitly excluding `/`, `\`, and `_`.
- [x] 1.2 Extend the test font fixture with representative simple, composite, empty-outline, whitelisted, and non-whitelisted punctuation mappings.

## 2. Scaled Punctuation Synthesis

- [x] 2.1 Add a punctuation synthesis pass after source-cmap retention that wraps eligible outlined glyphs in one-component composites.
- [x] 2.2 Reuse the Han body scale, horizontal-offset formula, and `baseDY_`, while preserving source advance widths and updating transformed bounds and LSB.
- [x] 2.3 Reuse generated wrappers for characters sharing a source glyph and preserve original cmap mappings when wrapping is skipped or fails.
- [x] 2.4 Keep punctuation additions separate from pinyin database synthesis counters and failure classifications.

## 3. Composite and Diagnostic Integrity

- [x] 3.1 Calculate cycle-safe reachable composite depth when appending nested composites and update `maxp.MaxComponentDepth`.
- [x] 3.2 Teach the integrity diagnostic to recognize valid scaled-punctuation replacements while continuing to report unexplained cmap changes.

## 4. Verification

- [x] 4.1 Add table-driven tests proving every whitelist group is scaled with the expected transform and that excluded technical symbols retain their source mappings.
- [x] 4.2 Add tests proving advance widths are unchanged, offsets match Han-body logic, repeated punctuation needs no special ligature, and empty or absent glyphs fail closed.
- [x] 4.3 Add a nested-composite punctuation test that verifies serialized cmap, bounds, metrics, and `maxp.MaxComponentDepth` after reparsing.
- [x] 4.4 Run the full CMake/CTest suite and audit a representative generated font with the development font tool.
