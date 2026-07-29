#include <algorithm>
#include <string>
#include <vector>

#include "test_font_fixture.h"
#include "ot_font_builder.h"
#include "ot_font_writer.h"

static OpenType_GlyphSimple rectangle(int16_t width, int16_t height)
{
    OpenType_GlyphSimple glyph = {};
    const int16_t coordinates[][2] = {
        { 0, 0 }, { width, 0 }, { width, height }, { 0, height }
    };
    for (size_t i = 0; i < 4; i++) {
        OpenType_GlyphPoint point = {};
        point.Flags = OpenType_FlagOnCurve;
        point.X = coordinates[i][0];
        point.Y = coordinates[i][1];
        glyph.Points.push_back(point);
    }
    glyph.EndPtsOfContours.push_back(3);
    glyph.NumberOfContours = 1;
    glyph.XMin = glyph.YMin = 0;
    glyph.XMax = width;
    glyph.YMax = height;
    return glyph;
}

static OpenType_GlyphSimple simpleI()
{
    OpenType_GlyphSimple glyph = rectangle(120, 500);
    const int16_t dot[][2] = {
        { 20, 650 }, { 100, 650 }, { 100, 750 }, { 20, 750 }
    };
    for (size_t i = 0; i < 4; i++) {
        OpenType_GlyphPoint point = {};
        point.Flags = OpenType_FlagOnCurve;
        point.X = dot[i][0];
        point.Y = dot[i][1];
        glyph.Points.push_back(point);
    }
    glyph.EndPtsOfContours.push_back(7);
    glyph.NumberOfContours = 2;
    glyph.YMax = 750;
    glyph.Instructions.push_back(0xB0);
    return glyph;
}

Status OpenType_TestFontFixture::Write(
    const char *path,
    const std::set<uint32_t> &extraCharacters,
    const std::set<uint32_t> &toneMarks,
    bool useSimpleI,
    bool useCompositeI,
    const std::set<uint32_t> &compositeCharacters,
    const std::set<uint32_t> &emptyCharacters,
    const std::map<uint32_t, uint32_t> &sharedMappings)
{
    OpenType_Font font;
    OpenType_Font::Builder builder(font);
    Status status = builder.InitializeTrueType(1000, 900, -250, 500);
    if (status != kOk) return status;

    OpenType_LongHorMetric metric = { 600, 0 };
    uint16_t glyphIndex = 0;
    OpenType_GlyphSimple notdef = rectangle(500, 700);
    status = builder.AddUnmappedSimpleGlyph(
        notdef, metric, ".notdef", glyphIndex);
    if (status != kOk) return status;

    std::set<uint32_t> characters = extraCharacters;
    static const uint32_t requiredHan[] = { 0x6C49, 0x8BED, 0x62FC, 0x97F3 };
    characters.insert(requiredHan, requiredHan + 4);
    for (uint32_t c = 'A'; c <= 'Z'; c++) characters.insert(c);
    for (uint32_t c = 'a'; c <= 'z'; c++) characters.insert(c);
    characters.insert(toneMarks.begin(), toneMarks.end());

    uint16_t lowercaseLGlyphIndex = 0;
    std::map<uint32_t, uint16_t> mappedGlyphs;
    for (std::set<uint32_t>::const_iterator it = characters.begin();
         it != characters.end(); ++it) {
        uint32_t c = *it;
        if (c == 'i' && useCompositeI) continue;
        if (compositeCharacters.find(c) != compositeCharacters.end()) {
            if (lowercaseLGlyphIndex == 0) return kError;
            OpenType_GlyphComposite glyph = {};
            glyph.NumberOfContours = -1;
            glyph.XMin = glyph.YMin = 0;
            glyph.XMax = glyph.YMax = 500;
            OpenType_GlyphComponent component = {};
            component.Flags = OpenType_FlagArgsAreXYValues;
            component.GlyphIndex = lowercaseLGlyphIndex;
            glyph.SubGlyphs.push_back(component);
            status = builder.AddMappedCompositeGlyph(
                c, glyph, metric, "", glyphIndex);
        } else {
            OpenType_GlyphSimple glyph =
                emptyCharacters.find(c) != emptyCharacters.end()
                    ? OpenType_GlyphSimple()
                    : c == 'i' && useSimpleI ? simpleI() :
                    rectangle(c >= 0x300 && c <= 0x36F ? 180 : 500,
                              c >= 0x300 && c <= 0x36F ? 100 : 500);
            status = builder.AddMappedSimpleGlyph(
                c, glyph, metric, "", glyphIndex);
        }
        if (status != kOk) return status;
        mappedGlyphs[c] = glyphIndex;
        if (c == 'l') lowercaseLGlyphIndex = glyphIndex;
    }

    if (useCompositeI) {
        if (lowercaseLGlyphIndex == 0) return kError;
        OpenType_GlyphComposite composite = {};
        composite.NumberOfContours = -1;
        composite.XMin = composite.YMin = 0;
        composite.XMax = 500;
        composite.YMax = 500;
        OpenType_GlyphComponent component = {};
        component.Flags = OpenType_FlagArgsAreXYValues;
        component.GlyphIndex = lowercaseLGlyphIndex;
        composite.SubGlyphs.push_back(component);
        status = builder.AddMappedCompositeGlyph(
            'i', composite, metric, "", glyphIndex);
        if (status != kOk) return status;
        mappedGlyphs['i'] = glyphIndex;
    }

    status = builder.Finish();
    if (status != kOk) return status;

    if (!sharedMappings.empty()) {
        std::map<uint32_t, uint16_t> mappings;
        const std::vector<CmapSequentialMapGroup> &groups = font.CmapGroups();
        for (size_t i = 0; i < groups.size(); i++) {
            const CmapSequentialMapGroup &group = groups[i];
            for (uint32_t c = group.startCharCode; c <= group.endCharCode; c++) {
                mappings[c] = (uint16_t)(
                    group.startGlyphID + c - group.startCharCode);
                if (c == 0xFFFFFFFFu) break;
            }
        }
        for (std::map<uint32_t, uint32_t>::const_iterator
                 it = sharedMappings.begin(); it != sharedMappings.end(); ++it) {
            std::map<uint32_t, uint16_t>::const_iterator source =
                mappedGlyphs.find(it->second);
            if (source == mappedGlyphs.end()) return kInvalidArgs;
            mappings[it->first] = source->second;
        }

        std::vector<CmapSequentialMapGroup> replacement;
        for (std::map<uint32_t, uint16_t>::const_iterator it = mappings.begin();
             it != mappings.end(); ++it) {
            CmapSequentialMapGroup group = {
                it->first, it->first, it->second
            };
            replacement.push_back(group);
        }
        status = font.SetCmap(replacement);
        if (status != kOk) return status;
    }

    OpenType_Font_Writer writer;
    return writer.Write(path, &font);
}
