#ifndef __PINYIN_FONT_PINYIN_FONT_BUILDER_H__
#define __PINYIN_FONT_PINYIN_FONT_BUILDER_H__

#include "status.h"
#include "ot_font.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

//------------------------------------------------------------------------------

class PinyinDB;

class PinyinFontBuilder
{
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
    double baseRatio_;
    double pinyinRatio_;
    int16_t pinyinCharSpace_;
    int16_t pinyinMarkVSpace_;
    int16_t pinyinCharYMin_;
    int16_t baseDY_;
    int16_t pinyinDY_;
    std::unordered_map<uint64_t, wchar_t> substitutions_;

    std::map<wchar_t, uint16_t> char2index_;

    OpenType_GlyphComposite glyph_;
    std::vector<glyphInfo> pinyinGlyphInfos_;

    uint16_t glyphCountOld_;
    uint16_t glyphCountAddOK_;
    uint16_t glyphCountAddFailed_;
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

private:
    int16_t __calcPinyinCharYMin();
    bool __checkRequiredGlyphs();
    bool __isMarkChar(wchar_t c);
    bool __alternativeChar(wchar_t &c);
    void __buildSubstitutions();
    Status __addPinyinGlyphs(const PinyinDB &pinyinDB);
    Status __addPinyinGlyph(uint32_t charcode, const std::wstring &pinyin);
    void __addSubGlyph(
        OpenType_GlyphComposite &glyph, 
        uint16_t glyphIndex, const boundingBox &bbox, double scale, int16_t dx, int16_t dy, bool isLastOne);
    bool __composePinyin(
        const std::wstring &pinyin, std::vector<glyphInfo> &glyphs, int16_t &totalWidth);
    bool __composeCluster(
        wchar_t cluster[3], std::vector<glyphInfo> &glyphs, int16_t &x);
    Status __updateCmap();
    Status __retainCommonGlyphs();
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_FONT_BUILDER_H__
