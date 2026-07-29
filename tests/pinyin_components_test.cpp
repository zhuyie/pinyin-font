#include "pinyin_components.h"
#include "ot_font_parser.h"
#include "ot_font_writer.h"
#include "test_font_fixture.h"
#include <cstdio>
#include <set>
#include <string>

static void addRectangle(
    OpenType_GlyphSimple &glyph,
    int16_t xMin,
    int16_t yMin,
    int16_t xMax,
    int16_t yMax,
    bool clockwise = true)
{
    const int16_t points[][2] = {
        { xMin, yMin }, { xMax, yMin }, { xMax, yMax }, { xMin, yMax }
    };
    const int order[][4] = {
        { 0, 1, 2, 3 },
        { 0, 3, 2, 1 }
    };
    int direction = clockwise ? 0 : 1;
    for (size_t i = 0; i < 4; i++) {
        OpenType_GlyphPoint point = {};
        point.X = points[order[direction][i]][0];
        point.Y = points[order[direction][i]][1];
        point.Flags = OpenType_FlagOnCurve;
        glyph.Points.push_back(point);
    }
    glyph.EndPtsOfContours.push_back((uint16_t)(glyph.Points.size() - 1));
    glyph.NumberOfContours = (int16_t)glyph.EndPtsOfContours.size();
}

static bool expect(bool condition, const char *message)
{
    if (!condition) std::fprintf(stderr, "%s\n", message);
    return condition;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s output-directory\n", argv[0]);
        return 1;
    }
    OpenType_GlyphSimple simpleI = {};
    addRectangle(simpleI, 0, 0, 120, 500);
    addRectangle(simpleI, 20, 650, 100, 750);
    simpleI.Instructions.push_back(0xB0);

    OpenType_GlyphSimple derived;
    if (!expect(PinyinComponents::DeriveDotlessI(simpleI, 500, derived),
                "failed to derive dotless i from separable simple i") ||
        !expect(derived.NumberOfContours == 1, "dot contour was not removed") ||
        !expect(derived.YMax == 500, "derived bounds include the removed dot") ||
        !expect(derived.Instructions.empty(), "derived instructions were not cleared")) {
        return 1;
    }

    OpenType_GlyphSimple hollowDotI = {};
    addRectangle(hollowDotI, 0, 0, 120, 500);
    addRectangle(hollowDotI, -10, 640, 130, 790);
    addRectangle(hollowDotI, 25, 675, 95, 755, false);
    if (!expect(PinyinComponents::DeriveDotlessI(hollowDotI, 500, derived),
                "failed to treat a hollow dot as one visual group") ||
        !expect(derived.NumberOfContours == 1, "hollow dot contours were not removed together")) {
        return 1;
    }

    OpenType_GlyphSimple ambiguousI = {};
    addRectangle(ambiguousI, 0, 0, 120, 500);
    addRectangle(ambiguousI, 20, 650, 100, 750);
    addRectangle(ambiguousI, 25, 850, 95, 940);
    if (!expect(!PinyinComponents::DeriveDotlessI(ambiguousI, 500, derived),
                "ambiguous two-dot glyph should fail closed")) {
        return 1;
    }

    OpenType_GlyphSimple connectedI = {};
    addRectangle(connectedI, 0, 0, 120, 750);
    if (!expect(!PinyinComponents::DeriveDotlessI(connectedI, 500, derived),
                "connected i should fail closed")) {
        return 1;
    }

    std::string sourcePath = std::string(argv[1]) + "/component-fixture.ttf";
    std::string generatedPath = std::string(argv[1]) + "/component-generated.ttf";
    std::set<uint32_t> noExtra;
    std::set<uint32_t> sourceMarks;
    sourceMarks.insert(0x0304);
    if (!expect(OpenType_TestFontFixture::Write(
                    sourcePath.c_str(), noExtra, sourceMarks, true, false) == kOk,
                "failed to write minimal TrueType fixture")) {
        return 1;
    }

    OpenType_Font font;
    OpenType_Font_Parser sourceParser;
    if (!expect(sourceParser.Parse(sourcePath.c_str(), &font) == kOk,
                "failed to parse minimal TrueType fixture")) {
        return 1;
    }
    uint16_t sourceMacron = font.CharToGlyphIndex(0x0304);
    int sourceGlyphCount = font.GlyphCount();
    PinyinComponents components(font);
    if (!expect(components.MarkGap() >= components.XHeight() / 10,
                "font-relative mark gap is too small after pinyin scaling")) {
        return 1;
    }
    PinyinComponentGlyph component = {};
    if (!expect(components.ResolveMark(0x0304, component) == kOk &&
                    !component.Generated && component.GlyphIndex == sourceMacron,
                "source mark was not preferred") ||
        !expect(components.ResolveMark(0x0301, component) == kOk &&
                    component.Generated,
                "missing mark was not generated")) {
        return 1;
    }
    uint16_t generatedAcute = component.GlyphIndex;
    if (!expect(
            component.Bounds.YMax - component.Bounds.YMin >=
                components.XHeight() / 4,
            "generated acute is too short to survive pinyin scaling") ||
        !expect(
            component.Bounds.XMax - component.Bounds.XMin >=
                components.XHeight() / 4,
            "generated acute is too narrow to remain visible")) {
        return 1;
    }
    if (!expect(components.ResolveMark(0x0301, component) == kOk &&
                    component.GlyphIndex == generatedAcute &&
                    font.GlyphCount() == sourceGlyphCount + 1,
                "generated mark was not reused") ||
        !expect(font.CharToGlyphIndex(0x0301) == 0,
                "internal mark unexpectedly changed cmap")) {
        return 1;
    }
    PinyinComponentGlyph dotless = {};
    if (!expect(components.ResolveDotlessI(dotless) == kOk && dotless.Generated,
                "dotless i was not generated from the fixture")) {
        return 1;
    }

    OpenType_Font_Writer writer;
    if (!expect(writer.Write(generatedPath.c_str(), &font) == kOk,
                "failed to serialize generated simple glyphs")) {
        return 1;
    }
    OpenType_Font reparsed;
    OpenType_Font_Parser generatedParser;
    if (!expect(generatedParser.Parse(generatedPath.c_str(), &reparsed) == kOk,
                "failed to reparse generated simple glyphs")) {
        return 1;
    }
    const OpenType_GlyphHeader *header = nullptr;
    reparsed.Glyph(dotless.GlyphIndex, &header);
    const OpenType_GlyphSimple *reparsedDotless =
        header && header->NumberOfContours >= 0
            ? (const OpenType_GlyphSimple*)header : nullptr;
    OpenType_LongHorMetric sourceIMetric = {};
    OpenType_LongHorMetric dotlessMetric = {};
    reparsed.GlyphHorMetric(reparsed.CharToGlyphIndex('i'), sourceIMetric);
    reparsed.GlyphHorMetric(dotless.GlyphIndex, dotlessMetric);
    if (!expect(reparsedDotless != nullptr &&
                    !reparsedDotless->Points.empty() &&
                    reparsedDotless->EndPtsOfContours.size() == 1 &&
                    reparsedDotless->Instructions.empty(),
                "serialized dotless i metadata is invalid") ||
        !expect(sourceIMetric.AdvanceWidth == dotlessMetric.AdvanceWidth,
                "dotless i did not preserve source advance width") ||
        !expect(reparsed.Maxp().MaxPoints >= reparsedDotless->Points.size() &&
                    reparsed.Maxp().MaxContours >=
                        reparsedDotless->EndPtsOfContours.size(),
                "maxp does not cover appended simple glyphs")) {
        return 1;
    }

    std::string compositePath = std::string(argv[1]) + "/component-composite-i.ttf";
    if (!expect(OpenType_TestFontFixture::Write(
                    compositePath.c_str(), noExtra, noExtra, false, true) == kOk,
                "failed to write composite-i fixture")) {
        return 1;
    }
    OpenType_Font compositeFont;
    OpenType_Font_Parser compositeParser;
    if (!expect(compositeParser.Parse(compositePath.c_str(), &compositeFont) == kOk,
                "failed to parse composite-i fixture")) {
        return 1;
    }
    PinyinComponents compositeComponents(compositeFont);
    if (!expect(compositeComponents.ResolveDotlessI(dotless) == kNotSupported,
                "composite i should be unsupported")) {
        return 1;
    }

    return 0;
}
