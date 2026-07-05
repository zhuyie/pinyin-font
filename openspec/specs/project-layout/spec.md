## Purpose

Define the repository layout and boundaries for the CLI-first pinyin font tool.

## Requirements

### Requirement: CLI-first repository layout
The project SHALL organize source directories around a CLI font-generation tool rather than a public C++ library API.

#### Scenario: Repository layout communicates product shape
- **WHEN** a contributor inspects the repository root
- **THEN** the root contains dedicated areas for internal source code, the CLI entry point, development tools, tests, and data inputs

#### Scenario: Public API directory is not introduced
- **WHEN** the layout migration is complete
- **THEN** the project does not add a public `include/pinyinfont/` API directory unless a separate change introduces an embeddable API contract

#### Scenario: GUI remains out of scope
- **WHEN** the layout migration is complete
- **THEN** the project does not add a GUI directory or GUI implementation placeholders

### Requirement: Internal source modules are domain-oriented
The project SHALL place internal implementation code under `src/` using domain-oriented module directories.

#### Scenario: OpenType code is grouped
- **WHEN** OpenType parsing, writing, cmap, glyph, name, or table code is located
- **THEN** it resides under `src/opentype/`

#### Scenario: Pinyin data code is grouped
- **WHEN** pinyin record loading, sorting, or normalization code is located
- **THEN** it resides under `src/pinyin/`

#### Scenario: Font synthesis code is grouped
- **WHEN** code that composes pinyin glyphs with base glyphs is located
- **THEN** it resides under `src/synthesis/`

### Requirement: CLI entry point is explicit
The project SHALL provide a dedicated CLI entry point outside internal source modules, and that entry point SHALL expose only the user-facing pinyin font generation workflow.

#### Scenario: User-facing executable source is located
- **WHEN** a contributor looks for the command-line program entry point
- **THEN** the entry point source resides under `cli/`

#### Scenario: Test naming is reserved for tests
- **WHEN** executable targets are listed
- **THEN** user-facing or development CLI executables are not named as tests unless they are registered test programs

#### Scenario: Product CLI uses explicit required inputs
- **WHEN** the user runs the product CLI
- **THEN** the command requires `--input <font.ttf>` and `--pinyin-db <pinyin-db.txt>` options

#### Scenario: Product CLI output is explicit or derived
- **WHEN** the user runs the product CLI with `--output <out.ttf>`
- **THEN** the generated font is written to the specified output path

#### Scenario: Product CLI default output is derived from input
- **WHEN** the user runs the product CLI without `--output`
- **THEN** the generated font is written to `<input>.pinyin.ttf`

#### Scenario: Product CLI omits diagnostic modes
- **WHEN** the user inspects or invokes the product CLI
- **THEN** it does not expose `build`, `dump`, `bench`, or `rewrite` mode arguments

### Requirement: Development tools remain separate
The project SHALL keep development-only utilities separate from the user-facing CLI and internal implementation modules.

#### Scenario: Font diagnostics are unified
- **WHEN** font inspection, parser benchmarking, rewrite roundtrip, table dump, or table purge behavior is needed
- **THEN** it is exposed through a unified `font_tool` executable under `tools/`

#### Scenario: Font info command exists
- **WHEN** a developer runs `font_tool info --input <font.ttf>`
- **THEN** the tool prints font summary information equivalent to the previous CLI dump behavior

#### Scenario: Font parser benchmark command exists
- **WHEN** a developer runs `font_tool bench-parse --input <font-directory>`
- **THEN** the tool benchmarks parsing of `.ttf` files under the directory

#### Scenario: Font rewrite command exists
- **WHEN** a developer runs `font_tool rewrite --input <font.ttf>`
- **THEN** the tool performs a parse/write roundtrip and writes either the specified `--output` path or a derived rewrite output path

#### Scenario: Font table dump command exists
- **WHEN** a developer runs `font_tool table-dump --input <font.ttf> --table <tag>`
- **THEN** the tool writes either the specified `--output` path or a derived table data output path

#### Scenario: Font table purge command exists
- **WHEN** a developer runs `font_tool table-purge --input <font.ttf> --table <tag>`
- **THEN** the tool writes either the specified `--output` path or a derived purged font output path

#### Scenario: Legacy table tool target is replaced
- **WHEN** executable targets are listed
- **THEN** the project does not expose a separate `font_table_tool` target in addition to `font_tool`

### Requirement: Tests are discoverable
The project SHALL include CTest-discoverable tests that verify the migrated structure still builds and supports core behavior.

#### Scenario: CTest discovers tests
- **WHEN** `ctest --test-dir <build-dir> --output-on-failure` is run after configuration
- **THEN** CTest discovers and runs at least one project test

#### Scenario: Migration smoke coverage exists
- **WHEN** the tests are run
- **THEN** at least one test exercises core project behavior affected by the migration, such as pinyin data loading, CLI invocation, or font processing on an approved fixture

### Requirement: Data inputs are project-owned or explicit
The project SHALL avoid relying on manually populated build-directory data as the source of truth for CLI or test behavior.

#### Scenario: CLI data dependency is explicit
- **WHEN** the CLI needs pinyin data
- **THEN** the data path is provided by a documented argument, a project-owned `data/` path, or a documented installed/copied resource path

#### Scenario: Tests do not depend on local build artifacts
- **WHEN** tests run from a fresh build directory
- **THEN** they do not require pre-existing untracked files under `build/data`

### Requirement: Generated artifacts are excluded from source structure
The project SHALL exclude generated build outputs, generated fonts, editor metadata, caches, and OS metadata from the tracked project structure.

#### Scenario: Local generated files are ignored
- **WHEN** local build products, generated font outputs, editor settings, cache directories, or `.DS_Store` files are created
- **THEN** they do not appear as untracked project files intended for review
