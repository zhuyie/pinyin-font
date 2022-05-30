#include "pinyin_font_builder.h"
#include "ot_font_parser.h"
#include "ot_font_writer.h"
#include "pinyin_db.h"

//------------------------------------------------------------------------------

PinyinFontBuilder::PinyinFontBuilder()
: baseRatio_(0.65), pinyinRatio_(0.35), 
  pinyinCharSpace_(0), pinyinMarkVSpace_(0), pinyinCharYMin_(0),
  baseDY_(0), pinyinDY_(0)
{
}

PinyinFontBuilder::~PinyinFontBuilder()
{
}

Status PinyinFontBuilder::Build(const char *sourceFont, const PinyinDB &pinyinDB)
{
    Status status;

    OpenType_Font_Parser parser;
    status = parser.Parse(sourceFont, &font_);
    if (status != kOk) {
        return status;
    }

    pinyinCharSpace_ = (int16_t)((font_.Head().XMax - font_.Head().XMin) * 0.1);
    pinyinMarkVSpace_ = (int16_t)(pinyinCharSpace_ * 0.33);
    pinyinCharYMin_ = __calcPinyinCharYMin();

    baseDY_ = font_.Head().YMin * pinyinRatio_ * 0.5;
    pinyinDY_ = baseDY_ + (int16_t)(font_.Head().YMax * baseRatio_) + (int16_t)(pinyinCharYMin_ * (-1) * pinyinRatio_);

    if (!__checkRequiredGlyphs()) {
        return kNotSupported;
    }

    __buildSubstitutions();

    status = __addPinyinGlyphs(pinyinDB);
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

bool PinyinFontBuilder::__checkRequiredGlyphs()
{
    size_t count = (sizeof(requiredChars) / sizeof(requiredChars[0]));
    for (size_t i = 0; i < count; i++) {
        uint16_t glyphID = font_.CharToGlyphIndex(requiredChars[i]);
        if (glyphID == 0) {
            return false;
        }
    }
    return true;
}

bool PinyinFontBuilder::__isMarkChar(wchar_t c)
{
    return (c == 0x0304) || (c == 0x0301) || (c == 0x030C) || (c == 0x0300) || (c == 0x0308);
}

bool PinyinFontBuilder::__alternativeChar(wchar_t &c)
{
    switch (c) {
    case 0x0304: c = 0x00AF; return true;
    case 0x0301: c = 0x00B4; return true;
    case 0x030C: c = 0x02C7; return true;
    case 0x0300: c = 0x0060; return true;
    case 0x0308: c = 0x00A8; return true;
    }
    return false;
}

void PinyinFontBuilder::__buildSubstitutions()
{
    static wchar_t s_substitution[][3] = {
        { 'u', 0x0308, 0x00FC },  // ü

        { 'a', 0x0304, 0x0101 },  // ā
        { 'a', 0x0301, 0x00E1 },  // á
        { 'a', 0x030C, 0x01CE },  // ǎ
        { 'a', 0x0300, 0x00E0 },  // à

        { 'e', 0x0304, 0x0113 },  // ē
        { 'e', 0x0301, 0x00E9 },  // é
        { 'e', 0x030C, 0x011B },  // ě
        { 'e', 0x0300, 0x00E8 },  // è

        { 'o', 0x0304, 0x014D },  // ō
        { 'o', 0x0301, 0x00F3 },  // ó
        { 'o', 0x030C, 0x01D2 },  // ǒ
        { 'o', 0x0300, 0x00F2 },  // ò

        { 'u', 0x0304, 0x016B },  // ū
        { 'u', 0x0301, 0x00FA },  // ú
        { 'u', 0x030C, 0x01D4 },  // ǔ
        { 'u', 0x0300, 0x00F9 },  // ù

        { 0x00FC, 0x0304, 0x01D6 },  // ǖ
        { 0x00FC, 0x0301, 0x01D8 },  // ǘ
        { 0x00FC, 0x030C, 0x01DA },  // ǚ
        { 0x00FC, 0x0300, 0x01DC },  // ǜ
    };
    for (int i = 0; i < sizeof(s_substitution) / sizeof(s_substitution[0]); i++) {
        uint16_t glyphIndex = font_.CharToGlyphIndex(s_substitution[i][2]);
        if (glyphIndex != 0) {
            uint64_t key = (uint64_t)(s_substitution[i][0]) << 32 | 
                           (uint64_t)(s_substitution[i][1]);
            substitutions_[key] = s_substitution[i][2];
        }
    }
}

Status PinyinFontBuilder::__addPinyinGlyphs(const PinyinDB &pinyinDB)
{
    PinyinRecord record;
    size_t count = pinyinDB.Count();
    for (size_t i = 0; i < count; i++) {
        pinyinDB.GetRecord(i, record);
        __addPinyinGlyph(record.charcode, record.pinyin0);
    }
    return kOk;
}

Status PinyinFontBuilder::__addPinyinGlyph(uint32_t charcode, const std::wstring &pinyin)
{
    uint32_t baseGlyphIndex = font_.CharToGlyphIndex(charcode);
    if (baseGlyphIndex == 0) {
        return kNotFound;
    }
    OpenType_GlyphHeader *baseGlyph = NULL;
    OpenType_LongHorMetric baseHmtx = { 0 };
    font_.Glyph(baseGlyphIndex, &baseGlyph);
    font_.GlyphHorMetric(baseGlyphIndex, baseHmtx);
    assert(baseGlyph != NULL);

    int16_t baseDX = baseHmtx.AdvanceWidth * (1.0 - baseRatio_) / 2;
    int16_t centerX = (int16_t)(baseGlyph->XMin + (baseGlyph->XMax - baseGlyph->XMin) / 2);

    OpenType_GlyphComposite &glyph = glyph_;
    glyph.NumberOfContours = -1;
    glyph.XMin = baseGlyph->XMin;
    glyph.XMax = baseGlyph->XMax;
    glyph.YMin = baseGlyph->YMin;
    glyph.YMax = baseGlyph->YMax;
    glyph.SubGlyphs.resize(0);

    std::vector<glyphInfo> &pinyinGlyphs = pinyinGlyphInfos_;
    pinyinGlyphs.resize(0);
    int16_t pinyinWidth = 0;
    if (!__composePinyin(pinyin, pinyinGlyphs, pinyinWidth)) {
        return kError;
    }
    for (size_t i = 0; i < pinyinGlyphs.size(); i++) {
        glyphInfo info = pinyinGlyphs[i];
        uint16_t pinyinDX = centerX - (uint16_t)((pinyinWidth / 2 - info.OffsetX) * pinyinRatio_);
        __addSubGlyph(glyph, info.GlyphIndex, pinyinRatio_, 
            pinyinDX, pinyinDY_ + info.OffsetY * pinyinRatio_, false);
    }

    __addSubGlyph(glyph, baseGlyphIndex, baseRatio_, baseDX, baseDY_, true);

    char name[20] = { 0 };
    sprintf(name, "uni%04X_pinyin", (unsigned int)charcode);

    Status status = font_.AddGlyph(&glyph, &baseHmtx, name);
    if (status != kOk) {
        return status;
    }

    return kOk;
}

void PinyinFontBuilder::__addSubGlyph(
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

bool PinyinFontBuilder::__composePinyin(
    const std::wstring &pinyin, std::vector<glyphInfo> &glyphs, int16_t &totalWidth)
{
    glyphs.clear();
    totalWidth = 0;

    // 3 kinds of cluster:
    // - ['u']
    // - ['u', u+0304]
    // - ['u', u+0308, u+0304]
    wchar_t cluster[3] = { 0 };
    for (size_t i = 0; i < pinyin.size(); i++) {
        wchar_t c = pinyin[i];
        if (__isMarkChar(c)) {
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
            if (!__composeCluster(cluster, glyphs, totalWidth)) {
                return false;
            }
            // start a new cluster
            cluster[0] = c;
        }
    }
    // last cluster
    if (!__composeCluster(cluster, glyphs, totalWidth)) {
        return false;
    }

    return true;
}

bool PinyinFontBuilder::__composeCluster(
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
        cluster[1] = cluster[2] = 0;
    }

    for (;;) {
        if (cluster[1] == 0) {
            break;
        }
        uint64_t key = (uint64_t)cluster[0] << 32 | (uint64_t)cluster[1];
        auto iter = substitutions_.find(key);
        if (iter == substitutions_.end()) {
            break;
        }
        cluster[0] = iter->second;
        cluster[1] = cluster[2];
        cluster[2] = 0;
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
    info.OffsetX = x - pGlyph->XMin + pinyinCharSpace_ / 2;
    info.OffsetY = 0;
    info.AdvanceWidth = (pGlyph->XMax - pGlyph->XMin) + pinyinCharSpace_;
    glyphs.push_back(info);

    centerX = (int16_t)(x + pinyinCharSpace_ / 2 + (pGlyph->XMax - pGlyph->XMin) / 2);
    DY = pGlyph->YMax + pinyinMarkVSpace_;

    x += info.AdvanceWidth;

    if (cluster[1] != 0) {
        // 1st mark
        info.GlyphIndex = font_.CharToGlyphIndex(cluster[1]);
        if (info.GlyphIndex == 0) {
            if (!__alternativeChar(cluster[1])) {
                return false;
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
        DY += (pGlyph->YMax - pGlyph->YMin) + pinyinMarkVSpace_;
    }
    if (cluster[2] != 0) {
        // 2nd mark
        info.GlyphIndex = font_.CharToGlyphIndex(cluster[2]);
        if (info.GlyphIndex == 0) {
            if (!__alternativeChar(cluster[2])) {
                return false;
            }
            info.GlyphIndex = font_.CharToGlyphIndex(cluster[2]);
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
