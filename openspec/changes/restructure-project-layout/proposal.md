## Why

The project is evolving from a prototype-style font experiment into a CLI tool for producing pinyin-annotated fonts. The current layout mixes internal library code, development commands, tests, generated assets, and local build outputs, which makes the intended product shape harder to see and maintain.

## What Changes

- Reorganize source files into domain-oriented internal modules under `src/`:
  - `src/opentype/` for OpenType parsing, writing, cmap, glyph, and table logic.
  - `src/pinyin/` for pinyin data loading and normalization.
  - `src/synthesis/` for pinyin font synthesis and glyph composition.
- Add a dedicated `cli/` entry point for the user-facing command-line tool.
- Keep development-only utilities under `tools/`.
- Add a `tests/` area for real regression tests that can be run by CTest.
- Add a `data/` area for reproducible pinyin data fixtures or documented data inputs.
- Avoid introducing a public `include/pinyinfont/` API surface for now because the project is primarily a CLI tool, not an embeddable font parsing library.
- Keep GUI structure out of scope until GUI work is actually started.
- Add repository hygiene rules so generated build outputs, generated fonts, editor files, caches, and OS metadata do not appear as project structure.

## Capabilities

### New Capabilities
- `project-layout`: Defines the intended repository layout and boundaries for internal source code, CLI entry points, development tools, tests, data, and generated artifacts.

### Modified Capabilities

None.

## Impact

- Affected code: `CMakeLists.txt`, current `source/` files, current `tools/` files, and any new `cli/`, `src/`, `tests/`, and `data/` paths.
- Affected behavior: the existing build should continue to produce the internal `pinyinfont` target and runnable command-line tooling after files are moved.
- Testing impact: CTest should discover at least the project-level regression/smoke tests introduced by this change.
- Documentation impact: README should describe the CLI-oriented project layout and basic build/test usage at a concise level.
