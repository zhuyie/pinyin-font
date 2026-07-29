## 1. Test Baseline and Coverage Accounting

- [x] 1.1 Add focused test support that can build or load a minimal TrueType fixture with configurable Han, Latin, tone-mark, and simple-`i` coverage without depending on ignored local fonts.
- [x] 1.2 Add a baseline synthesis test that distinguishes successful generation, missing source Han glyphs, and missing pinyin components while confirming failed synthesis preserves the source cmap mapping.
- [x] 1.3 Record the current ZCOOL sample coverage categories in a repeatable development diagnostic so component fallback improvements can be compared against the 3,842 success / 1,271 source-missing / 2,796 component-failure baseline.

## 2. Internal Simple-Glyph Support

- [x] 2.1 Add internal helpers for constructing simple TrueType contours, calculating glyph and contour-group bounds, and assigning safe horizontal metrics.
- [x] 2.2 Update `OpenType_Font::AddGlyph()` so appended simple glyphs maintain `maxp.MaxPoints`, `maxp.MaxContours`, glyph counts, horizontal metrics, and global bounds.
- [x] 2.3 Add unit tests that serialize and reparse generated simple glyphs and verify outline points, contour endpoints, empty instructions, metrics, and `maxp` metadata.

## 3. Source-Relative Tone Components

- [x] 3.1 Implement x-height resolution from valid `OS/2.sxHeight`, representative lowercase glyph measurements, and a bounded units-per-em fallback.
- [x] 3.2 Implement procedural TrueType outlines for macron, acute, caron, grave, and diaeresis using per-mark dimensions derived from the resolved x-height.
- [x] 3.3 Add a lazy internal component registry that prefers source glyphs, creates each missing tone component at most once, gives it an internal glyph name, and leaves cmap unchanged.
- [x] 3.4 Update mark positioning to use the carrying vowel's actual bbox center and `YMax`, and stack multiple marks from transformed component bounds and font-relative spacing.
- [x] 3.5 Add tests for source-mark preference, generated-mark fallback, component reuse, cmap invisibility, and non-overlapping one- and two-mark placement.

## 4. Derived Dotless i

- [x] 4.1 Implement contour-bound grouping for simple glyphs so overlapping or containing contours such as a hollow dot are treated as one visual group.
- [x] 4.2 Implement conservative dot recognition using vertical separation, horizontal alignment, size, and area bounds relative to the source `i` body and resolved x-height.
- [x] 4.3 Build and cache an internal dotless-`i` glyph by copying non-dot contours, recomputing contour endpoints and bounds, preserving source advance width, and clearing source instructions.
- [x] 4.4 Replace unconditional accented-`i` substitution with source-aware resolution that prefers existing `ī/í/ǐ/ì` and otherwise uses derived dotless `i` plus the resolved tone component.
- [x] 4.5 Add tests for a separable simple `i`, a hollow multi-contour dot, source precomposed-`i` preference, ambiguous or connected contours, and unsupported composite `i`.

## 5. Normalization and Synthesis Integration

- [x] 5.1 Extend pinyin normalization so `ḿ` and supported accented `n` forms decompose into ASCII base letters plus combining tone marks.
- [x] 5.2 Route all remaining mark lookup through the source-first component resolver without changing base-Han lookup or source-cmap overlay semantics.
- [x] 5.3 Track whether each generated character used source-only glyphs, generated tone components, or derived dotless `i`, and preserve a specific failure category when synthesis cannot continue safely.
- [x] 5.4 Add integration tests covering first through fourth tones, diaeresis plus tone, accented `i`, exceptional syllabic forms, and source mapping preservation on conservative fallback failure.

## 6. Diagnostics and Visual Validation

- [x] 6.1 Extend development diagnostics to report generated internal component kinds, reuse counts, source-Han absence, dotless-`i` derivation failure, and other component failures without adding noisy per-glyph product CLI output.
- [x] 6.2 Extend `font_tool integrity` checks for appended internal simple-glyph metadata and confirm internal components introduce no unexpected cmap mappings.
- [x] 6.3 Update `tools/preview.html` samples to cover every generated mark, accented `i`, diaeresis-plus-tone stacking, and representative 24px, 48px, 72px, and 120px inspection.

## 7. End-to-End Verification

- [x] 7.1 Run a fresh CMake build, CTest suite, `git diff --check`, and `openspec validate --all --strict`.
- [x] 7.2 Generate a font from `ZCOOLXiaoWei-Regular.ttf` and `data/TGHZ2013.txt`, compare coverage categories against the baseline, and verify component failures are eliminated or explicitly explained.
- [x] 7.4 Inspect the generated font in `tools/preview.html` across representative text and sizes, documenting any decorative-font limitations or dotless-`i` cases that correctly fail closed.
