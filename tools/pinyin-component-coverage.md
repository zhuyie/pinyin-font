# Pinyin component coverage diagnostic

Run the development diagnostic with:

```sh
./build/pinyinfont \
  --input build/ZCOOLXiaoWei-Regular.ttf \
  --pinyin-db data/TGHZ2013.txt \
  --output /tmp/ZCOOLXiaoWei-Regular.pinyin.ttf
```

Before internal components were added, the ZCOOL sample produced:

- 3,842 generated Han glyphs
- 1,271 records skipped because the source Han glyph was absent
- 2,796 failures caused by missing pinyin components

The CLI now reports these categories independently as
`SourceHanMissing`, `SourceOnlyGenerated`, `ToneFallbackGenerated`,
`DotlessIGenerated`, `ComponentFailed`, `DotlessIFailed`, and `OtherFailed`.
It also reports reuse counts for each generated internal component kind.

For the same sample after this change, the expected totals are:

- 6,638 generated Han glyphs
- 1,271 source-Han absences
- 0 component, dotless-i, or other synthesis failures

The integrity report currently shows 18 generated composite glyphs whose
decorative pinyin outline extends beyond the source Han advance width (the
pre-change sample had 16), plus additional composites below the source
`hhea.descender`. These are consequences of fitting long pinyin above narrow
Han advances, not malformed internal components: the `head` and OS/2 Windows
bounds remain valid, and internal simple-glyph bounds, instructions, cmap
visibility, and `maxp` metadata all pass.

Validate the generated font with:

```sh
./build/font_tool integrity \
  --source build/ZCOOLXiaoWei-Regular.ttf \
  --input /tmp/ZCOOLXiaoWei-Regular.pinyin.ttf
```

Visual inspection in `tools/preview.html` confirmed that the ZCOOL fallback
components remain readable from 24px through 120px after increasing the
font-relative mark gap and strengthening the procedural acute/grave outlines.
Decorative source accents are retained when present; only missing components
use the procedural outlines.

Additional end-to-end checks with `FZSSJW.TTF` and `FZKTJW.TTF` both generated
6,638 Han glyphs with zero component failures. Those fonts supplied their own
common precomposed pinyin glyphs, so only two exceptional acute uses required
an internal fallback and dotless-i derivation was not needed.
