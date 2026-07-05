## 1. Product CLI

- [x] 1.1 Reduce `cli/pinyinfont.cpp` to the pinyin font generation workflow only.
- [x] 1.2 Replace mode-based parsing with explicit long options: `--input`, `--pinyin-db`, and optional `--output`.
- [x] 1.3 Make `--input` and `--pinyin-db` required and return a clear usage error when either is missing.
- [x] 1.4 Preserve default output naming as `<input>.pinyin.ttf` when `--output` is omitted.
- [x] 1.5 Ensure `build`, `dump`, `bench`, and `rewrite` are no longer accepted by the product CLI.

## 2. Unified Font Tool

- [x] 2.1 Create `tools/font_tool.cpp` as the unified developer tool.
- [x] 2.2 Move the current CLI `dump` behavior to `font_tool info --input <font.ttf>`.
- [x] 2.3 Move the current CLI `bench` behavior to `font_tool bench-parse --input <font-directory>`.
- [x] 2.4 Move the current CLI `rewrite` behavior to `font_tool rewrite --input <font.ttf> [--output <out.ttf>]`.
- [x] 2.5 Merge current `font_table_tool` dump behavior into `font_tool table-dump --input <font.ttf> --table <tag> [--output <file.dat>]`.
- [x] 2.6 Merge current `font_table_tool` purge behavior into `font_tool table-purge --input <font.ttf> --table <tag> [--output <out.ttf>]`.
- [x] 2.7 Remove or replace the separate `font_table_tool` target so only `font_tool` exposes these diagnostics.

## 3. Build and Tests

- [x] 3.1 Update `CMakeLists.txt` to build the simplified `pinyinfont` CLI and unified `font_tool`.
- [x] 3.2 Keep CTest focused on stable library smoke coverage rather than CLI argument-shape checks.
- [x] 3.3 Cover `pinyinfont --input ... --pinyin-db ...` through manual verification.
- [x] 3.4 Cover `font_tool` diagnostic commands through manual verification.

## 4. Documentation

- [x] 4.1 Update README primary usage to `pinyinfont --input <font.ttf> --pinyin-db <pinyin-db.txt> [--output <out.ttf>]`.
- [x] 4.2 Remove mode-based `pinyinfont build/dump/bench/rewrite` examples from README.
- [x] 4.3 Document `font_tool` commands separately as development utilities.

## 5. Verification

- [x] 5.1 Run a fresh CMake configure into a clean build directory.
- [x] 5.2 Run `cmake --build <build-dir>`.
- [x] 5.3 Run `ctest --test-dir <build-dir> --output-on-failure`.
- [x] 5.4 Run product CLI smoke with explicit `--input` and `--pinyin-db`.
- [x] 5.5 Run `font_tool` smoke for at least `info` and one table operation.
