## Context

The project is CLI-first: `pinyinfont` generates the output font, while `font_tool` covers command-line diagnostics. A visual preview is different from both. It is a browser interaction tool for checking whether a generated font loads and whether pinyin annotations render acceptably.

There is an existing prototype at `build/preview.html`, but `build/` is ignored and the page hard-codes specific local font names. The durable version should live in source control and load user-selected files at runtime.

## Goals / Non-Goals

**Goals:**
- Provide a reusable preview page for generated pinyin fonts.
- Let users choose the pinyin font file directly in the browser.
- Let users optionally choose the original font file for side-by-side comparison.
- Show clear load status for selected fonts.
- Support common visual checks: custom text, font size, single glyphs, long pinyin strings, and multiline layout.
- Keep the preview tool independent from the `pinyinfont` product CLI and `font_tool` diagnostics.

**Non-Goals:**
- Do not add a GUI application or desktop shell.
- Do not add a web server requirement.
- Do not change the font generation algorithm.
- Do not parse font internals in JavaScript.
- Do not add automated browser tests for visual appearance.

## Decisions

1. Store the preview page as `tools/preview.html`.

   A single tracked HTML file is enough for the current workflow and avoids a new build target. If the preview grows assets later, it can move to `tools/preview/index.html` in a separate change.

2. Use browser file pickers instead of generating an HTML file from `font_tool`.

   The CLI already produces the font file. Loading local files directly in the browser makes the validation step repeatable without path rewriting, generated HTML output, or another command-line mode.

3. Use `FontFace` plus object URLs for selected fonts.

   This lets the page load arbitrary local `.ttf` files selected by the user. The page should revoke previous object URLs when a new file is selected to avoid leaking object URLs during repeated comparisons.

4. Make the original font optional.

   The generated pinyin font is the minimum artifact to validate. If an original font is selected, the page should show side-by-side comparison; otherwise it should still provide a useful single-font preview.

5. Keep visual validation manual.

   Browser rendering and visual quality are best judged by inspecting representative samples. CTest should remain focused on stable internal smoke coverage rather than screenshots or pixel assertions.

## Risks / Trade-offs

- Browser local-file limitations -> Use file inputs and object URLs instead of relying on relative `file://` paths.
- Font load failure can look like fallback rendering -> Display explicit load status for each selected font.
- Visual sample coverage may miss edge cases -> Include editable text so users can paste problematic strings.
- Single-file HTML can become bulky -> Keep the first version self-contained and revisit structure only if assets or complexity grow.

## Migration Plan

1. Create `tools/preview.html` based on the useful parts of `build/preview.html`.
2. Replace hard-coded font URLs with file inputs and runtime `FontFace` loading.
3. Add load status indicators for pinyin and original fonts.
4. Preserve useful preview sections: adjustable custom text, side-by-side comparison, glyph grid, long pinyin cases, and multiline samples.
5. Document the page in README as a manual validation tool.
6. Verify by opening the page and loading a generated pinyin font plus an optional original font.

## Open Questions

None.
