#include "ot_font_parser.h"
#include "scope_guard.h"
#include "utility.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <set>
#include <string>
#include <vector>

//------------------------------------------------------------------------------

struct Options {
    std::string source;
    std::string input;
    std::string output;
    std::string table;
};

static void printUsage(const char *program)
{
    std::fprintf(stdout, "usage:\n");
    std::fprintf(stdout, "  %s info --input <font.ttf>\n", program);
    std::fprintf(stdout, "  %s integrity --source <original.ttf> --input <generated.ttf>\n", program);
    std::fprintf(stdout, "  %s table-dump --input <font.ttf> --table <tag> [--output <file.dat>]\n", program);
    std::fprintf(stdout, "  %s table-purge --input <font.ttf> --table <tag> [--output <out.ttf>]\n", program);
}

static bool parseOptions(int argc, char *argv[], int start, Options &options)
{
    options = Options();
    for (int i = start; i < argc; i++) {
        const char *arg = argv[i];
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            return false;
        }
        if (i + 1 >= argc) {
            std::fprintf(stderr, "missing value for %s\n", arg);
            return false;
        }
        if (std::strcmp(arg, "--input") == 0) {
            options.input = argv[++i];
        } else if (std::strcmp(arg, "--source") == 0) {
            options.source = argv[++i];
        } else if (std::strcmp(arg, "--output") == 0) {
            options.output = argv[++i];
        } else if (std::strcmp(arg, "--table") == 0) {
            options.table = argv[++i];
        } else {
            std::fprintf(stderr, "unknown option: %s\n", arg);
            return false;
        }
    }
    return true;
}

static bool requireInput(const Options &options)
{
    if (options.input.empty()) {
        std::fprintf(stderr, "missing required option: --input\n");
        return false;
    }
    return true;
}

static bool requireSource(const Options &options)
{
    if (options.source.empty()) {
        std::fprintf(stderr, "missing required option: --source\n");
        return false;
    }
    return true;
}

static bool requireTable(const Options &options)
{
    if (options.table.empty()) {
        std::fprintf(stderr, "missing required option: --table\n");
        return false;
    }
    return true;
}

static std::string defaultTableDumpOutput(const std::string &fileName, const std::string &tableName)
{
    std::string output = fileName;
    output += ".";
    if (tableName == "OS/2") {
        output += "OS2";
    } else {
        output += tableName;
    }
    output += ".dat";
    return output;
}

//------------------------------------------------------------------------------

static void dumpBasicInfo(const OpenType_Font &font)
{
    const OpenType_Head &head = font.Head();
    std::fprintf(stdout, "Head:\n");
    std::fprintf(stdout, "  Version = 0x%08x\n", head.Version);
    std::fprintf(stdout, "  Flags = 0x%04x\n", (unsigned int)head.Flags);
    std::fprintf(stdout, "  UnitsPerEm = %d\n", (int)head.UnitsPerEm);
    std::fprintf(stdout, "  MacStyle = 0x%04x\n", (unsigned int)head.MacStyle);
    std::fprintf(stdout, "\n");
    const OpenType_Maxp &maxp = font.Maxp();
    std::fprintf(stdout, "Maxp:\n");
    std::fprintf(stdout, "  Version = 0x%08x\n", maxp.Version);
    std::fprintf(stdout, "  NumGlyphs = %d\n", (int)maxp.NumGlyphs);
    std::fprintf(stdout, "  MaxPoints = %d\n", (int)maxp.MaxPoints);
    std::fprintf(stdout, "  MaxContours = %d\n", (int)maxp.MaxContours);
    std::fprintf(stdout, "  MaxCompositePoints = %d\n", (int)maxp.MaxCompositePoints);
    std::fprintf(stdout, "  MaxCompositeContours = %d\n", (int)maxp.MaxCompositeContours);
    std::fprintf(stdout, "  MaxComponentElements = %d\n", (int)maxp.MaxComponentElements);
    std::fprintf(stdout, "  MaxComponentDepth = %d\n", (int)maxp.MaxComponentDepth);
    std::fprintf(stdout, "\n");
    const OpenType_OS2 &os2 = font.OS2();
    std::fprintf(stdout, "OS/2:\n");
    std::fprintf(stdout, "  version = %d\n", (int)os2.version);
    std::fprintf(stdout, "  xAvgCharWidth = %d\n", (int)os2.xAvgCharWidth);
    std::fprintf(stdout, "  usWeightClass = %u\n", (unsigned int)os2.usWeightClass);
    std::fprintf(stdout, "  usWidthClass = %u\n", (unsigned int)os2.usWidthClass);
    std::fprintf(stdout, "  fsType = 0x%04x\n", (unsigned int)os2.fsType);
    std::fprintf(stdout, "\n");
}

static void printNameRecords(const char *name, const std::vector<OpenType_NameRecord> &records)
{
    if (records.size() > 0) {
        for (size_t i = 0; i < records.size(); i++) {
            std::fprintf(stdout, "  %s = %ls (%d, %d, %d)\n",
                name, records[i].String.c_str(), records[i].PlatformID, records[i].EncodingID, records[i].LanguageID);
        }
    } else {
        std::fprintf(stdout, "  %s = <NotFound>\n", name);
    }
}

static void dumpName(const OpenType_Font &font)
{
    std::vector<OpenType_NameRecord> nameRecords;

    std::fprintf(stdout, "Name:\n");

    font.Name(1, nameRecords);
    printNameRecords("FamilyName", nameRecords);
    font.Name(2, nameRecords);
    printNameRecords("SubfamilyName", nameRecords);
    font.Name(3, nameRecords);
    printNameRecords("UniqueFontIdentifier", nameRecords);
    font.Name(4, nameRecords);
    printNameRecords("FullName", nameRecords);
    font.Name(5, nameRecords);
    printNameRecords("VersionString", nameRecords);
    font.Name(6, nameRecords);
    printNameRecords("PostScriptName", nameRecords);
    font.Name(7, nameRecords);
    printNameRecords("Trademark", nameRecords);
    font.Name(8, nameRecords);
    printNameRecords("ManufacturerName", nameRecords);
    font.Name(9, nameRecords);
    printNameRecords("Designer", nameRecords);
    font.Name(10, nameRecords);
    printNameRecords("Description", nameRecords);

    std::fprintf(stdout, "\n");
}

static void dumpPost(const OpenType_Font &font, const std::set<int> &indices)
{
    std::fprintf(stdout, "Post:\n");
    std::fprintf(stdout, "  Version = 0x%08x\n", font.Post().Version);
    std::fprintf(stdout, "  IsFixedPitch = %u\n", (unsigned int)font.Post().IsFixedPitch);

    std::string name;
    for (auto iter = indices.begin(); iter != indices.end(); ++iter) {
        int index = *iter;
        font.GlyphName(index, name);
        std::fprintf(stdout, "  GlyphName_%d = %s\n", index, name.c_str());
    }

    std::fprintf(stdout, "\n");
}

static void dumpGlyph(const OpenType_Font &font, const std::set<int> &indices)
{
    std::fprintf(stdout, "Glyph:\n");
    std::fprintf(stdout, "  Count = %d\n", font.GlyphCount());
    for (auto iter = indices.begin(); iter != indices.end(); ++iter) {
        int index = *iter;
        const OpenType_GlyphHeader *pHeader = nullptr;
        font.Glyph(index, &pHeader);
        if (pHeader == nullptr) {
            std::fprintf(stdout, "  Glyph_%d = <NoOutline>\n", index);
        } else if (pHeader->NumberOfContours >= 0) {
            OpenType_GlyphSimple *pSimple = (OpenType_GlyphSimple*)pHeader;
            std::fprintf(stdout, "  Glyph_%d = Simple{ Contours=%d Points=%d }\n",
                index, (int)pSimple->NumberOfContours, (int)pSimple->Points.size());
        } else {
            OpenType_GlyphComposite *pComposite = (OpenType_GlyphComposite*)pHeader;
            std::fprintf(stdout, "  Glyph_%d = Composite{", index);
            for (size_t j = 0; j < pComposite->SubGlyphs.size(); j++) {
                std::fprintf(stdout, " %d", (int)pComposite->SubGlyphs[j].GlyphIndex);
            }
            std::fprintf(stdout, " }\n");
        }
    }
    std::fprintf(stdout, "\n");
}

static void dumpHmtx(const OpenType_Font &font, const std::set<int> &indices)
{
    std::fprintf(stdout, "Hhea+Hmtx:\n");
    std::fprintf(stdout, "  Ascender = %d\n", (int)font.Hhea().Ascender);
    std::fprintf(stdout, "  Descender = %d\n", (int)font.Hhea().Descender);
    std::fprintf(stdout, "  LineGap = %d\n", (int)font.Hhea().LineGap);
    std::fprintf(stdout, "  AdvanceWidthMax = %d\n", (int)font.Hhea().AdvanceWidthMax);
    std::fprintf(stdout, "  MinLeftSideBearing = %d\n", (int)font.Hhea().MinLeftSideBearing);
    std::fprintf(stdout, "  MinRightSideBearing = %d\n", (int)font.Hhea().MinRightSideBearing);
    std::fprintf(stdout, "  XMaxExtent = %d\n", (int)font.Hhea().XMaxExtent);
    std::fprintf(stdout, "  NumberOfHMetrics = %d\n", (int)font.Hhea().NumberOfHMetrics);
    for (auto iter = indices.begin(); iter != indices.end(); ++iter) {
        int index = *iter;
        OpenType_LongHorMetric mtx;
        font.GlyphHorMetric(index, mtx);
        std::fprintf(stdout, "  Glyph_%d = { Advance=%d, LSB=%d }\n",
            index, (int)mtx.AdvanceWidth, (int)mtx.LSB);
    }
    std::fprintf(stdout, "\n");
}

static void dumpCmap(const OpenType_Font &font)
{
    std::fprintf(stdout, "Cmap:\n");
    std::fprintf(stdout, "  U+0061 -> %d\n", (int)font.CharToGlyphIndex(0x0061));
    std::fprintf(stdout, "  U+0030 -> %d\n", (int)font.CharToGlyphIndex(0x0030));
    std::fprintf(stdout, "  U+4E2D -> %d\n", (int)font.CharToGlyphIndex(0x4E2D));
    std::fprintf(stdout, "  U+0304 -> %d\n", (int)font.CharToGlyphIndex(0x0304));
    std::fprintf(stdout, "\n");
}

static int printFontInfo(const char *filename)
{
    OpenType_Font font;
    OpenType_Font_Parser parser;
    Status status = parser.Parse(filename, &font);
    if (status != kOk) {
        std::fprintf(stderr, "Parse failed, error=%d\n", status);
        return 1;
    }
    std::fprintf(stdout, "Parse succeeded\n");
    std::fprintf(stdout, "\n");

    int maxItems = font.GlyphCount();
    if (maxItems > 16) {
        maxItems = 16;
    }
    std::set<int> indices;
    while ((int)indices.size() < maxItems) {
        indices.insert(std::rand() % font.GlyphCount());
    }

    dumpBasicInfo(font);
    dumpName(font);
    dumpPost(font, indices);
    dumpGlyph(font, indices);
    dumpHmtx(font, indices);
    dumpCmap(font);

    std::fprintf(stdout, "\n");
    return 0;
}

//------------------------------------------------------------------------------

static int parseFont(const char *filename, OpenType_Font &font)
{
    OpenType_Font_Parser parser;
    Status status = parser.Parse(filename, &font);
    if (status != kOk) {
        std::fprintf(stderr, "Parse failed for %s, error=%d\n", filename, status);
        return 1;
    }
    return 0;
}

static bool isGeneratedGlyph(const OpenType_Font &source, uint16_t glyphID)
{
    return glyphID >= (uint16_t)source.GlyphCount();
}

enum class GeneratedReplacementKind {
    Invalid,
    Pinyin,
    Punctuation,
};

static GeneratedReplacementKind generatedReplacementKind(
    const OpenType_Font &source,
    const OpenType_Font &generated,
    uint16_t sourceGlyphID,
    uint16_t generatedGlyphID)
{
    if (!isGeneratedGlyph(source, generatedGlyphID)) {
        return GeneratedReplacementKind::Invalid;
    }
    const OpenType_GlyphHeader *header = nullptr;
    if (generated.Glyph(generatedGlyphID, &header) != kOk ||
        header == nullptr || header->NumberOfContours >= 0) {
        return GeneratedReplacementKind::Invalid;
    }
    const OpenType_GlyphComposite *composite =
        (const OpenType_GlyphComposite*)header;
    bool referencesSource = false;
    for (size_t i = 0; i < composite->SubGlyphs.size(); i++) {
        referencesSource =
            referencesSource ||
            composite->SubGlyphs[i].GlyphIndex == sourceGlyphID;
    }
    if (!referencesSource) {
        return GeneratedReplacementKind::Invalid;
    }

    std::string name;
    generated.GlyphName(generatedGlyphID, name);
    if (name.size() >= 5 &&
        name.compare(name.size() - 5, 5, "_py00") == 0) {
        return GeneratedReplacementKind::Pinyin;
    }
    if (name.size() >= 6 &&
        name.compare(name.size() - 6, 6, "_punct") == 0 &&
        composite->SubGlyphs.size() == 1) {
        return GeneratedReplacementKind::Punctuation;
    }
    return GeneratedReplacementKind::Invalid;
}

static void checkCmapIntegrity(const OpenType_Font &source, const OpenType_Font &generated)
{
    int total = 0;
    int preserved = 0;
    int pinyinReplacements = 0;
    int punctuationReplacements = 0;
    int dropped = 0;
    int unexpectedChanged = 0;

    const std::vector<CmapSequentialMapGroup> &groups = source.CmapGroups();
    for (size_t i = 0; i < groups.size(); i++) {
        const CmapSequentialMapGroup &group = groups[i];
        for (uint32_t charcode = group.startCharCode; charcode <= group.endCharCode; charcode++) {
            uint16_t sourceGlyphID = source.CharToGlyphIndex(charcode);
            uint16_t generatedGlyphID = generated.CharToGlyphIndex(charcode);
            if (sourceGlyphID != 0) {
                total++;
                if (generatedGlyphID == 0) {
                    dropped++;
                } else if (generatedGlyphID == sourceGlyphID) {
                    preserved++;
                } else {
                    GeneratedReplacementKind kind = generatedReplacementKind(
                        source, generated, sourceGlyphID, generatedGlyphID);
                    if (kind == GeneratedReplacementKind::Pinyin) {
                        pinyinReplacements++;
                    } else if (kind == GeneratedReplacementKind::Punctuation) {
                        punctuationReplacements++;
                    } else {
                        unexpectedChanged++;
                    }
                }
            }
            if (charcode == 0xFFFFFFFFu) {
                break;
            }
        }
    }

    std::fprintf(stdout, "Cmap integrity:\n");
    std::fprintf(stdout, "  SourceMappings = %d\n", total);
    std::fprintf(stdout, "  Preserved = %d\n", preserved);
    std::fprintf(stdout, "  PinyinReplacements = %d\n", pinyinReplacements);
    std::fprintf(stdout, "  ScaledPunctuationReplacements = %d\n",
        punctuationReplacements);
    std::fprintf(stdout, "  GeneratedReplacements = %d\n",
        pinyinReplacements + punctuationReplacements);
    std::fprintf(stdout, "  Dropped = %d\n", dropped);
    std::fprintf(stdout, "  UnexpectedChanged = %d\n", unexpectedChanged);
    std::fprintf(stdout, "\n");
}

static void checkLigatureIntegrity(const OpenType_Font &generated)
{
    const std::vector<OpenType_LigatureSubstitution> &rules =
        generated.LigatureSubstitutions();
    uint16_t atGlyph = generated.CharToGlyphIndex('@');
    int selectorRules = 0;
    int otherLigatures = 0;
    int invalidReferences = 0;
    for (size_t i = 0; i < rules.size(); i++) {
        const OpenType_LigatureSubstitution &rule = rules[i];
        bool valid = rule.LigatureGlyph > 0 &&
            rule.LigatureGlyph < generated.GlyphCount();
        for (size_t c = 0; c < rule.Components.size(); c++) {
            valid = valid && rule.Components[c] > 0 &&
                rule.Components[c] < generated.GlyphCount();
        }
        if (!valid) {
            invalidReferences++;
            continue;
        }
        bool selector = rule.Components.size() == 3 &&
            atGlyph != 0 && rule.Components[1] == atGlyph;
        if (selector) selectorRules++;
        else otherLigatures++;
    }
    std::fprintf(stdout, "GSUB ligature integrity:\n");
    std::fprintf(stdout, "  ParsedLigatures = %u\n",
        (unsigned int)rules.size());
    std::fprintf(stdout, "  SelectorRules = %d\n", selectorRules);
    std::fprintf(stdout, "  OtherLigatures = %d\n", otherLigatures);
    std::fprintf(stdout, "  InvalidReferences = %d\n", invalidReferences);
    std::fprintf(stdout, "  LigatureReferencesOK = %s\n",
        invalidReferences == 0 ? "yes" : "no");
    std::fprintf(stdout, "\n");
}

static int compositeDepth(const OpenType_Font &font, uint16_t glyphID, std::vector<uint8_t> &visiting)
{
    if (glyphID >= (uint16_t)font.GlyphCount()) {
        return 0;
    }
    if (visiting[glyphID]) {
        return 0;
    }
    const OpenType_GlyphHeader *header = nullptr;
    font.Glyph(glyphID, &header);
    if (header == nullptr || header->NumberOfContours >= 0) {
        return 0;
    }

    visiting[glyphID] = 1;
    const OpenType_GlyphComposite *composite = (const OpenType_GlyphComposite*)header;
    int depth = 1;
    for (size_t i = 0; i < composite->SubGlyphs.size(); i++) {
        int componentDepth = 1 + compositeDepth(font, composite->SubGlyphs[i].GlyphIndex, visiting);
        if (componentDepth > depth) {
            depth = componentDepth;
        }
    }
    visiting[glyphID] = 0;
    return depth;
}

static void checkMetricIntegrity(const OpenType_Font &source, const OpenType_Font &generated)
{
    int checkedGlyphs = 0;
    int xMinBeforeHead = 0;
    int yMinBeforeHead = 0;
    int xMaxAfterHead = 0;
    int yMaxAfterHead = 0;
    int xMaxAfterAdvance = 0;
    int yMaxAfterHheaAscender = 0;
    int yMinBeforeHheaDescender = 0;
    int yMaxAfterWinAscent = 0;
    int yMinBeforeWinDescent = 0;
    int maxTopComponents = 0;
    int maxDepth = 0;
    int composites = 0;
    int internalSimpleGlyphs = 0;
    int internalMappedGlyphs = 0;
    int internalBoundsMismatch = 0;
    int internalInstructions = 0;
    int actualMaxPoints = 0;
    int actualMaxContours = 0;
    std::set<uint16_t> mappedGlyphs;
    const std::vector<CmapSequentialMapGroup> &cmapGroups = generated.CmapGroups();
    for (size_t i = 0; i < cmapGroups.size(); i++) {
        const CmapSequentialMapGroup &group = cmapGroups[i];
        for (uint32_t charcode = group.startCharCode; charcode <= group.endCharCode; charcode++) {
            uint16_t glyphID = generated.CharToGlyphIndex(charcode);
            if (glyphID != 0) mappedGlyphs.insert(glyphID);
            if (charcode == 0xFFFFFFFFu) break;
        }
    }

    int firstGeneratedGlyph = source.GlyphCount();
    std::vector<uint8_t> visiting((size_t)generated.GlyphCount(), 0);
    for (int i = firstGeneratedGlyph; i < generated.GlyphCount(); i++) {
        const OpenType_GlyphHeader *header = nullptr;
        generated.Glyph(i, &header);
        if (header == nullptr) {
            continue;
        }
        checkedGlyphs++;

        OpenType_LongHorMetric mtx;
        generated.GlyphHorMetric(i, mtx);
        if (header->XMin < generated.Head().XMin) xMinBeforeHead++;
        if (header->YMin < generated.Head().YMin) yMinBeforeHead++;
        if (header->XMax > generated.Head().XMax) xMaxAfterHead++;
        if (header->YMax > generated.Head().YMax) yMaxAfterHead++;
        if (header->XMax > mtx.AdvanceWidth) xMaxAfterAdvance++;
        if (header->YMax > generated.Hhea().Ascender) yMaxAfterHheaAscender++;
        if (header->YMin < generated.Hhea().Descender) yMinBeforeHheaDescender++;
        if ((int)header->YMax > (int)generated.OS2().usWinAscent) yMaxAfterWinAscent++;
        if ((int)-header->YMin > (int)generated.OS2().usWinDescent) yMinBeforeWinDescent++;

        if (header->NumberOfContours < 0) {
            const OpenType_GlyphComposite *composite = (const OpenType_GlyphComposite*)header;
            composites++;
            if ((int)composite->SubGlyphs.size() > maxTopComponents) {
                maxTopComponents = (int)composite->SubGlyphs.size();
            }
            int depth = compositeDepth(generated, (uint16_t)i, visiting);
            if (depth > maxDepth) {
                maxDepth = depth;
            }
        } else {
            const OpenType_GlyphSimple *simple = (const OpenType_GlyphSimple*)header;
            actualMaxPoints = std::max(actualMaxPoints, (int)simple->Points.size());
            actualMaxContours = std::max(actualMaxContours, (int)simple->EndPtsOfContours.size());

            std::string glyphName;
            generated.GlyphName(i, glyphName);
            if (glyphName.find("pinyin.") == 0) {
                internalSimpleGlyphs++;
                if (mappedGlyphs.find((uint16_t)i) != mappedGlyphs.end()) {
                    internalMappedGlyphs++;
                }
                if (!simple->Instructions.empty()) internalInstructions++;
                if (!simple->Points.empty()) {
                    int16_t xMin = simple->Points[0].X;
                    int16_t yMin = simple->Points[0].Y;
                    int16_t xMax = simple->Points[0].X;
                    int16_t yMax = simple->Points[0].Y;
                    for (size_t point = 1; point < simple->Points.size(); point++) {
                        xMin = std::min(xMin, simple->Points[point].X);
                        yMin = std::min(yMin, simple->Points[point].Y);
                        xMax = std::max(xMax, simple->Points[point].X);
                        yMax = std::max(yMax, simple->Points[point].Y);
                    }
                    if (xMin != simple->XMin || yMin != simple->YMin ||
                        xMax != simple->XMax || yMax != simple->YMax) {
                        internalBoundsMismatch++;
                        std::fprintf(stdout,
                            "  InternalBoundsMismatch = %s header:(%d,%d,%d,%d) points:(%d,%d,%d,%d)\n",
                            glyphName.c_str(),
                            simple->XMin, simple->YMin, simple->XMax, simple->YMax,
                            xMin, yMin, xMax, yMax);
                    }
                }
            }
        }
    }

    std::fprintf(stdout, "Generated glyph metrics:\n");
    std::fprintf(stdout, "  CheckedGlyphs = %d\n", checkedGlyphs);
    std::fprintf(stdout, "  HeadOverflow = { XMin=%d, YMin=%d, XMax=%d, YMax=%d }\n",
        xMinBeforeHead, yMinBeforeHead, xMaxAfterHead, yMaxAfterHead);
    std::fprintf(stdout, "  AdvanceOverflow = %d\n", xMaxAfterAdvance);
    std::fprintf(stdout, "  HheaVerticalOverflow = { YMaxAboveAscender=%d, YMinBelowDescender=%d }\n",
        yMaxAfterHheaAscender, yMinBeforeHheaDescender);
    std::fprintf(stdout, "  OS2WinVerticalOverflow = { YMaxAboveWinAscent=%d, YMinBelowWinDescent=%d }\n",
        yMaxAfterWinAscent, yMinBeforeWinDescent);
    std::fprintf(stdout, "\n");

    std::fprintf(stdout, "Composite metadata:\n");
    std::fprintf(stdout, "  CompositeGlyphs = %d\n", composites);
    std::fprintf(stdout, "  ActualMaxComponentElements = %d\n", maxTopComponents);
    std::fprintf(stdout, "  SerializedMaxComponentElements = %d\n", (int)generated.Maxp().MaxComponentElements);
    std::fprintf(stdout, "  ActualMaxComponentDepth = %d\n", maxDepth);
    std::fprintf(stdout, "  SerializedMaxComponentDepth = %d\n", (int)generated.Maxp().MaxComponentDepth);
    std::fprintf(stdout, "  ComponentElementsOK = %s\n",
        maxTopComponents <= (int)generated.Maxp().MaxComponentElements ? "yes" : "no");
    std::fprintf(stdout, "  ComponentDepthOK = %s\n",
        maxDepth <= (int)generated.Maxp().MaxComponentDepth ? "yes" : "no");
    std::fprintf(stdout, "\n");

    std::fprintf(stdout, "Internal simple glyph metadata:\n");
    std::fprintf(stdout, "  InternalSimpleGlyphs = %d\n", internalSimpleGlyphs);
    std::fprintf(stdout, "  UnexpectedCmapMappings = %d\n", internalMappedGlyphs);
    std::fprintf(stdout, "  BoundsMismatch = %d\n", internalBoundsMismatch);
    std::fprintf(stdout, "  NonEmptyInstructions = %d\n", internalInstructions);
    std::fprintf(stdout, "  ActualMaxPoints = %d\n", actualMaxPoints);
    std::fprintf(stdout, "  SerializedMaxPoints = %d\n", (int)generated.Maxp().MaxPoints);
    std::fprintf(stdout, "  ActualMaxContours = %d\n", actualMaxContours);
    std::fprintf(stdout, "  SerializedMaxContours = %d\n", (int)generated.Maxp().MaxContours);
    std::fprintf(stdout, "  SimpleMetadataOK = %s\n",
        internalMappedGlyphs == 0 && internalBoundsMismatch == 0 &&
        internalInstructions == 0 &&
        actualMaxPoints <= (int)generated.Maxp().MaxPoints &&
        actualMaxContours <= (int)generated.Maxp().MaxContours ? "yes" : "no");
    std::fprintf(stdout, "\n");
}

static int checkGeneratedFontIntegrity(const char *sourceFile, const char *generatedFile)
{
    OpenType_Font source;
    OpenType_Font generated;
    if (parseFont(sourceFile, source) != 0) {
        return 1;
    }
    if (parseFont(generatedFile, generated) != 0) {
        return 1;
    }

    std::fprintf(stdout, "Integrity check:\n");
    std::fprintf(stdout, "  Source = %s\n", sourceFile);
    std::fprintf(stdout, "  Generated = %s\n", generatedFile);
    std::fprintf(stdout, "\n");

    checkCmapIntegrity(source, generated);
    checkLigatureIntegrity(generated);
    checkMetricIntegrity(source, generated);
    return 0;
}

//------------------------------------------------------------------------------

static bool readWholeFile(FILE *f, std::vector<uint8_t> &data)
{
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    if (fsize < 0)
        return false;
    fseek(f, 0, SEEK_SET);

    if (fsize == 0) {
        return true;
    }
    data.resize((size_t)fsize);
    uint8_t *buffer = &(data[0]);
    if (fread(buffer, 1, (size_t)fsize, f) != (size_t)fsize)
        return false;

    return true;
}

static int dumpTable(
    const std::string &fileName,
    const std::string &tableName,
    const std::string &outputName,
    const std::vector<uint8_t> &fontData,
    uint32_t offset,
    uint32_t length)
{
    if ((offset + length) > (uint32_t)fontData.size()) {
        std::printf("bad font format\n");
        return 1;
    }

    std::string outFileName = outputName.empty()
        ? defaultTableDumpOutput(fileName, tableName)
        : outputName;

    FILE *outFile = fopen(outFileName.c_str(), "wb");
    if (outFile == nullptr) {
        std::printf("can't open output file\n");
        return 1;
    }
    auto outFile_guard = scopeGuard([&outFile]{ fclose(outFile); });

    const uint8_t *data = &(fontData[0]);
    if ((fwrite(data + offset, 1, length, outFile) != length) || fflush(outFile) != 0) {
        std::printf("write table data failed\n");
        return 1;
    }

    std::printf("table dump OK, outfile = %s\n", outFileName.c_str());
    return 0;
}

static uint32_t checksum(const uint8_t *table, uint32_t length)
{
    assert(0 == (length & 3));
    const uint8_t *tableEnd = table + length;
    uint32_t sum = 0;
    while ((table + 4) <= tableEnd) {
        sum += u4(table);
        table += 4;
    }
    return sum;
}

static int purgeTable(
    const std::string &fileName,
    const std::string &tableName,
    const std::string &outputName,
    std::vector<uint8_t> &fontData)
{
    if (tableName == "head") {
        std::printf("can't purge table 'head'\n");
        return 1;
    }

    uint8_t *data = &(fontData[0]);
    size_t offset = 4;
    int numTables = (int)u2(data + offset);
    offset += 2;
    offset += 6;

    uint32_t tableOffset = 0, tableLength = 0;
    bool deleted = false;
    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i * 16;
        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        tableOffset = u4(data + x + 8);
        tableLength = u4(data + x + 12);

        if (std::strcmp(name, tableName.c_str()) == 0) {
            tableLength = ((tableLength - 1) / 4 + 1) * 4;

            fontData.erase(fontData.begin() + tableOffset, fontData.begin() + tableOffset + tableLength);
            fontData.erase(fontData.begin() + x, fontData.begin() + x + 16);

            numTables--;
            deleted = true;
            break;
        }
    }
    if (!deleted) {
        std::printf("table '%s' not found\n", tableName.c_str());
        return 1;
    }

    uint32_t sum = 0;
    uint32_t checksumAdjustmentOffset = 0;
    data = &(fontData[0]);

    uint16_t entrySelector = (uint16_t)(std::floor(std::log2(numTables)));
    uint16_t searchRange = (uint16_t)(std::exp2(entrySelector) * 16);
    uint16_t rangeShift = numTables * 16 - searchRange;
    put_u2(data + 4, numTables);
    put_u2(data + 6, searchRange);
    put_u2(data + 8, entrySelector);
    put_u2(data + 10, rangeShift);

    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i * 16;

        uint32_t tableChecksum = u4(data + x + 4);
        sum += tableChecksum;

        uint32_t tableOffset1 = u4(data + x + 8);
        if (tableOffset1 >= (tableOffset + tableLength)) {
            tableOffset1 = tableOffset1 - tableLength - 16;
        } else {
            tableOffset1 = tableOffset1 - 16;
        }
        put_u4(data + x + 8, tableOffset1);

        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        if (std::strcmp(name, "head") == 0) {
            checksumAdjustmentOffset = tableOffset1 + 8;
        }
    }

    if (checksumAdjustmentOffset != 0) {
        uint32_t length = 12 + 16 * numTables;
        sum += checksum(data, length);

        uint8_t *checksumAdjustment = data + checksumAdjustmentOffset;
        put_u4(checksumAdjustment, 0xB1B0AFBAu - sum);
    }

    std::string outFileName = outputName.empty() ? fileName + ".purged.ttf" : outputName;

    FILE *outFile = fopen(outFileName.c_str(), "wb");
    if (outFile == nullptr) {
        std::printf("can't open output file\n");
        return 1;
    }
    auto outFile_guard = scopeGuard([&outFile]{ fclose(outFile); });

    if ((fwrite(data, 1, fontData.size(), outFile) != fontData.size()) || fflush(outFile) != 0) {
        std::printf("write new file failed\n");
        return 1;
    }

    std::printf("table purge OK, outfile = %s\n", outFileName.c_str());
    return 0;
}

static int tableCommand(const char *mode, const Options &options)
{
    FILE *inFile = fopen(options.input.c_str(), "rb");
    if (inFile == nullptr) {
        std::printf("can't open file %s\n", options.input.c_str());
        return 1;
    }
    auto inFile_guard = scopeGuard([&inFile]{ fclose(inFile); });

    std::vector<uint8_t> fontData;
    if (!readWholeFile(inFile, fontData)) {
        std::printf("read file failed\n");
        return 1;
    }
    if (fontData.size() < 12) {
        std::printf("bad font format\n");
        return 1;
    }
    const uint8_t *data = &(fontData[0]);
    size_t len = fontData.size();
    size_t offset = 0;
    uint32_t magic = u4(data + offset);
    offset += 4;
    switch (magic) {
    case 0x00010000:
    case 0x4F54544F:
    case 0x74727565:
        break;
    default:
        std::printf("bad font format\n");
        return 1;
    }

    int numTables = (int)u2(data + offset);
    offset += 2;
    offset += 6;
    if (numTables <= 0 || len < offset + 16 * numTables) {
        std::printf("bad font format\n");
        return 1;
    }

    uint32_t tableOffset = 0;
    uint32_t tableLength = 0;
    std::printf("[tag]\t[checksum]\t[offset]\t[length]\n");
    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i * 16;
        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        uint32_t tableChecksum = u4(data + x + 4);
        uint32_t currentOffset = u4(data + x + 8);
        uint32_t length = u4(data + x + 12);
        std::printf("%s\t%08x\t%08x\t%08x\n", name, tableChecksum, currentOffset, length);

        if (std::strcmp(name, options.table.c_str()) == 0) {
            tableOffset = currentOffset;
            tableLength = length;
        }
    }
    std::printf("\n");

    if (tableOffset == 0) {
        std::printf("table '%s' not found\n", options.table.c_str());
        return 1;
    }

    if (std::strcmp(mode, "table-dump") == 0) {
        return dumpTable(options.input, options.table, options.output, fontData, tableOffset, tableLength);
    }
    return purgeTable(options.input, options.table, options.output, fontData);
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char *command = argv[1];
    Options options;
    if (!parseOptions(argc, argv, 2, options)) {
        printUsage(argv[0]);
        return 1;
    }

    std::srand((unsigned int)std::time(0));

    if (std::strcmp(command, "info") == 0) {
        if (!requireInput(options)) return 1;
        return printFontInfo(options.input.c_str());
    }
    if (std::strcmp(command, "integrity") == 0) {
        if (!requireSource(options) || !requireInput(options)) return 1;
        return checkGeneratedFontIntegrity(options.source.c_str(), options.input.c_str());
    }
    if (std::strcmp(command, "table-dump") == 0 || std::strcmp(command, "table-purge") == 0) {
        if (!requireInput(options) || !requireTable(options)) return 1;
        return tableCommand(command, options);
    }

    std::fprintf(stderr, "unknown command: %s\n", command);
    printUsage(argv[0]);
    return 1;
}
