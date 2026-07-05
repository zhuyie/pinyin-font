## Context

The repository now has a CLI-first layout, but the main `pinyinfont` executable still exposes a mode-based developer menu: `build`, `dump`, `bench`, and `rewrite`. At the same time, `tools/font_table_tool.cpp` exposes related low-level diagnostics for dumping and purging OpenType tables.

This leaves two unclear boundaries:
- The product CLI mixes the actual user workflow with parser/writer diagnostics.
- The pinyin database is treated as an optional positional argument even though it is a required input for deterministic output.

## Goals / Non-Goals

**Goals:**
- Make `pinyinfont` a single-purpose product CLI for generating pinyin-annotated fonts.
- Use explicit long options for the product CLI.
- Require `--pinyin-db` instead of relying on an implicit default.
- Keep `--output` optional, preserving the existing `<input>.pinyin.ttf` default when omitted.
- Move font diagnostics out of the product CLI and into one unified developer tool.
- Merge the current `font_table_tool` behavior into that unified tool.
- Update docs and verification notes to encode the new CLI/tool boundary.

**Non-Goals:**
- Do not change the font synthesis algorithm.
- Do not change the pinyin database file format.
- Do not introduce a third-party CLI parsing dependency.
- Do not add GUI behavior.

## Decisions

1. Use explicit long options for `pinyinfont`.

   The product command will use `pinyinfont --input <font.ttf> --pinyin-db <pinyin-db.txt> [--output <out.ttf>]`. This is clearer than positional arguments and gives room for future options such as layout ratios or tone style without breaking argument ordering.

2. Make `--pinyin-db` required.

   The pinyin database controls what glyphs are synthesized and should be visible as a required input. The existing project-owned `data/TGHZ2013.txt` remains available for examples and tests, but the CLI should not silently choose it for users.

3. Keep `--output` optional.

   Defaulting to `<input>.pinyin.ttf` preserves the current output behavior and keeps the common command short, while allowing users and tests to choose explicit output paths when needed.

4. Move diagnostics into `font_tool`.

   Developer commands should live under a unified `tools/font_tool.cpp` executable with explicit subcommands and options:
   - `info --input <font.ttf>`
   - `bench-parse --input <font-dir>`
   - `rewrite --input <font.ttf> [--output <out.ttf>]`
   - `table-dump --input <font.ttf> --table <tag> [--output <file.dat>]`
   - `table-purge --input <font.ttf> --table <tag> [--output <out.ttf>]`

   This replaces the existing `font_table_tool` executable and removes `dump`, `bench`, and `rewrite` from `pinyinfont`.

5. Keep parsing simple and local.

   The option grammar is small enough to implement with local argument parsing. A new dependency would be disproportionate for this change.

## Risks / Trade-offs

- Existing ad hoc commands break → Update README and keep tool command names explicit enough to translate old usage.
- Local parser code can become messy → Keep parsing small and command-specific; revisit only if option count grows.
- Moving code from CLI to tools can create duplication → Share the internal `pinyinfont` library target and move only CLI/tool orchestration code.
- `font_tool` becomes broad → Use specific subcommand names (`bench-parse`, `table-dump`) so modes remain understandable.

## Migration Plan

1. Reduce `cli/pinyinfont.cpp` to the generation path and explicit option parsing.
2. Add optional output path support to the generation flow if needed by the CLI.
3. Create `tools/font_tool.cpp` by combining the current CLI diagnostics and table tool behavior.
4. Replace the `font_table_tool` CMake target with `font_tool`.
5. Keep existing CTest smoke coverage for stable internal behavior and verify CLI/tool command contracts with manual smoke commands.
6. Update README to present product CLI usage separately from developer tool usage.
7. Verify fresh configure/build, CTest, `pinyinfont` smoke, and `font_tool` smoke.

## Open Questions

None.
