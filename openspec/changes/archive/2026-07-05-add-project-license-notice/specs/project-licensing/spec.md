## ADDED Requirements

### Requirement: Project license is declared
The project SHALL declare its source code and documentation license in a root license file.

#### Scenario: License file exists
- **WHEN** a contributor inspects the repository root
- **THEN** a `LICENSE` file is present

#### Scenario: MIT license is used
- **WHEN** the project license file is read
- **THEN** it identifies the project license as MIT

### Requirement: README describes licensing
The README SHALL document the project license and the font licensing boundary.

#### Scenario: Project license is documented
- **WHEN** a user reads the README
- **THEN** it states that the project is licensed under the MIT License

#### Scenario: Font rights are separated from tool rights
- **WHEN** a user reads the README font licensing notice
- **THEN** it states that the project license applies to the tool source and documentation but does not grant rights to third-party fonts

#### Scenario: Generated font responsibility is documented
- **WHEN** a user reads the README font licensing notice
- **THEN** it states that users are responsible for ensuring input and generated fonts comply with relevant font licenses

### Requirement: Runtime output remains focused
The project SHALL keep font licensing notices in documentation rather than normal command output.

#### Scenario: CLI usage stays quiet
- **WHEN** the `pinyinfont` command runs normally
- **THEN** it does not print a font licensing warning or disclaimer as part of normal output

#### Scenario: Tool usage stays quiet
- **WHEN** development tools run normally
- **THEN** they do not print a font licensing warning or disclaimer as part of normal output
