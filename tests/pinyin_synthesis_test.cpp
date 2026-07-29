#include "ot_font_parser.h"
#include "pinyin_db.h"
#include "pinyin_font_builder.h"
#include "test_font_fixture.h"
#include <cstdio>
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
    std::set<uint32_t> noMarks;
    if (!expect(OpenType_TestFontFixture::Write(
                    sourcePath.c_str(), han, noMarks, true, false) == kOk,
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
    OpenType_Font_Parser generatedParser;
    if (!expect(generatedParser.Parse(outputPath.c_str(), &generated) == kOk,
                "failed to parse synthesized fixture") ||
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
