#include "pinyin_layout.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>

static bool isValidVerticalBand(int16_t yMin, int16_t yMax)
{
    return yMax > 0 && yMin <= 0 && yMax > yMin;
}

PinyinVerticalBand SelectPinyinVerticalBand(
    const OpenType_OS2 &os2,
    const OpenType_Hhea &hhea,
    const OpenType_Head &head)
{
    if (isValidVerticalBand(os2.sTypoDescender, os2.sTypoAscender)) {
        return { os2.sTypoDescender, os2.sTypoAscender };
    }
    if (isValidVerticalBand(hhea.Descender, hhea.Ascender)) {
        return { hhea.Descender, hhea.Ascender };
    }
    return { head.YMin, head.YMax };
}

bool ResolvePinyinHorizontalLayout(
    const std::vector<PinyinHorizontalComponent> &components,
    uint16_t advanceWidth,
    double scaleLimit,
    PinyinHorizontalLayout &layout)
{
    if (components.empty() || advanceWidth == 0 || scaleLimit <= 0) {
        return false;
    }

    int32_t inkMin = std::numeric_limits<int32_t>::max();
    int32_t inkMax = std::numeric_limits<int32_t>::min();
    for (size_t i = 0; i < components.size(); i++) {
        inkMin = std::min(inkMin,
            (int32_t)components[i].XMin + components[i].OffsetX);
        inkMax = std::max(inkMax,
            (int32_t)components[i].XMax + components[i].OffsetX);
    }
    int32_t inkWidth = inkMax - inkMin;
    if (inkWidth <= 0) return false;

    double scaleCap = std::min(
        scaleLimit, (double)advanceWidth / inkWidth);
    int32_t scaleX =
        (int32_t)std::floor(scaleCap * OpenType_F2Dot14Scale);
    layout.ComponentOffsetsX.resize(components.size());

    while (scaleX > 0) {
        int32_t scaledMin = std::numeric_limits<int32_t>::max();
        int32_t scaledMax = std::numeric_limits<int32_t>::min();
        for (size_t i = 0; i < components.size(); i++) {
            int32_t dx = (int32_t)std::lround(
                (double)components[i].OffsetX * scaleX /
                    OpenType_F2Dot14Scale);
            if (dx < INT16_MIN || dx > INT16_MAX) return false;
            layout.ComponentOffsetsX[i] = (int16_t)dx;
            scaledMin = std::min(scaledMin,
                (int32_t)((int64_t)components[i].XMin * scaleX /
                    OpenType_F2Dot14Scale) + dx);
            scaledMax = std::max(scaledMax,
                (int32_t)((int64_t)components[i].XMax * scaleX /
                    OpenType_F2Dot14Scale) + dx);
        }

        if (scaledMax - scaledMin <= advanceWidth) {
            int32_t targetMin =
                ((int32_t)advanceWidth - (scaledMax - scaledMin)) / 2;
            int32_t shift = targetMin - scaledMin;
            bool offsetsFit = true;
            for (size_t i = 0; i < layout.ComponentOffsetsX.size(); i++) {
                int32_t shifted =
                    (int32_t)layout.ComponentOffsetsX[i] + shift;
                if (shifted < INT16_MIN || shifted > INT16_MAX) {
                    offsetsFit = false;
                    break;
                }
                layout.ComponentOffsetsX[i] = (int16_t)shifted;
            }
            if (offsetsFit) {
                layout.ScaleX = (int16_t)scaleX;
                return true;
            }
        }
        scaleX--;
    }
    return false;
}
