#ifndef __PINYIN_FONT_PINYIN_FONT_BUILDER_H__
#define __PINYIN_FONT_PINYIN_FONT_BUILDER_H__

#include "status.h"
#include "ot_font.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

//------------------------------------------------------------------------------

class PinyinDB;
class PinyinComponents;

struct PinyinSynthesisStats {
    uint32_t SourceHanMissing = 0;
    uint32_t SourceOnlyGenerated = 0;
    uint32_t ToneFallbackGenerated = 0;
    uint32_t DotlessIGenerated = 0;
    uint32_t ComponentFailed = 0;
    uint32_t DotlessIFailed = 0;
    uint32_t OtherFailed = 0;
    uint32_t AlternateGlyphsGenerated = 0;
    uint32_t SelectorLigaturesGenerated = 0;
    uint32_t SelectorMissingInputOmissions = 0;
    uint32_t AlternateSynthesisOmissions = 0;
};

struct PinyinComponentStats {
    uint32_t MacronUses = 0;
    uint32_t AcuteUses = 0;
    uint32_t CaronUses = 0;
    uint32_t GraveUses = 0;
    uint32_t DiaeresisUses = 0;
    uint32_t DotlessIUses = 0;
};

class PinyinFontBuilder
{
    enum class ComposeFailure {
        None,
        Component,
        DotlessI,
        Other,
    };

    typedef struct {
        int16_t XMin;
        int16_t YMin;
        int16_t XMax;
        int16_t YMax;
    } boundingBox;
    typedef struct {
        uint16_t GlyphIndex;
        boundingBox BBox;
        int16_t OffsetX;
        int16_t OffsetY;
        int16_t AdvanceWidth;
    } glyphInfo;

    OpenType_Font font_;
    std::unique_ptr<PinyinComponents> components_;
    double baseRatio_;
    double pinyinRatio_;
    int16_t pinyinCharSpace_;
    int16_t pinyinMarkVSpace_;
    int16_t pinyinCharYMin_;
    int16_t baseDY_;
    int16_t pinyinDY_;
    std::unordered_map<uint64_t, wchar_t> substitutions_;

    std::map<uint32_t, uint16_t> char2index_;

    OpenType_GlyphComposite glyph_;
    std::vector<glyphInfo> pinyinGlyphInfos_;

    uint16_t glyphCountOld_;
    uint16_t glyphCountAddOK_;
    uint16_t glyphCountAddFailed_;
    PinyinSynthesisStats synthesisStats_;
    bool usedGeneratedMark_;
    bool usedDotlessI_;
    uint32_t parseTime_;
    uint32_t synthesisTime_;
    uint32_t writeTime_;

public:
    PinyinFontBuilder();
    ~PinyinFontBuilder();

    Status Build(
        const char *sourceFont, 
        const PinyinDB &pinyinDB);
    Status Build(
        const char *sourceFont,
        const char *outputFont,
        const PinyinDB &pinyinDB);
    void GetStats(
        uint16_t &glyphCountOld, 
        uint16_t &glyphCountAddOK, 
        uint16_t &glyphCountAddFailed,
        uint32_t &parseTime, 
        uint32_t &synthesisTime, 
        uint32_t &writeTime);
    const PinyinSynthesisStats &GetSynthesisStats() const { return synthesisStats_; }
    void GetComponentStats(PinyinComponentStats &stats) const;

private:
    int16_t __calcPinyinCharYMin();
    bool __checkRequiredGlyphs();
    bool __isMarkChar(wchar_t c);
    void __buildSubstitutions();
    Status __retainSourceCmap();
    Status __addScaledPunctuationGlyphs();
    bool __hasOutline(const OpenType_GlyphHeader *glyph) const;
    Status __addPinyinGlyphs(const PinyinDB &pinyinDB);
    Status __addPinyinGlyph(
        uint32_t charcode,
        const std::wstring &pinyin,
        size_t readingIndex,
        bool mapped,
        ComposeFailure &composeFailure,
        uint16_t &glyphIndex);
    void __addSubGlyph(
        OpenType_GlyphComposite &glyph, 
        uint16_t glyphIndex, const boundingBox &bbox, double scale, int16_t dx, int16_t dy, bool isLastOne);
    ComposeFailure __composePinyin(
        const std::wstring &pinyin, std::vector<glyphInfo> &glyphs, int16_t &totalWidth);
    ComposeFailure __composeCluster(
        const wchar_t cluster[3], std::vector<glyphInfo> &glyphs, int16_t &x);
    ComposeFailure __appendMarkGlyph(
        wchar_t mark, int16_t hCenter, int16_t y,
        std::vector<glyphInfo> &glyphs, int16_t &markHeight);
    Status __updateCmap();
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_FONT_BUILDER_H__
