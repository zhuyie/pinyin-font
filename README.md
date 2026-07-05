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

The main executable is `build/pinyinfont`.

## Test

```sh
ctest --test-dir build --output-on-failure
```

## CLI

```sh
build/pinyinfont build <font.ttf> [pinyin-db]
```

If `pinyin-db` is omitted, `build` mode reads `data/TGHZ2013.txt` relative to the current working directory. Run the command from the repository root or pass an explicit path:

```sh
build/pinyinfont build input.ttf data/TGHZ2013.txt
```

Diagnostic modes retained in the CLI:

```sh
build/pinyinfont dump <font.ttf>
build/pinyinfont bench <font-directory>
build/pinyinfont rewrite <font.ttf>
```

Development table utilities remain separate under `tools/` and build as `font_table_tool`.
