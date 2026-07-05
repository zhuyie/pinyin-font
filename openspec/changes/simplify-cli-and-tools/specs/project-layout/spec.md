## MODIFIED Requirements

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
