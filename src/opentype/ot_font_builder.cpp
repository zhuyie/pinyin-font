#include "ot_font_builder.h"
#include <algorithm>

OpenType_Font::Builder::Builder(OpenType_Font &font)
: font_(font), initialized_(false), finished_(false)
{
}

Status OpenType_Font::Builder::InitializeTrueType(
    uint16_t unitsPerEm,
    int16_t ascender,
    int16_t descender,
    int16_t xHeight)
{
    if (initialized_ || font_.GlyphCount() != 0 ||
        unitsPerEm < 16 || unitsPerEm > 16384 ||
        ascender <= descender || xHeight <= 0) {
        return kInvalidArgs;
    }

    font_.head_.Version = 0x00010000;
    font_.head_.MagicNumber = 0x5F0F3CF5;
    font_.head_.UnitsPerEm = unitsPerEm;
    font_.head_.IndexToLocFormat = 1;

    font_.maxp_.Version = 0x00010000;
    font_.maxp_.MaxZones = 1;
    font_.post_.Version = 0x00020000;

    font_.os2_.version = 4;
    font_.os2_.sxHeight = xHeight;
    font_.os2_.sCapHeight = ascender;
    font_.os2_.sTypoAscender = ascender;
    font_.os2_.sTypoDescender = descender;
    font_.os2_.usWinAscent = (uint16_t)std::max<int>(0, ascender);
    font_.os2_.usWinDescent = (uint16_t)std::max<int>(0, -descender);

    font_.hhea_.MajorVersion = 1;
    font_.hhea_.Ascender = ascender;
    font_.hhea_.Descender = descender;
    font_.hhea_.CaretSlopeRise = 1;

    initialized_ = true;
    return kOk;
}

Status OpenType_Font::Builder::addGlyph(
    const OpenType_GlyphHeader &glyph,
    const OpenType_LongHorMetric &metric,
    const std::string &name,
    uint16_t &glyphIndex)
{
    if (!initialized_ || finished_) return kError;
    OpenType_GlyphName glyphName;
    glyphName.ID = name.empty() ? 0 : 258;
    glyphName.Str = name;
    return font_.AddGlyph(&glyph, &metric, glyphName, glyphIndex);
}

Status OpenType_Font::Builder::addMapping(
    uint32_t charcode,
    uint16_t glyphIndex)
{
    if (charcode > 0x10FFFF || glyphIndex == 0 ||
        mappings_.find(charcode) != mappings_.end()) {
        return kInvalidArgs;
    }
    mappings_[charcode] = glyphIndex;
    return kOk;
}

Status OpenType_Font::Builder::AddMappedSimpleGlyph(
    uint32_t charcode,
    const OpenType_GlyphSimple &glyph,
    const OpenType_LongHorMetric &metric,
    const std::string &name,
    uint16_t &glyphIndex)
{
    Status status = addGlyph(glyph, metric, name, glyphIndex);
    return status == kOk ? addMapping(charcode, glyphIndex) : status;
}

Status OpenType_Font::Builder::AddMappedCompositeGlyph(
    uint32_t charcode,
    const OpenType_GlyphComposite &glyph,
    const OpenType_LongHorMetric &metric,
    const std::string &name,
    uint16_t &glyphIndex)
{
    Status status = addGlyph(glyph, metric, name, glyphIndex);
    return status == kOk ? addMapping(charcode, glyphIndex) : status;
}

Status OpenType_Font::Builder::AddUnmappedSimpleGlyph(
    const OpenType_GlyphSimple &glyph,
    const OpenType_LongHorMetric &metric,
    const std::string &name,
    uint16_t &glyphIndex)
{
    return addGlyph(glyph, metric, name, glyphIndex);
}

Status OpenType_Font::Builder::AddUnmappedCompositeGlyph(
    const OpenType_GlyphComposite &glyph,
    const OpenType_LongHorMetric &metric,
    const std::string &name,
    uint16_t &glyphIndex)
{
    return addGlyph(glyph, metric, name, glyphIndex);
}

Status OpenType_Font::Builder::Finish()
{
    if (!initialized_ || finished_ || font_.GlyphCount() == 0 ||
        mappings_.empty()) {
        return kError;
    }

    std::vector<CmapSequentialMapGroup> groups;
    for (std::map<uint32_t, uint16_t>::const_iterator iter = mappings_.begin();
         iter != mappings_.end(); ++iter) {
        if (!groups.empty() &&
            iter->first == groups.back().endCharCode + 1 &&
            iter->second == groups.back().startGlyphID +
                (groups.back().endCharCode - groups.back().startCharCode) + 1) {
            groups.back().endCharCode = iter->first;
        } else {
            CmapSequentialMapGroup group = {
                iter->first, iter->first, iter->second
            };
            groups.push_back(group);
        }
    }

    Status status = font_.SetCmap(groups);
    if (status != kOk) return status;
    font_.hhea_.NumberOfHMetrics = (uint16_t)font_.GlyphCount();
    finished_ = true;
    return kOk;
}
