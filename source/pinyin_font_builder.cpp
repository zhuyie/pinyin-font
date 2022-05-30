#include "pinyin_font_builder.h"
#include "ot_font_parser.h"
#include "ot_font_writer.h"
#include <string>

//------------------------------------------------------------------------------

PinyinFontBuilder::PinyinFontBuilder()
: charSpace_(0), pinyinCharYMin_(0)
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

    charSpace_ = (int16_t)((font_.Head().XMax - font_.Head().XMin) * 0.1);
    pinyinCharYMin_ = __calcPinyinCharYMin();

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

int16_t PinyinFontBuilder::__calcPinyinCharYMin()
{
    static char testChars[] = { 'f', 'g', 'j', 'p', 'q', 'y' };
    int16_t ymin = 0;
    for (int i = 0; i < sizeof(testChars) / sizeof(testChars[0]); i++) {
        uint16_t glyphIndex = font_.CharToGlyphIndex(testChars[i]);
        if (glyphIndex == 0) {
            continue;
        }
        OpenType_GlyphHeader *pGlyph = NULL;
        font_.Glyph(glyphIndex, &pGlyph);
        if (pGlyph->YMin < ymin) {
            ymin = pGlyph->YMin;
        }
    }
    return ymin;
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
    __AddPinyinGlyph(0x6C49, L"ha\u0300n", 0.65);
    __AddPinyinGlyph(0x8BED, L"yu\u030C", 0.65);
    __AddPinyinGlyph(0x62FC, L"pi\u0304n", 0.65);
    __AddPinyinGlyph(0x97F3, L"yi\u0304n", 0.65);
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

    double pinyinRatio = 1.0 - baseRatio;
    int16_t baseDX = baseHmtx.AdvanceWidth * (1.0 - baseRatio) / 2;
    int16_t pinyinDY = (int16_t)(font_.Head().YMax * baseRatio) + (int16_t)(pinyinCharYMin_ * (-1) * pinyinRatio);
    int16_t centerX = (int16_t)(baseGlyph->XMin + (baseGlyph->XMax - baseGlyph->XMin) / 2);

    OpenType_GlyphComposite glyph;
    glyph.NumberOfContours = -1;
    glyph.XMin = baseGlyph->XMin;
    glyph.XMax = baseGlyph->XMax;
    glyph.YMin = baseGlyph->YMin;
    glyph.YMax = baseGlyph->YMax;

    std::vector<glyphInfo> pinyinGlyphs;
    int16_t pinyinWidth = 0;
    if (!__ComposePinyin(pinyin, pinyinGlyphs, pinyinWidth)) {
        return kError;
    }
    for (size_t i = 0; i < pinyinGlyphs.size(); i++) {
        glyphInfo info = pinyinGlyphs[i];
        uint16_t pinyinDX = centerX - (uint16_t)((pinyinWidth / 2 - info.OffsetX) * pinyinRatio);
        __AddSubGlyph(glyph, info.GlyphIndex, pinyinRatio, pinyinDX, pinyinDY + info.OffsetY * pinyinRatio, false);
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

bool PinyinFontBuilder::__ComposePinyin(
    const std::wstring &pinyin, std::vector<glyphInfo> &glyphs, int16_t &totalWidth)
{
    glyphs.clear();
    totalWidth = 0;

    glyphs.reserve(pinyin.size());

    // 3 kinds of cluster:
    // - ['u']
    // - ['u', u+0304]
    // - ['u', u+0308, u+0304]
    wchar_t cluster[3] = { 0 };
    for (size_t i = 0; i < pinyin.size(); i++) {
        wchar_t c = pinyin[i];
        if (__IsMarkChar(c)) {
            if (cluster[0] == 0) {
                return false;
            } else if (cluster[1] == 0) {
                cluster[1] = c;
            } else if (cluster[2] == 0) {
                cluster[2] = c;
            } else {
                return false;
            }
        } else {
            // previous cluster ended
            if (!__ComposeCluster(cluster, glyphs, totalWidth)) {
                return false;
            }
            // start a new cluster
            cluster[0] = c;
        }
    }
    // last cluster
    if (!__ComposeCluster(cluster, glyphs, totalWidth)) {
        return false;
    }

    return true;
}

bool PinyinFontBuilder::__ComposeCluster(
    wchar_t cluster[3], std::vector<glyphInfo> &glyphs, int16_t &x)
{
    if (cluster[0] == 0) {
        return true;
    }
    if (cluster[0] == 'i' && cluster[1] != 0) {
        switch (cluster[1]) {
        case 0x0304:
            cluster[0] = 0x012B;  // Latin Small Letter I with Macron
            break;
        case 0x0301:
            cluster[0] = 0x00ED;  // Latin Small Letter I with Acute
            break;
        case 0x030C:
            cluster[0] = 0x01D0;  // Latin Small Letter I with Caron
            break;
        case 0x0300:
            cluster[0] = 0x00EC;  // Latin Small Letter I with Grave
            break;
        default:
            return false;
        }
        cluster[1] = 0;
    }

    glyphInfo info;
    int16_t centerX, DY;
    OpenType_GlyphHeader *pGlyph = NULL;
    OpenType_LongHorMetric mtx = { 0 };

    // normal character
    info.GlyphIndex = font_.CharToGlyphIndex(cluster[0]);
    if (info.GlyphIndex == 0) {
        return false;
    }
    font_.Glyph(info.GlyphIndex, &pGlyph);
    font_.GlyphHorMetric(info.GlyphIndex, mtx);
    info.OffsetX = x - pGlyph->XMin + charSpace_ / 2;
    info.OffsetY = 0;
    info.AdvanceWidth = (pGlyph->XMax - pGlyph->XMin) + charSpace_;
    glyphs.push_back(info);

    centerX = (int16_t)(x + charSpace_ / 2 + (pGlyph->XMax - pGlyph->XMin) / 2);
    DY = pGlyph->YMax;

    x += info.AdvanceWidth;

    if (cluster[1] != 0) {
        // 1st mark
        info.GlyphIndex = font_.CharToGlyphIndex(cluster[1]);
        if (info.GlyphIndex == 0) {
            switch (cluster[1]) {
            case 0x0304:
                cluster[1] = 0x00AF;
                break;
            case 0x0301:
                cluster[1] = 0x00B4;
                break;
            case 0x030C:
                cluster[1] = 0x02C7;
                break;
            case 0x0300:
                cluster[1] = 0x0060;
                break;
            }
            info.GlyphIndex = font_.CharToGlyphIndex(cluster[1]);
            if (info.GlyphIndex == 0) {
                return false;
            }
        }
        font_.Glyph(info.GlyphIndex, &pGlyph);
        font_.GlyphHorMetric(info.GlyphIndex, mtx);
        info.OffsetX = centerX - (pGlyph->XMax - pGlyph->XMin) / 2 - pGlyph->XMin;
        info.OffsetY = DY - pGlyph->YMin;
        info.AdvanceWidth = pGlyph->XMax - pGlyph->XMin;
        glyphs.push_back(info);
    }

    cluster[0] = cluster[1] = cluster[2] = 0;
    return true;
}
