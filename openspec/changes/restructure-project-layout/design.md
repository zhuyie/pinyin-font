## Context

The current repository is small and buildable, but it still reflects an early prototype layout. Production code lives in `source/`, the executable named `pinyinfont_test` is actually a multi-mode development CLI, test discovery is not configured for CTest, and reproducible data inputs currently appear under the local `build/` directory rather than a project-owned data boundary.

The project direction is CLI-first. The implementation should clarify the product entry point and internal engine structure without promising a reusable public C++ API or preparing GUI folders ahead of actual GUI work.

## Goals / Non-Goals

**Goals:**
- Make the repository structure communicate the CLI-tool product shape.
- Move internal implementation code into domain-oriented `src/` modules.
- Provide a dedicated `cli/` entry point for the runnable pinyin font tool.
- Keep development-only font table utilities in `tools/`.
- Establish a `tests/` area with at least one CTest-discoverable smoke/regression test.
- Establish a `data/` area or documented data contract so CLI/test inputs are not coupled to `build/data`.
- Add repository hygiene so generated outputs and local metadata stay out of the source tree view.
- Preserve existing buildable behavior while moving files.

**Non-Goals:**
- Do not create a public `include/pinyinfont/` API surface.
- Do not add GUI structure or GUI implementation.
- Do not redesign OpenType parsing, pinyin normalization, or font synthesis algorithms.
- Do not introduce new external dependencies solely for the layout migration.

## Decisions

1. Use `src/opentype/`, `src/pinyin/`, and `src/synthesis/` for internal implementation.

   This mirrors the current code responsibilities without over-abstracting them. `opentype` owns parsing, writing, cmap, glyph, and table concerns; `pinyin` owns pinyin records and normalization; `synthesis` owns the process of composing pinyin glyphs with base glyphs into a new font. Alternative considered: `src/build/`, but that conflicts semantically with the CMake build directory and would make documentation ambiguous.

2. Keep the library target internal.

   CMake can still build an internal static target to share implementation between CLI, tools, and tests, but headers do not need to move to a public include tree. Alternative considered: adding `include/pinyinfont/`; rejected because the project is not currently positioned as an embeddable font parsing library.

3. Promote the user-facing executable out of `source/test.cpp`.

   The current file contains useful command modes such as dump, bench, rewrite, and build, but its name and location hide that role. The migration should create a proper CLI entry point under `cli/`, with development-only or diagnostic commands either retained intentionally or split into `tools/` when appropriate.

4. Treat data as an explicit project input, not a build artifact.

   The CLI and tests should accept or locate pinyin data through a stable project-owned path or an explicit argument. Build outputs can copy data for convenience, but source behavior must not depend on manually populated `build/data`.

5. Add tests as smoke/regression coverage rather than a broad unit-test rewrite.

   The first goal is to make CTest meaningful and guard the migration. Suitable tests include parsing/writing a known test font if a small fixture is available, loading a small pinyin fixture, or exercising the CLI on a minimal fixture. Large font assets should not be added without an intentional decision.

## Risks / Trade-offs

- File moves can make diffs noisy → Keep changes mechanical first and avoid algorithm edits in the same pass.
- Existing local build data may hide missing project data → Verify from a clean or freshly configured build directory after migration.
- Font fixtures may be large or licensing-sensitive → Prefer tiny generated/open fixtures, minimal pinyin text fixtures, or tests that do not require bundling proprietary fonts.
- Renaming `pinyinfont_test` may disrupt ad hoc local commands → Document the new command name and keep equivalent modes where they are still useful.
- A CLI-first layout may still need future GUI reuse → Keep internal modules reusable through CMake targets, but defer GUI directories and UX choices until that work starts.

## Migration Plan

1. Add repository hygiene for generated outputs and local metadata.
2. Move current source files into `src/opentype/`, `src/pinyin/`, and `src/synthesis/` according to responsibility.
3. Move or rewrite the current CLI-like `source/test.cpp` into `cli/` with a product-appropriate executable name.
4. Update `CMakeLists.txt` include paths, targets, and executable names.
5. Add a `tests/` target registered with CTest for at least one migration smoke/regression check.
6. Establish `data/` fixtures or documented data input behavior and remove dependence on `build/data`.
7. Update README with concise build, test, and CLI usage.
8. Verify with a fresh CMake configure/build and `ctest --output-on-failure`.

Rollback is straightforward because the change is mostly file movement and build wiring: revert the migration commit or restore the previous `source/` layout and CMake target definitions.

## Open Questions

- What final CLI executable name should be used: `pinyinfont`, `pinyin-font`, or another name?
- Should diagnostic modes like `dump`, `bench`, and `rewrite` remain in the main CLI or move to separate tools?
- Which font fixture, if any, is acceptable to commit for regression tests?
