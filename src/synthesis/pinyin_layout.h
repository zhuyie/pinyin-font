#ifndef __PINYIN_FONT_PINYIN_LAYOUT_H__
#define __PINYIN_FONT_PINYIN_LAYOUT_H__

#include "ot_font.h"
#include <cstdint>
#include <vector>

struct PinyinVerticalBand {
    int16_t YMin;
    int16_t YMax;
};

PinyinVerticalBand SelectPinyinVerticalBand(
    const OpenType_OS2 &os2,
    const OpenType_Hhea &hhea,
    const OpenType_Head &head);

struct PinyinHorizontalComponent {
    int16_t XMin;
    int16_t XMax;
    int32_t OffsetX;
};

struct PinyinHorizontalLayout {
    int16_t ScaleX;
    std::vector<int16_t> ComponentOffsetsX;
};

bool ResolvePinyinHorizontalLayout(
    const std::vector<PinyinHorizontalComponent> &components,
    uint16_t advanceWidth,
    double scaleLimit,
    PinyinHorizontalLayout &layout);

#endif // !__PINYIN_FONT_PINYIN_LAYOUT_H__
