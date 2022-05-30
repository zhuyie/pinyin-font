#ifndef __PINYIN_FONT_PINYIN_FONT_BUILDER_H__
#define __PINYIN_FONT_PINYIN_FONT_BUILDER_H__

#include "status.h"
#include "ot_font.h"
#include <string>

//------------------------------------------------------------------------------

class PinyinFontBuilder
{
    typedef struct {
        uint16_t GlyphIndex;
        int16_t OffsetX;
        int16_t OffsetY;
        int16_t AdvanceWidth;
    } glyphInfo;

    OpenType_Font font_;
    int16_t charSpace_;
    int16_t pinyinCharYMin_;

public:
    PinyinFontBuilder();
    ~PinyinFontBuilder();

    Status Build(const char *sourceFont);

private:
    int16_t __calcPinyinCharYMin();
    Status __checkRequiredGlyphs();
    Status __AddPinyinGlyphs();
    Status __AddPinyinGlyph(uint32_t charcode, const std::wstring &pinyin, double baseRatio);
    void __AddSubGlyph(OpenType_GlyphComposite &glyph, 
        uint16_t glyphIndex, double scale, int16_t dx, int16_t dy, bool isLastOne);
    bool __ComposePinyin(
        const std::wstring &pinyin, std::vector<glyphInfo> &glyphs, int16_t &totalWidth);
    bool __ComposeCluster(
        wchar_t cluster[3], std::vector<glyphInfo> &glyphs, int16_t &x);
    bool __IsMarkChar(wchar_t c) {
        return (c == 0x0304) || (c == 0x0301) || (c == 0x030C) || (c == 0x0300) || (c == 0x0308);
    }
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_FONT_BUILDER_H__
