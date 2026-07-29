#include "ot_font_parser.h"
#include "pinyin_db.h"
#include "pinyin_font_builder.h"
#include "test_font_fixture.h"
#include <cstdio>
#include <map>
#include <set>
#include <string>

static bool expect(bool condition, const char *message)
{
    if (!condition) std::fprintf(stderr, "%s\n", message);
    return condition;
}

static bool writeDB(const std::string &path)
{
    FILE *file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const char *contents =
        "一\t4E00\tmā\n"
        "丁\t4E01\tmá\n"
        "丂\t4E02\tmǎ\n"
        "七\t4E03\tmà\n"
        "丄\t4E04\tnǚ\n"
        "丅\t4E05\tmǐ\n"
        "呒\t5452\tḿ\n"
        "嗯\t55EF\tńg\n"
        "万\t4E07\tyī\n"
        "丆\t4E06\tmā\n";
    bool ok = std::fputs(contents, file) >= 0;
    std::fclose(file);
    return ok;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s output-directory\n", argv[0]);
        return 1;
    }
    std::string directory = argv[1];
    std::string sourcePath = directory + "/synthesis-fixture.ttf";
    std::string outputPath = directory + "/synthesis-generated.ttf";
    std::string databasePath = directory + "/synthesis-db.txt";

    std::set<uint32_t> han;
    for (uint32_t c = 0x4E00; c <= 0x4E05; c++) han.insert(c);
    han.insert(0x5452);
    han.insert(0x55EF);
    han.insert(0x4E07);
    han.insert(0x012B);
    static const uint32_t punctuation[] = {
        0x0021, 0x0022, 0x0027, 0x0028, 0x0029, 0x002C, 0x002D,
        0x002E, 0x003A, 0x003B, 0x003F, 0x005B, 0x005D, 0x007B, 0x007D,
        0x00B7, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026,
        0x3001, 0x3002, 0x3008, 0x3009, 0x300A, 0x300B, 0x300C, 0x300D,
        0x300E, 0x300F, 0x3010, 0x3011, 0x3014, 0x30FB,
        0xFF01, 0xFF02, 0xFF07, 0xFF08, 0xFF09, 0xFF0C, 0xFF0D,
        0xFF0E, 0xFF1A, 0xFF1B, 0xFF1F, 0xFF3B, 0xFF3D, 0xFF5B, 0xFF5D,
    };
    han.insert(
        punctuation, punctuation + sizeof(punctuation) / sizeof(punctuation[0]));
    static const uint32_t excludedPunctuation[] = { '/', '\\', '_' };
    han.insert(
        excludedPunctuation,
        excludedPunctuation +
            sizeof(excludedPunctuation) / sizeof(excludedPunctuation[0]));
    std::set<uint32_t> noMarks;
    std::set<uint32_t> compositeCharacters;
    compositeCharacters.insert(0x3002);
    std::set<uint32_t> emptyCharacters;
    emptyCharacters.insert(0x3014);
    std::map<uint32_t, uint32_t> sharedMappings;
    sharedMappings[0x002E] = 0x002C;
    if (!expect(OpenType_TestFontFixture::Write(
                    sourcePath.c_str(), han, noMarks, true, false,
                    compositeCharacters, emptyCharacters, sharedMappings) == kOk,
                "failed to create synthesis fixture") ||
        !expect(writeDB(databasePath), "failed to create synthesis database")) {
        return 1;
    }

    PinyinDB database;
    if (!expect(database.Load(databasePath.c_str()) == kOk,
                "failed to load synthesis database")) {
        return 1;
    }
    PinyinFontBuilder builder;
    if (!expect(builder.Build(
                    sourcePath.c_str(), outputPath.c_str(), database) == kOk,
                "fixture synthesis failed")) {
        return 1;
    }
    uint16_t oldCount = 0, addOK = 0, addFailed = 0;
    uint32_t parseTime = 0, synthesisTime = 0, writeTime = 0;
    builder.GetStats(
        oldCount, addOK, addFailed, parseTime, synthesisTime, writeTime);
    const PinyinSynthesisStats &stats = builder.GetSynthesisStats();
    if (!expect(addOK == 9 && addFailed == 0,
                "unexpected fixture coverage totals") ||
        !expect(stats.SourceHanMissing == 1,
                "missing source Han was not classified") ||
        !expect(stats.ComponentFailed == 0 && stats.DotlessIFailed == 0 &&
                    stats.OtherFailed == 0,
                "component fallback left unexplained failures") ||
        !expect(stats.ToneFallbackGenerated > 0 &&
                    stats.DotlessIGenerated > 0,
                "fixture did not exercise tone and dotless-i fallback")) {
        return 1;
    }

    OpenType_Font generated;
    OpenType_Font original;
    OpenType_Font_Parser generatedParser;
    OpenType_Font_Parser originalParserForPunctuation;
    if (!expect(generatedParser.Parse(outputPath.c_str(), &generated) == kOk,
                "failed to parse synthesized fixture") ||
        !expect(originalParserForPunctuation.Parse(
                    sourcePath.c_str(), &original) == kOk,
                "failed to parse source fixture") ||
        !expect(generated.CharToGlyphIndex(0x4E00) >= oldCount &&
                    generated.CharToGlyphIndex(0x4E05) >= oldCount &&
                    generated.CharToGlyphIndex(0x5452) >= oldCount &&
                    generated.CharToGlyphIndex(0x55EF) >= oldCount &&
                    generated.CharToGlyphIndex(0x4E07) >= oldCount,
                "tone integration did not replace expected source mappings") ||
        !expect(generated.CharToGlyphIndex(0x4E06) == 0,
                "missing source Han unexpectedly gained a mapping")) {
        return 1;
    }

    for (size_t i = 0;
         i < sizeof(punctuation) / sizeof(punctuation[0]); i++) {
        uint32_t charcode = punctuation[i];
        uint16_t sourceIndex = original.CharToGlyphIndex(charcode);
        uint16_t scaledIndex = generated.CharToGlyphIndex(charcode);
        if (charcode == 0x3014) {
            if (!expect(scaledIndex == sourceIndex,
                        "empty punctuation did not preserve its source mapping")) {
                return 1;
            }
            continue;
        }

        const OpenType_GlyphHeader *scaledHeader = nullptr;
        const OpenType_GlyphHeader *sourceHeader = nullptr;
        generated.Glyph(scaledIndex, &scaledHeader);
        original.Glyph(sourceIndex, &sourceHeader);
        const OpenType_GlyphComposite *scaled =
            scaledHeader && scaledHeader->NumberOfContours < 0
                ? (const OpenType_GlyphComposite*)scaledHeader : nullptr;
        OpenType_LongHorMetric sourceMetric = { 0 };
        OpenType_LongHorMetric scaledMetric = { 0 };
        original.GlyphHorMetric(sourceIndex, sourceMetric);
        generated.GlyphHorMetric(scaledIndex, scaledMetric);
        if (!expect(scaledIndex >= oldCount && scaled &&
                        scaled->SubGlyphs.size() == 1 &&
                        scaled->SubGlyphs[0].GlyphIndex == sourceIndex,
                    "whitelisted punctuation was not wrapped") ||
            !expect(scaled->SubGlyphs[0].Transform[0] ==
                            (int16_t)(0.65 * 16384.0) &&
                        scaled->SubGlyphs[0].Transform[3] ==
                            (int16_t)(0.65 * 16384.0),
                    "punctuation did not use the Han body scale") ||
            !expect(scaled->SubGlyphs[0].Arg1 ==
                            (int16_t)(sourceMetric.AdvanceWidth * 0.35 / 2) &&
                        scaled->SubGlyphs[0].Arg2 == 0,
                    "punctuation did not use the Han body offsets") ||
            !expect(sourceHeader &&
                        scaledHeader->XMin ==
                            (int16_t)(sourceHeader->XMin * 0.65 +
                                scaled->SubGlyphs[0].Arg1) &&
                        scaledHeader->YMin ==
                            (int16_t)(sourceHeader->YMin * 0.65) &&
                        scaledHeader->XMax ==
                            (int16_t)(sourceHeader->XMax * 0.65 +
                                scaled->SubGlyphs[0].Arg1) &&
                        scaledHeader->YMax ==
                            (int16_t)(sourceHeader->YMax * 0.65),
                    "punctuation transformed bounds are inconsistent") ||
            !expect(scaledMetric.AdvanceWidth == sourceMetric.AdvanceWidth &&
                        scaledMetric.LSB == scaledHeader->XMin,
                    "punctuation horizontal metrics are inconsistent")) {
            return 1;
        }
    }
    if (!expect(generated.CharToGlyphIndex(0x002C) ==
                    generated.CharToGlyphIndex(0x002E),
                "shared source punctuation did not reuse its wrapper") ||
        !expect(generated.CharToGlyphIndex(0x3015) == 0,
                "absent punctuation unexpectedly gained a mapping")) {
        return 1;
    }
    for (size_t i = 0;
         i < sizeof(excludedPunctuation) / sizeof(excludedPunctuation[0]); i++) {
        uint32_t charcode = excludedPunctuation[i];
        if (!expect(generated.CharToGlyphIndex(charcode) ==
                        original.CharToGlyphIndex(charcode),
                    "excluded technical punctuation was remapped")) {
            return 1;
        }
    }
    if (!expect(generated.Maxp().MaxComponentDepth >= 2,
                "nested punctuation composite depth was not updated")) {
        return 1;
    }

    const OpenType_GlyphHeader *precomposedHeader = nullptr;
    generated.Glyph(generated.CharToGlyphIndex(0x4E07), &precomposedHeader);
    const OpenType_GlyphComposite *precomposed =
        precomposedHeader && precomposedHeader->NumberOfContours < 0
            ? (const OpenType_GlyphComposite*)precomposedHeader : nullptr;
    bool usedSourcePrecomposed = false;
    if (precomposed) {
        uint16_t sourcePrecomposed = generated.CharToGlyphIndex(0x012B);
        for (size_t i = 0; i < precomposed->SubGlyphs.size(); i++) {
            usedSourcePrecomposed =
                usedSourcePrecomposed ||
                precomposed->SubGlyphs[i].GlyphIndex == sourcePrecomposed;
        }
    }
    const OpenType_GlyphHeader *stackedHeader = nullptr;
    generated.Glyph(generated.CharToGlyphIndex(0x4E04), &stackedHeader);
    const OpenType_GlyphComposite *stacked =
        stackedHeader && stackedHeader->NumberOfContours < 0
            ? (const OpenType_GlyphComposite*)stackedHeader : nullptr;
    if (!expect(usedSourcePrecomposed,
                "source precomposed accented i was not preferred") ||
        !expect(stacked && stacked->SubGlyphs.size() >= 4 &&
                    stacked->SubGlyphs[2].Arg2 > stacked->SubGlyphs[1].Arg2,
                "diaeresis and tone components overlap vertically")) {
        return 1;
    }

    std::string compositeSource = directory + "/synthesis-composite-i.ttf";
    std::string compositeOutput = directory + "/synthesis-composite-i-generated.ttf";
    std::string failureDBPath = directory + "/synthesis-failure-db.txt";
    std::set<uint32_t> oneHan;
    oneHan.insert(0x4E00);
    if (!expect(OpenType_TestFontFixture::Write(
                    compositeSource.c_str(), oneHan, noMarks, false, true) == kOk,
                "failed to create composite-i synthesis fixture")) {
        return 1;
    }
    FILE *failureDB = std::fopen(failureDBPath.c_str(), "wb");
    if (!failureDB) return 1;
    std::fputs("一\t4E00\tmǐ\n", failureDB);
    std::fclose(failureDB);
    PinyinDB failureDatabase;
    failureDatabase.Load(failureDBPath.c_str());
    PinyinFontBuilder failureBuilder;
    if (!expect(failureBuilder.Build(
                    compositeSource.c_str(), compositeOutput.c_str(),
                    failureDatabase) == kOk,
                "conservative fallback build failed globally") ||
        !expect(failureBuilder.GetSynthesisStats().DotlessIFailed == 1,
                "unsupported composite i was not classified")) {
        return 1;
    }
    OpenType_Font compositeOriginal;
    OpenType_Font compositeGenerated;
    OpenType_Font_Parser originalParser;
    OpenType_Font_Parser outputParser;
    if (!expect(originalParser.Parse(
                    compositeSource.c_str(), &compositeOriginal) == kOk &&
                outputParser.Parse(
                    compositeOutput.c_str(), &compositeGenerated) == kOk,
                "failed to parse conservative fallback fonts") ||
        !expect(compositeOriginal.CharToGlyphIndex(0x4E00) ==
                    compositeGenerated.CharToGlyphIndex(0x4E00),
                "failed synthesis did not preserve the source cmap mapping")) {
        return 1;
    }

    return 0;
}
