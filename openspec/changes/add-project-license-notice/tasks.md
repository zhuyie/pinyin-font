## 1. License File

- [x] 1.1 Add a root `LICENSE` file.
- [x] 1.2 Use standard MIT License text.
- [x] 1.3 Set the copyright line to `Copyright (c) 2026 zhuyie`.

## 2. README Notice

- [x] 2.1 Add an English `License` section stating that the project is licensed under MIT.
- [x] 2.2 Add an English `Font Licensing Notice` section.
- [x] 2.3 State that the project license applies only to the tool source and documentation, not third-party fonts.
- [x] 2.4 State that users are responsible for ensuring input and generated fonts comply with relevant font licenses.

## 3. Runtime Behavior

- [x] 3.1 Do not add runtime license or font copyright warnings to `pinyinfont`.
- [x] 3.2 Do not add runtime license or font copyright warnings to development tools.

## 4. Verification

- [x] 4.1 Verify the repository root contains `LICENSE`.
- [x] 4.2 Verify README contains the license and font licensing notice.
- [x] 4.3 Search CLI/tool source to confirm no runtime warning text was added.
- [x] 4.4 Run `openspec validate add-project-license-notice --strict`.
- [x] 4.5 Run `git diff --check`.
