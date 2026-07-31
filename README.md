# pinyin-font

`pinyin-font` is a command-line tool for generating pinyin-annotated TrueType fonts.

```text
src/opentype/   OpenType parsing, writing, cmap, glyph, and table logic
src/pinyin/     Pinyin database loading and normalization
src/synthesis/  Pinyin glyph and font synthesis
cli/            User-facing command-line entry point
tools/          Development and diagnostic utilities
tests/          CTest-discoverable smoke/regression tests
```

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

## CLI

```sh
build/pinyinfont --input <font.ttf> --pinyin-db <pinyin-db.txt> [--output <out.ttf>]
```

The pinyin database is required because it controls which pinyin readings are
synthesized. Users are responsible for generating or obtaining the database
and providing its path to the CLI. Its format comes from
[`zhuyie/pinyin-db`](https://github.com/zhuyie/pinyin-db): UTF-8 text with one
entry per line and three tab-separated fields — the character, its hexadecimal
Unicode scalar value, and one or more comma-separated pinyin readings.

```text
三	4E09	sān
上	4E0A	shàng,shǎng
下	4E0B	xià
```

If `--output` is omitted, the output path defaults to
`<font.ttf>.pinyin.ttf`.

### Polyphonic reading selectors

For a character with multiple database readings, append `@` and the one-based
reading index to select a non-default annotation:

```text
藏	cáng,zàng

收藏@1   selects cáng
西藏@2   selects zàng
```

The generated font implements valid selectors with the standard OpenType
`liga` feature. Supporting shaping engines consume the visible `@1` or `@2`
suffix and display one annotated Han glyph. The selector remains part of the
underlying text, so it is still present when copying, searching, or exposing
the text to accessibility software. If standard ligatures are disabled, the
renderer does not support GSUB, or the index is invalid, the suffix remains
visible as literal text.

## Tools

Developer diagnostics live in `font_tool`:

```sh
build/font_tool info --input <font.ttf>
build/font_tool table-dump --input <font.ttf> --table <tag> [--output <file.dat>]
build/font_tool table-purge --input <font.ttf> --table <tag> [--output <out.ttf>]
build/font_tool integrity --source <original.ttf> --input <generated.ttf>
```

Use `info` to inspect a font's core tables, names, representative glyphs and
metrics, and selected cmap mappings.

Use `table-dump` to extract a table's raw bytes. If `--output` is omitted, the
default path is `<font.ttf>.<tag>.dat` (`OS/2` is written as `OS2` in the
filename).

Use `table-purge` to write a copy of the font without the selected table. It
updates the table directory and font checksum; the required `head` table cannot
be removed. If `--output` is omitted, the default path is
`<font.ttf>.purged.ttf`.

Use `integrity` after generating a font to compare it with its source. The
report checks source cmap preservation and the metrics, bounds, component
structure, instructions, cmap visibility, and `maxp` metadata of generated
glyphs. It also reports parsed GSUB selector ligatures and invalid glyph
references.

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
