## Why

The repository currently has no explicit project license, and the tool modifies font files whose source fonts may have strict licensing terms. The project should clearly license its own source code while making the font copyright boundary visible to users.

## What Changes

- Add a root `LICENSE` file using the MIT License.
- Add an English README license section pointing to the MIT project license.
- Add an English README font licensing notice explaining that the project license applies only to the tool source and documentation, not to third-party fonts.
- Clarify that users are responsible for ensuring their input fonts and generated fonts comply with the relevant font licenses.
- Do not add runtime warning output to the CLI or tools.

## Capabilities

### New Capabilities
- `project-licensing`: Project license and font licensing notice requirements.

### Modified Capabilities
- None.

## Impact

- Affected files: `LICENSE`, `README.md`, and OpenSpec artifacts.
- Affected behavior: no runtime behavior change.
- APIs: no C++ API or command-line argument change.
- Dependencies: no new dependency.
