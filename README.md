# pinyin-font

`pinyin-font` is a command-line tool for generating pinyin-annotated TrueType fonts.

The project is organized around the CLI product, with the font processing code kept as internal implementation:

```text
src/opentype/   OpenType parsing, writing, cmap, glyph, and table logic
src/pinyin/     Pinyin database loading and normalization
src/synthesis/  Pinyin glyph and font synthesis
cli/            User-facing command-line entry point
tools/          Development and diagnostic utilities
tests/          CTest-discoverable smoke/regression tests
data/           Project-owned pinyin data inputs
```

## Build

```sh
cmake -S . -B build
cmake --build build
```

The main executable is `build/pinyinfont`. Development font diagnostics build as `build/font_tool`.

## Test

```sh
ctest --test-dir build --output-on-failure
```

## CLI

```sh
build/pinyinfont --input <font.ttf> --pinyin-db <pinyin-db.txt> [--output <out.ttf>]
```

The pinyin database is required because it controls which pinyin readings are synthesized. If `--output` is omitted, the output path defaults to `<font.ttf>.pinyin.ttf`.

```sh
build/pinyinfont --input input.ttf --pinyin-db data/TGHZ2013.txt
```

## Tools

Developer diagnostics live in `font_tool`:

```sh
build/font_tool info --input <font.ttf>
build/font_tool bench-parse --input <font-directory>
build/font_tool rewrite --input <font.ttf> [--output <out.ttf>]
build/font_tool table-dump --input <font.ttf> --table <tag> [--output <file.dat>]
build/font_tool table-purge --input <font.ttf> --table <tag> [--output <out.ttf>]
```

Visual validation lives in the standalone browser page `tools/preview.html`.
Open it directly, select the generated pinyin font, and optionally select the
original font for side-by-side comparison.

## License

This project is licensed under the MIT License.

## Font Licensing Notice

This tool modifies and writes font files. The project license applies only to
this tool's source code and documentation; it does not grant any rights to use,
modify, redistribute, embed, or publish third-party fonts.

Before processing a font, make sure its license permits the intended use,
including modification and redistribution of derived font files. Generated
fonts remain subject to the license terms of the original font and any other
applicable rights. You are responsible for ensuring that your use of input and
generated fonts complies with the relevant font licenses.
