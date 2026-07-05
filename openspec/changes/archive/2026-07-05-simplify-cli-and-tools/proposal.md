## Why

The main `pinyinfont` command should behave like a product CLI for one job: generating a pinyin-annotated font. The current mode-based command mixes user-facing generation with developer diagnostics, and the implicit pinyin database default hides a required input that materially affects output.

## What Changes

- **BREAKING**: Replace `pinyinfont build <font.ttf> [pinyin-db]` with explicit options:
  - `pinyinfont --input <font.ttf> --pinyin-db <pinyin-db.txt> [--output <out.ttf>]`
- **BREAKING**: Remove `dump`, `bench`, and `rewrite` modes from the main `pinyinfont` CLI.
- Make `--pinyin-db` required for the main CLI.
- Make `--output` optional; if omitted, preserve the existing `<input>.pinyin.ttf` output naming behavior.
- Merge developer/diagnostic commands into a single tools executable, replacing the current `font_table_tool` boundary:
  - `font_tool info --input <font.ttf>`
  - `font_tool bench-parse --input <font-dir>`
  - `font_tool rewrite --input <font.ttf> [--output <out.ttf>]`
  - `font_tool table-dump --input <font.ttf> --table <tag> [--output <file.dat>]`
  - `font_tool table-purge --input <font.ttf> --table <tag> [--output <out.ttf>]`
- Update README, CMake targets, and verification notes to reflect the split between product CLI and developer tool.

## Capabilities

### New Capabilities

None.

### Modified Capabilities
- `project-layout`: Clarify that the CLI entry point exposes only the pinyin font generation command, while diagnostic font inspection, benchmarking, rewrite, and table operations belong under a unified development tool in `tools/`.

## Impact

- Affected code: `cli/pinyinfont.cpp`, `tools/font_table_tool.cpp` or replacement tool source, `CMakeLists.txt`, tests, and README.
- Affected behavior: existing mode-based invocations are intentionally replaced by explicit option-based commands.
- Testing impact: CTest remains focused on stable internal smoke coverage; implementation verification should run manual product CLI and `font_tool` smoke commands.
- Documentation impact: README should present `pinyinfont --input ... --pinyin-db ...` as the primary usage and list `font_tool` commands separately as development utilities.
