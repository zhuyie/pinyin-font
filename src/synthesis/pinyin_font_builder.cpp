#include "pinyin_font_builder.h"
#include "ot_font_parser.h"
#include "ot_font_writer.h"
#include "pinyin_db.h"
#include "pinyin_components.h"
#include <algorithm>
#include <cassert>
#include <chrono>
using namespace std::chrono;

//------------------------------------------------------------------------------

PinyinFontBuilder::PinyinFontBuilder()
: baseRatio_(0.65), pinyinRatio_(0.35), 
  pinyinMarkVSpace_(0), pinyinCharYMin_(0),
  baseDY_(0), pinyinDY_(0),
  glyphCountOld_(0), glyphCountAddOK_(0), glyphCountAddFailed_(0),
  usedGeneratedMark_(false), usedDotlessI_(false),
  parseTime_(0), synthesisTime_(0), writeTime_(0)
{
}

PinyinFontBuilder::~PinyinFontBuilder()
{
}

Status PinyinFontBuilder::Build(const char *sourceFont, const PinyinDB &pinyinDB)
{
    std::string outFile = sourceFont;
    outFile += ".pinyin.ttf";
    return Build(sourceFont, outFile.c_str(), pinyinDB);
}

Status PinyinFontBuilder::Build(const char *sourceFont, const char *outputFont, const PinyinDB &pinyinDB)
{
    Status status;
    auto start = system_clock::now();
    
    OpenType_Font_Parser parser;
    status = parser.Parse(sourceFont, &font_, {"GSUB"});
    if (status != kOk) {
        return status;
    }

    auto parseDone = system_clock::now();

    glyphCountOld_ = font_.GlyphCount();
    glyphCountAddOK_ = glyphCountAddFailed_ = 0;
    synthesisStats_ = PinyinSynthesisStats();
    components_.reset(new PinyinComponents(font_));

    pinyinMarkVSpace_ = components_->MarkGap();
    pinyinCharYMin_ = __calcPinyinCharYMin();

    PinyinVerticalBand verticalBand = SelectPinyinVerticalBand(
        font_.OS2(), font_.Hhea(), font_.Head());
    baseDY_ = 0;
    if (verticalBand.YMin < 0) {
        baseDY_ = (int16_t)(verticalBand.YMin * (1.0 - baseRatio_));
    }
    pinyinDY_ = baseDY_ +
        (int16_t)(verticalBand.YMax * baseRatio_) +
        (int16_t)(pinyinCharYMin_ * (-1) * pinyinRatio_);

    if (!__checkRequiredGlyphs()) {
        return kNotSupported;
    }

    __buildSubstitutions();

    status = __retainSourceCmap();
    if (status != kOk) {
        return status;
    }

    status = __addScaledPunctuationGlyphs();
    if (status != kOk) {
        return status;
    }

    status = __addPinyinGlyphs(pinyinDB);
    if (status != kOk) {
        return status;
    }

    status = __updateCmap();
    if (status != kOk) {
        return status;
    }

    auto synthesisDone = system_clock::now();

    OpenType_Font_Writer writer;
    status = writer.Write(outputFont, &font_);
    if (status != kOk) {
        return status;
    }

    auto writeDone = system_clock::now();

    parseTime_ = (uint32_t)(duration_cast<microseconds>(parseDone - start).count());
    synthesisTime_ = (uint32_t)(duration_cast<microseconds>(synthesisDone - parseDone).count());
    writeTime_ = (uint32_t)(duration_cast<microseconds>(writeDone - synthesisDone).count());

    return kOk;
}

void PinyinFontBuilder::GetStats(
    uint16_t &glyphCountOld, 
    uint16_t &glyphCountAddOK, 
    uint16_t &glyphCountAddFailed,
    uint32_t &parseTime, 
    uint32_t &synthesisTime, 
    uint32_t &writeTime)
{
    glyphCountOld = glyphCountOld_;
    glyphCountAddOK = glyphCountAddOK_;
    glyphCountAddFailed = glyphCountAddFailed_;
    parseTime = parseTime_;
    synthesisTime = synthesisTime_;
    writeTime = writeTime_;
}

void PinyinFontBuilder::GetComponentStats(PinyinComponentStats &stats) const
{
    stats = PinyinComponentStats();
    if (!components_) return;
    stats.MacronUses = components_->GeneratedMarkUseCount(0x0304);
    stats.AcuteUses = components_->GeneratedMarkUseCount(0x0301);
    stats.CaronUses = components_->GeneratedMarkUseCount(0x030C);
    stats.GraveUses = components_->GeneratedMarkUseCount(0x0300);
    stats.DiaeresisUses = components_->GeneratedMarkUseCount(0x0308);
    stats.DotlessIUses = components_->DotlessIUseCount();
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
        const OpenType_GlyphHeader *pGlyph = NULL;
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

void PinyinFontBuilder::__buildSubstitutions()
{
    substitutions_.clear();
    static wchar_t s_substitution[][3] = {
        { 'i', 0x0304, 0x012B },  // ī
        { 'i', 0x0301, 0x00ED },  // í
        { 'i', 0x030C, 0x01D0 },  // ǐ
        { 'i', 0x0300, 0x00EC },  // ì

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
    Status status;
    PinyinRecord record;
    size_t count = pinyinDB.Count();
    for (size_t i = 0; i < count; i++) {
        pinyinDB.GetRecord(i, record);
        ComposeFailure composeFailure = ComposeFailure::None;
        uint16_t defaultGlyphIndex = 0;
        status = __addPinyinGlyph(
            record.charcode, record.pinyin[0], 0, true,
            composeFailure, defaultGlyphIndex);
        if (status == kNotFound) {
            synthesisStats_.SourceHanMissing++;
            continue;
        } else if (status == kOk) {
            glyphCountAddOK_++;
            if (usedDotlessI_) {
                synthesisStats_.DotlessIGenerated++;
            } else if (usedGeneratedMark_) {
                synthesisStats_.ToneFallbackGenerated++;
            } else {
                synthesisStats_.SourceOnlyGenerated++;
            }
        } else {
            glyphCountAddFailed_++;
            if (composeFailure == ComposeFailure::DotlessI) {
                synthesisStats_.DotlessIFailed++;
            } else if (composeFailure == ComposeFailure::Component) {
                synthesisStats_.ComponentFailed++;
            } else {
                synthesisStats_.OtherFailed++;
            }
        }

        if (status != kOk) continue;
        size_t readingCount = 0;
        while (readingCount < 4 && !record.pinyin[readingCount].empty())
            readingCount++;
        if (readingCount < 2) continue;

        uint16_t atGlyphIndex = font_.CharToGlyphIndex('@');
        for (size_t readingIndex = 0; readingIndex < readingCount;
             readingIndex++) {
            uint16_t digitGlyphIndex =
                font_.CharToGlyphIndex((uint32_t)('1' + readingIndex));
            if (atGlyphIndex == 0 || digitGlyphIndex == 0) {
                synthesisStats_.SelectorMissingInputOmissions++;
                continue;
            }

            uint16_t selectedGlyphIndex = defaultGlyphIndex;
            if (readingIndex > 0) {
                ComposeFailure alternateFailure = ComposeFailure::None;
                Status alternateStatus = __addPinyinGlyph(
                    record.charcode, record.pinyin[readingIndex],
                    readingIndex, false, alternateFailure, selectedGlyphIndex);
                if (alternateStatus != kOk) {
                    synthesisStats_.AlternateSynthesisOmissions++;
                    continue;
                }
                synthesisStats_.AlternateGlyphsGenerated++;
            }

            std::vector<uint16_t> components;
            components.push_back(defaultGlyphIndex);
            components.push_back(atGlyphIndex);
            components.push_back(digitGlyphIndex);
            if (font_.AddLigatureSubstitution(
                    components, selectedGlyphIndex) == kOk) {
                synthesisStats_.SelectorLigaturesGenerated++;
            } else {
                synthesisStats_.AlternateSynthesisOmissions++;
            }
        }
    }
    return kOk;
}

Status PinyinFontBuilder::__retainSourceCmap()
{
    char2index_.clear();
    const std::vector<CmapSequentialMapGroup> &groups = font_.CmapGroups();
    for (size_t i = 0; i < groups.size(); i++) {
        const CmapSequentialMapGroup &group = groups[i];
        for (uint32_t charcode = group.startCharCode; charcode <= group.endCharCode; charcode++) {
            uint32_t glyphID = group.startGlyphID + (charcode - group.startCharCode);
            if (glyphID > 0 && glyphID < (uint32_t)font_.GlyphCount()) {
                char2index_[charcode] = (uint16_t)glyphID;
            }
            if (charcode == 0xFFFFFFFFu) {
                break;
            }
        }
    }
    return kOk;
}

static const uint32_t commonPunctuation[] = {
    // Half-width punctuation.
    0x0021, // !
    0x0022, // "
    0x0027, // '
    0x0028, // (
    0x0029, // )
    0x002C, // ,
    0x002D, // -
    0x002E, // .
    0x003A, // :
    0x003B, // ;
    0x003F, // ?
    0x005B, // [
    0x005D, // ]
    0x007B, // {
    0x007D, // }

    // Typographic punctuation.
    0x00B7, // ·
    0x2013, // –
    0x2014, // —
    0x2018, // ‘
    0x2019, // ’
    0x201C, // “
    0x201D, // ”
    0x2026, // …

    // CJK punctuation.
    0x3001, // 、
    0x3002, // 。
    0x3008, // 〈
    0x3009, // 〉
    0x300A, // 《
    0x300B, // 》
    0x300C, // 「
    0x300D, // 」
    0x300E, // 『
    0x300F, // 』
    0x3010, // 【
    0x3011, // 】
    0x3014, // 〔
    0x3015, // 〕
    0x30FB, // ・

    // Full-width counterparts.
    0xFF01, // ！
    0xFF02, // ＂
    0xFF07, // ＇
    0xFF08, // （
    0xFF09, // ）
    0xFF0C, // ，
    0xFF0D, // －
    0xFF0E, // ．
    0xFF1A, // ：
    0xFF1B, // ；
    0xFF1F, // ？
    0xFF3B, // ［
    0xFF3D, // ］
    0xFF5B, // ｛
    0xFF5D, // ｝
};

bool PinyinFontBuilder::__hasOutline(const OpenType_GlyphHeader *glyph) const
{
    if (glyph == NULL) return false;
    if (glyph->NumberOfContours >= 0) {
        const OpenType_GlyphSimple *simple =
            (const OpenType_GlyphSimple*)glyph;
        return !simple->Points.empty();
    }
    const OpenType_GlyphComposite *composite =
        (const OpenType_GlyphComposite*)glyph;
    return !composite->SubGlyphs.empty();
}

Status PinyinFontBuilder::__addScaledPunctuationGlyphs()
{
    std::map<uint16_t, uint16_t> wrappers;
    size_t count = sizeof(commonPunctuation) / sizeof(commonPunctuation[0]);
    for (size_t i = 0; i < count; i++) {
        uint32_t charcode = commonPunctuation[i];
        uint16_t sourceGlyphIndex = font_.CharToGlyphIndex(charcode);
        if (sourceGlyphIndex == 0) continue;

        std::map<uint16_t, uint16_t>::const_iterator cached =
            wrappers.find(sourceGlyphIndex);
        if (cached != wrappers.end()) {
            char2index_[charcode] = cached->second;
            continue;
        }

        const OpenType_GlyphHeader *sourceGlyph = NULL;
        if (font_.Glyph(sourceGlyphIndex, &sourceGlyph) != kOk ||
            !__hasOutline(sourceGlyph)) {
            continue;
        }

        OpenType_LongHorMetric metric = { 0 };
        if (font_.GlyphHorMetric(sourceGlyphIndex, metric) != kOk) {
            continue;
        }

        BoundingBox bbox = {
            sourceGlyph->XMin, sourceGlyph->YMin,
            sourceGlyph->XMax, sourceGlyph->YMax
        };
        OpenType_GlyphComposite glyph = {};
        glyph.NumberOfContours = -1;
        int16_t dx =
            (int16_t)(metric.AdvanceWidth * (1.0 - baseRatio_) / 2);
        int16_t baseScale =
            (int16_t)(baseRatio_ * OpenType_F2Dot14Scale);
        __addSubGlyph(
            glyph, sourceGlyphIndex, bbox, baseScale, baseScale,
            dx, baseDY_, true);

        char nameBuf[24] = { 0 };
        snprintf(
            nameBuf, sizeof(nameBuf), "uni%04X_punct",
            (unsigned int)charcode);
        OpenType_GlyphName name;
        name.ID = 258;
        name.Str = nameBuf;
        metric.LSB = glyph.XMin;

        uint16_t glyphIndex = 0;
        if (font_.AddGlyph(&glyph, &metric, name, glyphIndex) != kOk) {
            continue;
        }
        wrappers[sourceGlyphIndex] = glyphIndex;
        char2index_[charcode] = glyphIndex;
    }
    return kOk;
}

Status PinyinFontBuilder::__addPinyinGlyph(
    uint32_t charcode,
    const std::wstring &pinyin,
    size_t readingIndex,
    bool mapped,
    ComposeFailure &composeFailure,
    uint16_t &glyphIndex)
{
    glyphIndex = 0;
    composeFailure = ComposeFailure::None;
    usedGeneratedMark_ = false;
    usedDotlessI_ = false;

    uint32_t baseGlyphIndex = font_.CharToGlyphIndex(charcode);
    if (baseGlyphIndex == 0) {
        return kNotFound;
    }
    const OpenType_GlyphHeader *baseGlyph = NULL;
    font_.Glyph(baseGlyphIndex, &baseGlyph);
    assert(baseGlyph != NULL);
    OpenType_LongHorMetric baseHmtx = { 0 };
    font_.GlyphHorMetric(baseGlyphIndex, baseHmtx);
    BoundingBox baseBBox;
    baseBBox.XMin = baseGlyph->XMin;
    baseBBox.YMin = baseGlyph->YMin;
    baseBBox.XMax = baseGlyph->XMax;
    baseBBox.YMax = baseGlyph->YMax;

    int16_t baseDX = (int16_t)(baseHmtx.AdvanceWidth * (1.0 - baseRatio_) / 2);
    OpenType_GlyphComposite &glyph = glyph_;
    glyph.NumberOfContours = -1;
    glyph.XMin = 0;
    glyph.XMax = 0;
    glyph.YMin = 0;
    glyph.YMax = 0;
    glyph.SubGlyphs.resize(0);

    std::vector<GlyphInfo> &pinyinGlyphs = pinyinGlyphInfos_;
    pinyinGlyphs.resize(0);
    composeFailure = __composePinyin(pinyin, pinyinGlyphs);
    if (composeFailure != ComposeFailure::None) {
        return kError;
    }

    std::vector<PinyinHorizontalComponent> horizontalComponents;
    horizontalComponents.reserve(pinyinGlyphs.size());
    for (size_t i = 0; i < pinyinGlyphs.size(); i++) {
        PinyinHorizontalComponent component = {
            pinyinGlyphs[i].BBox.XMin,
            pinyinGlyphs[i].BBox.XMax,
            pinyinGlyphs[i].OffsetX
        };
        horizontalComponents.push_back(component);
    }
    PinyinHorizontalLayout horizontalLayout = {};
    if (!ResolvePinyinHorizontalLayout(
            horizontalComponents, baseHmtx.AdvanceWidth,
            pinyinRatio_, horizontalLayout)) {
        composeFailure = ComposeFailure::Other;
        return kError;
    }
    int16_t pinyinScaleY =
        (int16_t)(pinyinRatio_ * OpenType_F2Dot14Scale);

    for (size_t i = 0; i < pinyinGlyphs.size(); i++) {
        const GlyphInfo &info = pinyinGlyphs[i];
        __addSubGlyph(glyph, info.GlyphIndex, info.BBox,
            horizontalLayout.ScaleX, pinyinScaleY,
            horizontalLayout.ComponentOffsetsX[i],
            (int16_t)(pinyinDY_ + info.OffsetY * pinyinRatio_), false);
    }

    int16_t baseScale =
        (int16_t)(baseRatio_ * OpenType_F2Dot14Scale);
    __addSubGlyph(glyph, baseGlyphIndex, baseBBox,
        baseScale, baseScale, baseDX, baseDY_, true);

    char nameBuf[24] = { 0 };
    snprintf(nameBuf, sizeof(nameBuf), "uni%04X_py%02u",
        (unsigned int)charcode, (unsigned int)readingIndex);
    OpenType_GlyphName name;
    name.ID = 258;
    name.Str = nameBuf;

    baseHmtx.LSB = glyph.XMin;

    Status status = font_.AddGlyph(&glyph, &baseHmtx, name, glyphIndex);
    if (status != kOk) {
        return status;
    }

    if (mapped) char2index_[charcode] = glyphIndex;

    return kOk;
}

void PinyinFontBuilder::__addSubGlyph(
    OpenType_GlyphComposite &glyph, uint16_t glyphIndex,
    const BoundingBox &bbox, int16_t scaleX, int16_t scaleY,
    int16_t dx, int16_t dy, bool isLastOne)
{
    OpenType_GlyphComponent c = { 0 };
    c.Flags = OpenType_FlagArgsAreXYValues |
        OpenType_FlagUnscaledComponentOffset;
    c.Flags |= scaleX == scaleY
        ? OpenType_FlagWeHaveAScale
        : OpenType_FlagWeHaveAnXAndYScale;
    if (!isLastOne) {
        c.Flags |= OpenType_FlagMoreComponents;
    }
    if (dx > 127 || dx < -128 || dy > 127 || dy < -128) {
        c.Flags |= OpenType_FlagArg1And2AreWords;
    }
    c.Arg1 = dx;
    c.Arg2 = dy;
    c.Transform[0] = scaleX;
    c.Transform[3] = scaleY;
    c.GlyphIndex = glyphIndex;
    glyph.SubGlyphs.push_back(c);

    BoundingBox newBBox;
    newBBox.XMin = (int16_t)((int64_t)bbox.XMin * scaleX /
        OpenType_F2Dot14Scale + dx);
    newBBox.YMin = (int16_t)((int64_t)bbox.YMin * scaleY /
        OpenType_F2Dot14Scale + dy);
    newBBox.XMax = (int16_t)((int64_t)bbox.XMax * scaleX /
        OpenType_F2Dot14Scale + dx);
    newBBox.YMax = (int16_t)((int64_t)bbox.YMax * scaleY /
        OpenType_F2Dot14Scale + dy);
    bool firstOne = (glyph.SubGlyphs.size() == 1);
    if (firstOne || glyph.XMin > newBBox.XMin)
        glyph.XMin = newBBox.XMin;
    if (firstOne || glyph.YMin > newBBox.YMin)
        glyph.YMin = newBBox.YMin;
    if (firstOne || glyph.XMax < newBBox.XMax)
        glyph.XMax = newBBox.XMax;
    if (firstOne || glyph.YMax < newBBox.YMax)
        glyph.YMax = newBBox.YMax;
}

PinyinFontBuilder::ComposeFailure PinyinFontBuilder::__composePinyin(
    const std::wstring &pinyin, std::vector<GlyphInfo> &glyphs)
{
    glyphs.clear();
    int32_t cursor = 0;

    // 3 kinds of cluster:
    // - ['u']
    // - ['u', u+0304]
    // - ['u', u+0308, u+0304]
    wchar_t cluster[3] = { 0 };
    for (size_t i = 0; i < pinyin.size(); i++) {
        wchar_t c = pinyin[i];
        if (__isMarkChar(c)) {
            if (cluster[0] == 0) {
                return ComposeFailure::Other;
            } else if (cluster[1] == 0) {
                cluster[1] = c;
            } else if (cluster[2] == 0) {
                cluster[2] = c;
            } else {
                return ComposeFailure::Other;
            }
        } else {
            // previous cluster ended
            ComposeFailure failure =
                __composeCluster(cluster, glyphs, cursor);
            if (failure != ComposeFailure::None) return failure;
            // start a new cluster
            cluster[0] = c;
            cluster[1] = cluster[2] = 0;
        }
    }
    // last cluster
    ComposeFailure failure = __composeCluster(cluster, glyphs, cursor);
    if (failure != ComposeFailure::None) return failure;
    if (glyphs.empty()) return ComposeFailure::Other;
    return ComposeFailure::None;
}

PinyinFontBuilder::ComposeFailure PinyinFontBuilder::__composeCluster(
    const wchar_t cluster[3], std::vector<GlyphInfo> &glyphs, int32_t &x)
{
    if (cluster[0] == 0) {
        return ComposeFailure::None;
    }

    wchar_t baseChar = cluster[0];
    wchar_t marks[2] = { cluster[1], cluster[2] };
    while (marks[0] != 0) {
        uint64_t key = (uint64_t)baseChar << 32 | (uint64_t)marks[0];
        auto iter = substitutions_.find(key);
        if (iter == substitutions_.end()) {
            break;
        }
        baseChar = iter->second;
        marks[0] = marks[1];
        marks[1] = 0;
    }

    GlyphInfo info = {};
    int32_t hCenter;
    int16_t markY;
    const OpenType_GlyphHeader *pGlyph = NULL;

    PinyinComponentBounds componentBounds = {};
    bool useDotlessI = baseChar == 'i' && marks[0] != 0;
    if (useDotlessI) {
        PinyinComponentGlyph component = {};
        if (!components_ || components_->ResolveDotlessI(component) != kOk) {
            return ComposeFailure::DotlessI;
        }
        info.GlyphIndex = component.GlyphIndex;
        componentBounds = component.Bounds;
        usedDotlessI_ = component.Generated;
    } else {
        info.GlyphIndex = font_.CharToGlyphIndex(baseChar);
        if (info.GlyphIndex == 0) {
            return ComposeFailure::Component;
        }
        font_.Glyph(info.GlyphIndex, &pGlyph);
        componentBounds = { pGlyph->XMin, pGlyph->YMin, pGlyph->XMax, pGlyph->YMax };
    }
    info.BBox.XMin = componentBounds.XMin;
    info.BBox.YMin = componentBounds.YMin;
    info.BBox.XMax = componentBounds.XMax;
    info.BBox.YMax = componentBounds.YMax;
    OpenType_LongHorMetric metric = { 0 };
    if (font_.GlyphHorMetric(info.GlyphIndex, metric) != kOk ||
        metric.AdvanceWidth == 0 || x > INT16_MAX) {
        return ComposeFailure::Component;
    }
    info.OffsetX = (int16_t)x;
    info.OffsetY = 0;
    glyphs.push_back(info);

    hCenter = x + metric.AdvanceWidth / 2;
    markY = componentBounds.YMax + pinyinMarkVSpace_;

    x += metric.AdvanceWidth;

    for (size_t i = 0; i < 2 && marks[i] != 0; i++) {
        int16_t markHeight = 0;
        ComposeFailure failure =
            __appendMarkGlyph(marks[i], hCenter, markY, glyphs, markHeight);
        if (failure != ComposeFailure::None) return failure;
        if (i == 0 && marks[1] != 0) {
            markY += markHeight + pinyinMarkVSpace_;
        }
    }

    return ComposeFailure::None;
}

PinyinFontBuilder::ComposeFailure PinyinFontBuilder::__appendMarkGlyph(
    wchar_t mark, int32_t hCenter, int16_t y,
    std::vector<GlyphInfo> &glyphs, int16_t &markHeight)
{
    GlyphInfo info = {};
    PinyinComponentGlyph component = {};
    if (!components_ || components_->ResolveMark(mark, component) != kOk) {
        return ComposeFailure::Component;
    }

    info.GlyphIndex = component.GlyphIndex;
    info.BBox.XMin = component.Bounds.XMin;
    info.BBox.YMin = component.Bounds.YMin;
    info.BBox.XMax = component.Bounds.XMax;
    info.BBox.YMax = component.Bounds.YMax;
    int32_t markOffset = hCenter -
        ((int32_t)component.Bounds.XMin + component.Bounds.XMax) / 2;
    if (markOffset < INT16_MIN || markOffset > INT16_MAX) {
        return ComposeFailure::Component;
    }
    info.OffsetX = (int16_t)markOffset;
    info.OffsetY = y - component.Bounds.YMin;
    glyphs.push_back(info);
    markHeight = component.Bounds.YMax - component.Bounds.YMin;
    usedGeneratedMark_ = usedGeneratedMark_ || component.Generated;
    return ComposeFailure::None;
}

Status PinyinFontBuilder::__updateCmap()
{
    std::vector<CmapSequentialMapGroup> groups;
    CmapSequentialMapGroup group = { 0 };
    bool hasGroup = false;
    for (auto iter = char2index_.begin(); iter != char2index_.end(); iter++) {
        uint32_t charcode = iter->first;
        uint16_t glyphID = iter->second;
        if (!hasGroup) {
            group.startCharCode = charcode;
            group.endCharCode = charcode;
            group.startGlyphID = glyphID;
            hasGroup = true;
        } else if (charcode != group.endCharCode + 1 ||
            glyphID != (group.startGlyphID + (group.endCharCode - group.startCharCode) + 1)) {
            // current group ended
            groups.push_back(group);
            // start a new group
            group.startCharCode = charcode;
            group.endCharCode = charcode;
            group.startGlyphID = glyphID;
        } else {
            group.endCharCode++;
        }
    }
    // last group
    if (hasGroup) {
        groups.push_back(group);
    }

    return font_.SetCmap(groups);
}
