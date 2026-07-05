## 1. Repository Hygiene

- [x] 1.1 Add ignore rules for build directories, generated font outputs, editor metadata, caches, and `.DS_Store`.
- [x] 1.2 Confirm `git status --short` no longer presents local generated outputs as reviewable project structure.

## 2. Source Layout Migration

- [x] 2.1 Create `src/opentype/`, `src/pinyin/`, `src/synthesis/`, `cli/`, `tests/`, and `data/` as needed.
- [x] 2.2 Move OpenType-related implementation and headers from `source/` into `src/opentype/`.
- [x] 2.3 Move pinyin database implementation and headers from `source/` into `src/pinyin/`.
- [x] 2.4 Move pinyin font synthesis implementation and headers from `source/` into `src/synthesis/`.
- [x] 2.5 Move the current CLI-like `source/test.cpp` into `cli/` and rename the executable target away from test-oriented naming.
- [x] 2.6 Keep `tools/font_table_tool.cpp` under `tools/` and update its include paths for the new internal source layout.

## 3. Build Configuration

- [x] 3.1 Update `CMakeLists.txt` to build the internal `pinyinfont` target from the new `src/` paths.
- [x] 3.2 Update CLI and tool targets to use the new include directories without adding a public `include/pinyinfont/` API.
- [x] 3.3 Enable CTest and register at least one project test target or command.
- [x] 3.4 Configure data handling so CLI/tests use explicit or project-owned data paths rather than manually populated `build/data`.

## 4. Tests and Fixtures

- [x] 4.1 Add a small pinyin data fixture or documented data fixture path under `data/` or `tests/fixtures/`.
- [x] 4.2 Add at least one smoke/regression test that exercises migrated behavior such as pinyin data loading, CLI invocation, or approved font processing.
- [x] 4.3 Verify tests run from a freshly configured build directory without requiring untracked files under `build/data`.

## 5. Documentation

- [x] 5.1 Update `README.md` with concise project purpose, target layout, build command, test command, and CLI usage.
- [x] 5.2 Document any retained diagnostic modes and whether they live in the CLI or `tools/`.
- [x] 5.3 Avoid documenting a public library API or GUI plan as part of this change.

## 6. Verification

- [x] 6.1 Run a fresh CMake configure into a clean build directory.
- [x] 6.2 Run `cmake --build <build-dir>` and confirm the internal library, CLI, tools, and tests build.
- [x] 6.3 Run `ctest --test-dir <build-dir> --output-on-failure` and confirm tests are discovered and pass.
- [x] 6.4 Run the CLI smoke path documented in README and confirm it works with the new data path behavior.
