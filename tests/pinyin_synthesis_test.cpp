#include "ot_font_parser.h"
#include "pinyin_db.h"
#include "pinyin_font_builder.h"
#include "test_font_fixture.h"
#include "test_runner.h"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

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
        "妈\t5988\tmā\n"
        "麻\t9EBB\tmá\n"
        "马\t9A6C\tmǎ\n"
        "骂\t9A82\tmà\n"
        "女\t5973\tnǚ\n"
        "米\t7C73\tmǐ\n"
        "呒\t5452\tḿ\n"
        "嗯\t55EF\tńg,ňg,ǹg\n"
        "一\t4E00\tyī\n"
        "藏\t85CF\tcáng,zàng\n"
        "六\t516D\tliù\n";
    bool ok = std::fputs(contents, file) >= 0;
    std::fclose(file);
    return ok;
}

PINYINFONT_TEST(pinyin_synthesis)
{
    std::string directory = context.FixtureDirectory;
    std::string sourcePath = directory + "/synthesis-fixture.ttf";
    std::string outputPath = directory + "/synthesis-generated.ttf";
    std::string databasePath = directory + "/synthesis-db.txt";

    std::set<uint32_t> han;
    static const uint32_t annotatedHan[] = {
        0x5988, 0x9EBB, 0x9A6C, 0x9A82, 0x5973, 0x7C73
    };
    han.insert(
        annotatedHan,
        annotatedHan + sizeof(annotatedHan) / sizeof(annotatedHan[0]));
    han.insert(0x5452);
    han.insert(0x55EF);
    han.insert(0x4E00);
    han.insert(0x85CF);
    han.insert(0x012B);
    han.insert('@');
    han.insert('1');
    han.insert('2');
    han.insert('4');
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
    if (!expect(addOK == 10 && addFailed == 0,
                "unexpected fixture coverage totals") ||
        !expect(stats.SourceHanMissing == 1,
                "missing source Han was not classified") ||
        !expect(stats.ComponentFailed == 0 && stats.DotlessIFailed == 0 &&
                    stats.OtherFailed == 0,
                "component fallback left unexplained failures") ||
        !expect(stats.ToneFallbackGenerated > 0 &&
                    stats.DotlessIGenerated > 0,
                "fixture did not exercise tone and dotless-i fallback") ||
        !expect(stats.AlternateGlyphsGenerated == 2 &&
                    stats.SelectorLigaturesGenerated == 4 &&
                    stats.SelectorMissingInputOmissions == 1 &&
                    stats.AlternateSynthesisOmissions == 0,
                "polyphonic selector statistics are inconsistent")) {
        return 1;
    }

    OpenType_Font generated;
    OpenType_Font original;
    OpenType_Font generatedWithoutGsub;
    OpenType_Font_Parser generatedParser;
    OpenType_Font_Parser originalParserForPunctuation;
    OpenType_Font_Parser generatedWithoutGsubParser;
    if (!expect(generatedParser.Parse(
                    outputPath.c_str(), &generated) == kOk,
                "failed to parse synthesized fixture") ||
        !expect(generatedWithoutGsubParser.Parse(
                    outputPath.c_str(), &generatedWithoutGsub,
                    {"GSUB"}) == kOk &&
                    generatedWithoutGsub.LigatureSubstitutions().empty(),
                "skipTables did not omit GSUB parsing") ||
        !expect(originalParserForPunctuation.Parse(
                    sourcePath.c_str(), &original) == kOk,
                "failed to parse source fixture") ||
        !expect(generated.CharToGlyphIndex(0x5988) >= oldCount &&
                    generated.CharToGlyphIndex(0x9EBB) >= oldCount &&
                    generated.CharToGlyphIndex(0x9A6C) >= oldCount &&
                    generated.CharToGlyphIndex(0x9A82) >= oldCount &&
                    generated.CharToGlyphIndex(0x5973) >= oldCount &&
                    generated.CharToGlyphIndex(0x7C73) >= oldCount &&
                    generated.CharToGlyphIndex(0x5452) >= oldCount &&
                    generated.CharToGlyphIndex(0x55EF) >= oldCount &&
                    generated.CharToGlyphIndex(0x4E00) >= oldCount &&
                    generated.CharToGlyphIndex(0x85CF) >= oldCount,
                "tone integration did not replace expected source mappings") ||
        !expect(generated.CharToGlyphIndex(0x516D) == 0,
                "missing source Han unexpectedly gained a mapping")) {
        return 1;
    }

    // The parser intentionally handles outline/cmap data only, so inspect the
    // generated GSUB bytes directly for the selector feature contract.
    std::ifstream generatedFile(outputPath.c_str(), std::ios::binary);
    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(generatedFile)),
        std::istreambuf_iterator<char>());
    auto u2 = [&bytes](size_t p) -> uint16_t {
        return (uint16_t)((bytes[p] << 8) | bytes[p + 1]);
    };
    auto u4 = [&bytes](size_t p) -> uint32_t {
        return ((uint32_t)bytes[p] << 24) |
            ((uint32_t)bytes[p + 1] << 16) |
            ((uint32_t)bytes[p + 2] << 8) | bytes[p + 3];
    };
    size_t gsub = 0;
    uint32_t gsubLength = 0;
    if (bytes.size() >= 12) {
        uint16_t tableCount = u2(4);
        for (uint16_t i = 0; i < tableCount; i++) {
            size_t record = 12 + (size_t)i * 16;
            if (record + 16 <= bytes.size() &&
                bytes[record] == 'G' && bytes[record + 1] == 'S' &&
                bytes[record + 2] == 'U' && bytes[record + 3] == 'B') {
                gsub = u4(record + 8);
                gsubLength = u4(record + 12);
            }
        }
    }
    bool gsubValid = gsub > 0 && gsub + gsubLength <= bytes.size() &&
        gsubLength >= 10 && u4(gsub) == 0x00010000;
    bool hasDflt = false, hasHani = false, hasLiga = false;
    bool hasAt1 = false, hasAt2 = false, hasAt4 = false;
    if (gsubValid) {
        size_t scriptList = gsub + u2(gsub + 4);
        size_t featureList = gsub + u2(gsub + 6);
        size_t lookupList = gsub + u2(gsub + 8);
        hasDflt = scriptList + 14 <= bytes.size() &&
            bytes[scriptList + 2] == 'D' && bytes[scriptList + 3] == 'F' &&
            bytes[scriptList + 4] == 'L' && bytes[scriptList + 5] == 'T';
        hasHani = scriptList + 14 <= bytes.size() &&
            bytes[scriptList + 8] == 'h' && bytes[scriptList + 9] == 'a' &&
            bytes[scriptList + 10] == 'n' && bytes[scriptList + 11] == 'i';
        hasLiga = featureList + 8 <= bytes.size() &&
            bytes[featureList + 2] == 'l' && bytes[featureList + 3] == 'i' &&
            bytes[featureList + 4] == 'g' && bytes[featureList + 5] == 'a';
        size_t lookup = lookupList + u2(lookupList + 2);
        size_t subtable = lookup + u2(lookup + 6);
        size_t coverage = subtable + u2(subtable + 2);
        uint16_t defaultGlyph = generated.CharToGlyphIndex(0x85CF);
        uint16_t atGlyph = generated.CharToGlyphIndex('@');
        uint16_t oneGlyph = generated.CharToGlyphIndex('1');
        uint16_t twoGlyph = generated.CharToGlyphIndex('2');
        uint16_t fourGlyph = generated.CharToGlyphIndex('4');
        uint16_t coverageCount = u2(coverage + 2);
        for (uint16_t c = 0; c < coverageCount; c++) {
            if (u2(coverage + 4 + c * 2) != defaultGlyph) continue;
            size_t set = subtable + u2(subtable + 6 + c * 2);
            uint16_t ligatureCount = u2(set);
            for (uint16_t l = 0; l < ligatureCount; l++) {
                size_t ligature = set + u2(set + 2 + l * 2);
                if (u2(ligature + 2) != 3 ||
                    u2(ligature + 4) != atGlyph) continue;
                uint16_t digit = u2(ligature + 6);
                uint16_t replacement = u2(ligature);
                hasAt1 = hasAt1 ||
                    (digit == oneGlyph && replacement == defaultGlyph);
                hasAt2 = hasAt2 ||
                    (digit == twoGlyph && replacement != defaultGlyph &&
                     replacement < generated.GlyphCount());
                hasAt4 = hasAt4 || digit == fourGlyph;
            }
        }
    }
    if (!expect(gsubValid && hasDflt && hasHani && hasLiga,
                "generated GSUB structure is invalid") ||
        !expect(hasAt1 && hasAt2 && !hasAt4,
                "generated selector ligatures do not match database readings")) {
        return 1;
    }
    bool singleReadingHasRule = false;
    const std::vector<OpenType_LigatureSubstitution> &parsedRules =
        generated.LigatureSubstitutions();
    for (size_t i = 0; i < parsedRules.size(); i++) {
        singleReadingHasRule = singleReadingHasRule ||
            parsedRules[i].Components[0] ==
                generated.CharToGlyphIndex(0x4E00);
    }
    if (!expect(!singleReadingHasRule,
                "single-reading character unexpectedly gained a selector") ||
        !expect(generated.CharToGlyphIndex('@') ==
                    original.CharToGlyphIndex('@') &&
                generated.CharToGlyphIndex('1') ==
                    original.CharToGlyphIndex('1') &&
                generated.CharToGlyphIndex('2') ==
                    original.CharToGlyphIndex('2') &&
                generated.CharToGlyphIndex('4') ==
                    original.CharToGlyphIndex('4'),
                "literal selector cmap fallback was not preserved")) {
        return 1;
    }

    // Exercise rule ordering and duplicate rejection in the in-memory model.
    uint16_t atGlyph = generated.CharToGlyphIndex('@');
    uint16_t oneGlyph = generated.CharToGlyphIndex('1');
    uint16_t twoGlyph = generated.CharToGlyphIndex('2');
    uint16_t defaultGlyph = generated.CharToGlyphIndex(0x85CF);
    std::vector<uint16_t> shortRule = { atGlyph, oneGlyph };
    std::vector<uint16_t> longRule = { atGlyph, oneGlyph, twoGlyph };
    if (!expect(generated.AddLigatureSubstitution(
                    shortRule, defaultGlyph) == kOk &&
                generated.AddLigatureSubstitution(
                    longRule, defaultGlyph) == kOk &&
                generated.AddLigatureSubstitution(
                    shortRule, defaultGlyph) == kInvalidArgs,
                "ligature model did not reject duplicate input") ||
        !expect(generated.LigatureSubstitutions()[0].Components.size() == 3,
                "ligature rules were not ordered longest-first")) {
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
    generated.Glyph(generated.CharToGlyphIndex(0x4E00), &precomposedHeader);
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
    generated.Glyph(generated.CharToGlyphIndex(0x5973), &stackedHeader);
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
    oneHan.insert(0x5426);
    oneHan.insert('@');
    oneHan.insert('1');
    oneHan.insert('2');
    if (!expect(OpenType_TestFontFixture::Write(
                    compositeSource.c_str(), oneHan, noMarks, false, true) == kOk,
                "failed to create composite-i synthesis fixture")) {
        return 1;
    }
    FILE *failureDB = std::fopen(failureDBPath.c_str(), "wb");
    if (!failureDB) return 1;
    std::fputs("否\t5426\tfǒu,pǐ\n", failureDB);
    std::fclose(failureDB);
    PinyinDB failureDatabase;
    failureDatabase.Load(failureDBPath.c_str());
    PinyinFontBuilder failureBuilder;
    if (!expect(failureBuilder.Build(
                    compositeSource.c_str(), compositeOutput.c_str(),
                    failureDatabase) == kOk,
                "conservative fallback build failed globally") ||
        !expect(
            failureBuilder.GetSynthesisStats().AlternateSynthesisOmissions == 1 &&
            failureBuilder.GetSynthesisStats().SelectorLigaturesGenerated == 1,
            "unsupported alternate reading was not isolated and classified")) {
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
        !expect(compositeOriginal.CharToGlyphIndex(0x5426) !=
                    compositeGenerated.CharToGlyphIndex(0x5426) &&
                    compositeGenerated.LigatureSubstitutions().size() == 1,
                "alternate failure damaged the valid default mapping or rule")) {
        return 1;
    }

    return 0;
}
