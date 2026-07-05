## Context

`pinyin-font` is a CLI tool that reads and writes font files. The repository code can be licensed under a normal software license, but the fonts processed by the tool remain subject to their own licenses. This distinction matters because generated fonts are derived from user-provided input fonts.

The current README is English, so the new notice should stay English for consistency. The user also explicitly prefers not to print runtime warnings.

## Goals / Non-Goals

**Goals:**
- Add a standard MIT License file for the project source code and documentation.
- Document the font licensing boundary in README.
- Make clear that generated fonts remain subject to source font licensing terms.
- Keep the CLI and tools quiet; no runtime legal warning should be printed.

**Non-Goals:**
- Do not provide legal advice or license compatibility analysis.
- Do not inspect, validate, or enforce font license metadata.
- Do not change generated font metadata.
- Do not add startup warnings, build warnings, or command output disclaimers.
- Do not add a dependency or license scanning tool.

## Decisions

1. Use the standard MIT License text in `LICENSE`.

   MIT is appropriate for the tool source because it is short, permissive, and already includes the usual software warranty disclaimer. The license should cover the project code and documentation, not third-party font files.

2. Keep the README notice concise and English-only.

   The README is currently English. A compact `License` section and `Font Licensing Notice` section are enough to communicate the boundary without turning the documentation into legal prose.

3. Avoid runtime warnings.

   The warning belongs in durable documentation. Printing it during normal command execution would make the CLI noisy and would not provide meaningful enforcement.

## Risks / Trade-offs

- Users may overlook README text -> Put the notice under clear headings near the existing usage/tool documentation.
- Disclaimer could sound like legal advice -> Keep it factual and focused on user responsibility.
- MIT warranty disclaimer does not cover font rights -> Add a separate font licensing notice rather than relying on MIT text alone.

## Migration Plan

1. Add root `LICENSE` with standard MIT text and project copyright holder.
2. Add README `License` section.
3. Add README `Font Licensing Notice` section.
4. Verify no runtime warning text is added to CLI or tool output.
5. Run OpenSpec validation and whitespace checks.

## Open Questions

None.
