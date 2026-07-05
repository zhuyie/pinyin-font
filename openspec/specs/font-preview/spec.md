## Purpose

Define the browser-based visual validation tool for generated pinyin font files.

## Requirements

### Requirement: Browser preview page exists
The project SHALL provide a tracked browser page for manually previewing generated pinyin font files.

#### Scenario: Preview page is discoverable
- **WHEN** a contributor looks for the visual font validation tool
- **THEN** the project provides it under `tools/`

#### Scenario: Preview page does not require a build artifact
- **WHEN** the repository is checked out
- **THEN** the preview page is available outside ignored build output directories

### Requirement: Preview page loads selected fonts
The preview page SHALL load user-selected local font files through browser file inputs.

#### Scenario: Pinyin font is selected
- **WHEN** the user selects a generated pinyin font file
- **THEN** the page uses that file for pinyin-font preview rendering

#### Scenario: Original font is selected
- **WHEN** the user selects an original font file
- **THEN** the page uses that file for original-font comparison rendering

#### Scenario: Original font is omitted
- **WHEN** the user selects only a generated pinyin font file
- **THEN** the page still renders the pinyin-font preview without requiring an original font

#### Scenario: Font loading status is visible
- **WHEN** a selected font is loaded or rejected by the browser
- **THEN** the page displays the corresponding load status to the user

### Requirement: Preview page supports visual validation
The preview page SHALL provide representative controls and samples for checking generated pinyin font rendering.

#### Scenario: User edits preview text
- **WHEN** the user enters custom preview text
- **THEN** the pinyin-font preview renders the custom text

#### Scenario: User adjusts preview size
- **WHEN** the user changes the preview size control
- **THEN** the preview text updates to the selected size

#### Scenario: Side-by-side comparison is available
- **WHEN** both pinyin and original fonts are loaded
- **THEN** the page shows comparable pinyin-font and original-font renderings

#### Scenario: Long pinyin samples are available
- **WHEN** the preview page is opened
- **THEN** it includes sample characters or text useful for checking long pinyin and tone mark placement

#### Scenario: Multiline layout samples are available
- **WHEN** the preview page is opened
- **THEN** it includes multiline sample text useful for checking line height, clipping, and visual stability
