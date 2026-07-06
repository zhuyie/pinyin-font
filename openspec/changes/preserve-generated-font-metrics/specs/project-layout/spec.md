## ADDED Requirements

### Requirement: Generated font integrity diagnostics are available
The project SHALL expose generated-font integrity diagnostics through the unified development font tool rather than the product CLI.

#### Scenario: Font tool provides generated font integrity diagnostics
- **WHEN** a developer needs to inspect cmap preservation or metric consistency for a generated pinyin font
- **THEN** `font_tool` provides a command for that diagnostic workflow

#### Scenario: Product CLI remains focused
- **WHEN** the `pinyinfont` command runs normally
- **THEN** it does not print generated-font integrity warnings as part of ordinary successful output
