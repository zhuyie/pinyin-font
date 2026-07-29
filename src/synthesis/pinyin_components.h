#ifndef PINYIN_FONT_PINYIN_COMPONENTS_H
#define PINYIN_FONT_PINYIN_COMPONENTS_H

#include "ot_font.h"
#include "status.h"
#include <cstdint>
#include <map>

struct PinyinComponentBounds {
    int16_t XMin;
    int16_t YMin;
    int16_t XMax;
    int16_t YMax;
};

struct PinyinComponentGlyph {
    uint16_t GlyphIndex;
    PinyinComponentBounds Bounds;
    bool Generated;
};

enum PinyinDotlessIStatus {
    kPinyinDotlessIUnknown = 0,
    kPinyinDotlessIReady,
    kPinyinDotlessIUnavailable
};

class PinyinComponents
{
public:
    explicit PinyinComponents(OpenType_Font &font);

    int16_t XHeight() const { return xHeight_; }
    int16_t MarkGap() const { return markGap_; }

    Status ResolveMark(wchar_t mark, PinyinComponentGlyph &component);
    Status ResolveDotlessI(PinyinComponentGlyph &component);

    uint32_t GeneratedMarkUseCount(wchar_t mark) const;
    uint32_t DotlessIUseCount() const { return dotlessIUseCount_; }
    PinyinDotlessIStatus DotlessIStatus() const { return dotlessIStatus_; }

    static bool DeriveDotlessI(
        const OpenType_GlyphSimple &source,
        int16_t xHeight,
        OpenType_GlyphSimple &derived);
    static bool CalculateBounds(
        const std::vector<OpenType_GlyphPoint> &points,
        PinyinComponentBounds &bounds);

private:
    OpenType_Font &font_;
    int16_t xHeight_;
    int16_t markGap_;
    std::map<wchar_t, uint16_t> generatedMarks_;
    std::map<wchar_t, uint32_t> generatedMarkUseCounts_;
    PinyinDotlessIStatus dotlessIStatus_;
    uint16_t dotlessIGlyphIndex_;
    uint32_t dotlessIUseCount_;

    int16_t resolveXHeight() const;
    Status createMark(wchar_t mark, uint16_t &glyphIndex);
    Status addSimpleGlyph(
        const OpenType_GlyphSimple &glyph,
        const OpenType_LongHorMetric &metric,
        const char *name,
        uint16_t &glyphIndex);
    static bool alternativeMark(wchar_t mark, wchar_t &alternative);
    static void addContour(
        OpenType_GlyphSimple &glyph,
        const int16_t coordinates[][2],
        size_t count);
    static void finalizeGlyphBounds(OpenType_GlyphSimple &glyph);
};

#endif
