#ifndef __PINYIN_FONT_PINYIN_FONT_BUILDER_H__
#define __PINYIN_FONT_PINYIN_FONT_BUILDER_H__

#include "status.h"
#include "ot_font.h"
#include <string>

//------------------------------------------------------------------------------

class PinyinFontBuilder
{
    OpenType_Font font_;

public:
    PinyinFontBuilder();
    ~PinyinFontBuilder();

    Status Build(const char *sourceFont);

private:
    Status __checkRequiredGlyphs();
    Status __AddPinyinGlyphs();
    Status __AddPinyinGlyph(uint32_t charcode, const std::wstring &pinyin, double baseRatio);
    void __AddSubGlyph(OpenType_GlyphComposite &glyph, uint16_t glyphIndex, double scale, int16_t dx, int16_t dy, bool isLastOne);
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_FONT_BUILDER_H__
