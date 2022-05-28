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
    __AddPinyinGlyph(0x6C49, L"han", 0.65);
    __AddPinyinGlyph(0x8BED, L"yu", 0.65);
    __AddPinyinGlyph(0x62FC, L"pin", 0.65);
    __AddPinyinGlyph(0x97F3, L"yin", 0.65);
    return kOk;
}

Status PinyinFontBuilder::__AddPinyinGlyph(uint32_t charcode, const std::wstring &pinyin, double baseRatio)
{
    assert(baseRatio > 0 && baseRatio < 0.9);

    uint32_t baseGlyphIndex = font_.CharToGlyphIndex(charcode);
    if (baseGlyphIndex == 0) {
        return kNotFound;
    }
    OpenType_GlyphHeader *baseGlyph = NULL;
    OpenType_LongHorMetric baseHmtx = { 0 };
    if (font_.Glyph(baseGlyphIndex, &baseGlyph) != kOk || font_.GlyphHorMetric(baseGlyphIndex, baseHmtx) != kOk) {
        return kError;
    }
    assert(baseGlyph != NULL);

    double pinyinRatio = 0.9 - baseRatio;
    int16_t baseDX = baseHmtx.AdvanceWidth * (1.0 - baseRatio) / 2;
    int16_t pinyinDY = (int16_t)(font_.Hhea().Ascender * baseRatio) + (int16_t)(font_.Hhea().Descender * (-1) * pinyinRatio);
    int16_t centerX = (int16_t)(baseGlyph->XMin + (baseGlyph->XMax - baseGlyph->XMin) / 2);

    OpenType_GlyphComposite glyph;
    glyph.NumberOfContours = -1;
    glyph.XMin = baseGlyph->XMin;
    glyph.XMax = baseGlyph->XMax;
    glyph.YMin = baseGlyph->YMin;
    glyph.YMax = baseGlyph->YMax;

    const int MAX_PINYIN_CHARS = 10;
    uint16_t glyphIndexes[MAX_PINYIN_CHARS];
    uint16_t glyphOriginXs[MAX_PINYIN_CHARS];
    int32_t totalWidth = 0;
    for (size_t i = 0; i < MAX_PINYIN_CHARS && i < pinyin.size(); i++) {
        wchar_t c = pinyin[i];
        glyphIndexes[i] = font_.CharToGlyphIndex(c);
        if (glyphIndexes[i] == 0) {
            return kError;
        }
        OpenType_LongHorMetric mtx = { 0 };
        font_.GlyphHorMetric(glyphIndexes[i], mtx);
        glyphOriginXs[i] = totalWidth;
        totalWidth += mtx.AdvanceWidth;
    }
    for (size_t i = 0; i < MAX_PINYIN_CHARS && i < pinyin.size(); i++) {
        uint16_t pinyinDX = centerX - (uint16_t)((totalWidth / 2 - glyphOriginXs[i]) * pinyinRatio);
        __AddSubGlyph(glyph, glyphIndexes[i], pinyinRatio, pinyinDX, pinyinDY, false);
    }

    __AddSubGlyph(glyph, baseGlyphIndex, baseRatio, baseDX, 0, true);

    char name[20] = { 0 };
    sprintf(name, "uni%04X_pinyin", (unsigned int)charcode);

    Status status = font_.AddGlyph(&glyph, &baseHmtx, name);
    if (status != kOk) {
        return status;
    }

    return kOk;
}

void PinyinFontBuilder::__AddSubGlyph(
    OpenType_GlyphComposite &glyph, uint16_t glyphIndex, double scale, int16_t dx, int16_t dy, bool isLastOne)
{
    OpenType_GlyphComponent c = { 0 };
    c.Flags = OpenType_FlagArgsAreXYValues | OpenType_FlagUnscaledComponentOffset | OpenType_FlagWeHaveAScale;
    if (!isLastOne) {
        c.Flags |= OpenType_FlagMoreComponents;
    }
    if (dx > 127 || dx < -128 || dy > 127 || dy < -128) {
        c.Flags |= OpenType_FlagArg1And2AreWords;
    }
    c.Arg1 = dx;
    c.Arg2 = dy;
    c.Transform[0] = c.Transform[3] = (int16_t)(scale * 16384.0);  // in F2DOT14
    c.GlyphIndex = glyphIndex;
    glyph.SubGlyphs.push_back(c);
}
