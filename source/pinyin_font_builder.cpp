#include "pinyin_font_builder.h"
#include "ot_font_parser.h"
#include "ot_font_writer.h"
#include <string>

//------------------------------------------------------------------------------

PinyinFontBuilder::PinyinFontBuilder()
{
}

PinyinFontBuilder::~PinyinFontBuilder()
{
}

Status PinyinFontBuilder::Build(const char *sourceFont)
{
    Status status;

    OpenType_Font_Parser parser;
    status = parser.Parse(sourceFont, &font_);
    if (status != kOk) {
        return status;
    }

    status = __checkRequiredGlyphs();
    if (status != kOk) {
        return status;
    }

    status = __AddPinyinGlyphs();
    if (status != kOk) {
        return status;
    }

    std::string outFile = sourceFont;
    outFile += ".pinyin.ttf";
    OpenType_Font_Writer writer;
    status = writer.Write(outFile.c_str(), &font_);
    if (status != kOk) {
        return status;
    }

    return kOk;
}

static uint32_t requiredChars[] = {
    0x6C49, 0x8BED, 0x62FC, 0x97F3,  // han yu pin yin
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

Status PinyinFontBuilder::__checkRequiredGlyphs()
{
    size_t count = (sizeof(requiredChars) / sizeof(requiredChars[0]));
    for (size_t i = 0; i < count; i++) {
        uint16_t glyphID = font_.CharToGlyphIndex(requiredChars[i]);
        if (glyphID == 0) {
            return kNotSupported;
        }
    }
    return kOk;
}

Status PinyinFontBuilder::__AddPinyinGlyphs()
{
    __AddPinyinGlyph(0x6C49, "han");
    __AddPinyinGlyph(0x8BED, "yu");
    __AddPinyinGlyph(0x62FC, "pin");
    __AddPinyinGlyph(0x97F3, "yin");
    return kOk;
}

Status PinyinFontBuilder::__AddPinyinGlyph(uint32_t charcode, const char* pinyin)
{
    uint32_t baseGlyphIndex = font_.CharToGlyphIndex(charcode);
    if (baseGlyphIndex == 0) {
        return kNotFound;
    }
    OpenType_GlyphHeader *baseGlyph = NULL;
    Status status = font_.Glyph(baseGlyphIndex, &baseGlyph);
    if (status != kOk) {
        return status;
    }
    assert(baseGlyph != NULL);
    OpenType_LongHorMetric mtx = { 0 };
    status = font_.GlyphHorMetric(baseGlyphIndex, mtx);
    if (status != kOk) {
        return status;
    }

    OpenType_GlyphComposite g;
    g.NumberOfContours = -1;
    g.XMin = baseGlyph->XMin;
    g.XMax = baseGlyph->XMax;
    g.YMin = baseGlyph->YMin;
    g.YMax = baseGlyph->YMax;
    OpenType_GlyphComponent c = { 0 };
    c.Flags = OpenType_FlagArgsAreXYValues | OpenType_FlagWeHaveAScale;
    c.Transform[0] = (int16_t)(0.7 * 16384.0);  // in F2DOT14
    c.Transform[3] = c.Transform[0];
    c.GlyphIndex = baseGlyphIndex;
    g.SubGlyphs.push_back(c);

    char glyphName[20] = { 0 };
    sprintf(glyphName, "uni%04X_pinyin", (unsigned int)charcode);

    status = font_.AddGlyph(&g, &mtx, glyphName);
    if (status != kOk) {
        return status;
    }

    return kOk;
}
