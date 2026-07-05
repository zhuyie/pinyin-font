## 1. Preview Page Structure

- [x] 1.1 Add tracked `tools/preview.html` as a standalone browser preview page.
- [x] 1.2 Base the preview content on the useful checks from `build/preview.html` without depending on build-directory files.
- [x] 1.3 Add file inputs for the generated pinyin font and optional original font.
- [x] 1.4 Keep the page self-contained with no external runtime dependencies.

## 2. Font Loading

- [x] 2.1 Load the selected pinyin font with browser `FontFace` and an object URL.
- [x] 2.2 Load the optional original font with the same browser APIs.
- [x] 2.3 Display clear loaded, missing, and failed status for each font.
- [x] 2.4 Revoke replaced object URLs when users select different font files.

## 3. Visual Preview Controls

- [x] 3.1 Add editable custom preview text with a useful default sample.
- [x] 3.2 Add a font-size control that updates the main preview.
- [x] 3.3 Show pinyin-font rendering even when no original font is selected.
- [x] 3.4 Show side-by-side pinyin/original comparison when both fonts are selected.
- [x] 3.5 Include representative single-glyph samples for long pinyin and tone mark placement.
- [x] 3.6 Include multiline sample text for line height, clipping, and layout stability checks.

## 4. Documentation

- [x] 4.1 Document `tools/preview.html` in README as the visual validation tool.
- [x] 4.2 Explain the manual workflow: generate a font, open the preview page, select the pinyin font, and optionally select the original font.
- [x] 4.3 Keep the preview documentation separate from `pinyinfont` CLI and `font_tool` diagnostics.

## 5. Verification

- [x] 5.1 Open the preview page locally and verify the page renders without a build step.
- [x] 5.2 Select a generated pinyin font and confirm the pinyin preview uses it.
- [x] 5.3 Select an original font and confirm side-by-side comparison appears.
- [x] 5.4 Exercise custom text and font-size controls.
- [x] 5.5 Run `openspec validate add-font-preview-page --strict`.
