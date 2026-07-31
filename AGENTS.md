# Repository Guidelines

## Commit Messages

All commits created in this repository must follow the Conventional Commits
format:

```text
<type>(optional-scope): <description>
```

Use one of these types:

- `feat`: add or change user-facing functionality
- `fix`: correct a defect
- `docs`: change documentation only
- `refactor`: restructure code without changing behavior
- `test`: add or update tests
- `build`: change the build system or dependencies
- `ci`: change continuous integration
- `perf`: improve performance
- `chore`: perform repository maintenance
- `revert`: revert an earlier commit

Keep the description concise, lowercase, and imperative. Add a scope when it
makes the affected area clearer. Before committing, verify that the complete
message follows this convention.

Examples:

```text
feat(synthesis): support explicit polyphonic readings
fix(opentype): preserve source cmap mappings
docs: document the pinyin database format
refactor(cli): remove obsolete font tool commands
```

Automatically generated merge commits are exempt from this convention.
