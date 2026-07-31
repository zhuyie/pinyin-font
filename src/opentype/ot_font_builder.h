#ifndef PINYIN_FONT_OT_FONT_BUILDER_H
#define PINYIN_FONT_OT_FONT_BUILDER_H

#include "ot_font.h"
#include <map>
#include <string>

class OpenType_Font::Builder
{
public:
    explicit Builder(OpenType_Font &font);

    Status InitializeTrueType(
        uint16_t unitsPerEm,
        int16_t ascender,
        int16_t descender,
        int16_t xHeight);

    Status AddMappedSimpleGlyph(
        uint32_t charcode,
        const OpenType_GlyphSimple &glyph,
        const OpenType_LongHorMetric &metric,
        const std::string &name,
        uint16_t &glyphIndex);

    Status AddMappedCompositeGlyph(
        uint32_t charcode,
        const OpenType_GlyphComposite &glyph,
        const OpenType_LongHorMetric &metric,
        const std::string &name,
        uint16_t &glyphIndex);

    Status AddUnmappedSimpleGlyph(
        const OpenType_GlyphSimple &glyph,
        const OpenType_LongHorMetric &metric,
        const std::string &name,
        uint16_t &glyphIndex);

    Status AddUnmappedCompositeGlyph(
        const OpenType_GlyphComposite &glyph,
        const OpenType_LongHorMetric &metric,
        const std::string &name,
        uint16_t &glyphIndex);

    Status Finish();

private:
    OpenType_Font &font_;
    std::map<uint32_t, uint16_t> mappings_;
    bool initialized_;
    bool finished_;

    Status addGlyph(
        const OpenType_GlyphHeader &glyph,
        const OpenType_LongHorMetric &metric,
        const std::string &name,
        uint16_t &glyphIndex);
    Status addMapping(uint32_t charcode, uint16_t glyphIndex);
};

#endif
