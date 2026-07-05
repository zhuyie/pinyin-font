## Why

Generated pinyin fonts need a quick visual acceptance path after the CLI writes a `.ttf` file. The current `build/preview.html` is a useful manual prototype, but it is hard-coded to local build artifacts and is not a reusable project tool.

## What Changes

- Add a standalone browser preview page under `tools/` for visual validation of generated fonts.
- Let users load a generated pinyin font directly from the browser using a file picker.
- Let users optionally load the original source font for side-by-side comparison.
- Show browser font loading status so users can tell whether the selected font files were accepted.
- Provide adjustable sample rendering for custom text, single-glyph checks, long pinyin cases, and multiline text.
- Document the preview page as a development/validation tool separate from `pinyinfont` and `font_tool`.

## Capabilities

### New Capabilities
- `font-preview`: Browser-based visual validation for generated pinyin font files.

### Modified Capabilities
- None.

## Impact

- Affected files: `tools/preview.html`, `README.md`, and OpenSpec artifacts.
- Affected behavior: users gain a convenient visual verification workflow after generating a font.
- APIs: no C++ API or product CLI change.
- Dependencies: no external dependency; the page should use standard browser APIs such as `FontFace`, `URL.createObjectURL`, and `document.fonts`.
