#include "ot_font_parser.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>

static void testBasicInfo(const OpenType_Font &font)
{
    const OpenType_Head &head = font.Head();
    fprintf(stdout, "Head:\n");
    fprintf(stdout, "  Version = 0x%08x\n", head.Version);
    fprintf(stdout, "  Flags = 0x%04x\n", (unsigned int)head.Flags);
    fprintf(stdout, "  UnitsPerEm = %d\n", (int)head.UnitsPerEm);
    fprintf(stdout, "  MacStyle = 0x%04x\n", (unsigned int)head.MacStyle);
    fprintf(stdout, "\n");
    const OpenType_Maxp &maxp = font.Maxp();
    fprintf(stdout, "Maxp:\n");
    fprintf(stdout, "  Version = 0x%08x\n", maxp.Version);
    fprintf(stdout, "  NumGlyphs = %d\n", (int)maxp.NumGlyphs);
    fprintf(stdout, "  MaxContours = %d\n", (int)maxp.MaxContours);
    fprintf(stdout, "  MaxCompositePoints = %d\n", (int)maxp.MaxCompositePoints);
    fprintf(stdout, "  MaxCompositeContours = %d\n", (int)maxp.MaxCompositeContours);
    fprintf(stdout, "  MaxComponentElements = %d\n", (int)maxp.MaxComponentElements);
    fprintf(stdout, "  MaxComponentDepth = %d\n", (int)maxp.MaxComponentDepth);
    fprintf(stdout, "\n");
    const OpenType_OS2 &os2 = font.OS2();
    fprintf(stdout, "OS/2:\n");
    fprintf(stdout, "  version = %d\n", (int)os2.version);
    fprintf(stdout, "  xAvgCharWidth = %d\n", (int)os2.xAvgCharWidth);
    fprintf(stdout, "  usWeightClass = %u\n", (unsigned int)os2.usWeightClass);
    fprintf(stdout, "  usWidthClass = %u\n", (unsigned int)os2.usWidthClass);
    fprintf(stdout, "  fsType = 0x%04x\n", (unsigned int)os2.fsType);
    fprintf(stdout, "\n");
}

static void __printNameRecords(const char *name, const std::vector<OpenType_NameRecord> &records)
{
    if (records.size() > 0) {
        for (size_t i = 0; i < records.size(); i++) {
            fprintf(stdout, "  %s = %ls (%d, %d, %d)\n", 
                name, records[i].String.c_str(), records[i].PlatformID, records[i].EncodingID, records[i].LanguageID);
        }
    } else {
        fprintf(stdout, "  %s = <NotFound>\n", name);
    }
}

static void testName(const OpenType_Font &font)
{
    std::vector<OpenType_NameRecord> nameRecords;

    fprintf(stdout, "Name:\n");

    font.Name(1, nameRecords);  // Font Family name
    __printNameRecords("FamilyName", nameRecords);
    font.Name(2, nameRecords);  // Font Subfamily name
    __printNameRecords("SubfamilyName", nameRecords);
    font.Name(3, nameRecords);  // Unique font identifier
    __printNameRecords("UniqueFontIdentifier", nameRecords);
    font.Name(4, nameRecords);  // Full font name
    __printNameRecords("FullName", nameRecords);
    font.Name(5, nameRecords);  // Version string
    __printNameRecords("VersionString", nameRecords);
    font.Name(6, nameRecords);  // PostScript name
    __printNameRecords("PostScriptName", nameRecords);
    font.Name(7, nameRecords);  // Trademark
    __printNameRecords("Trademark", nameRecords);
    font.Name(8, nameRecords);  // Manufacturer Name
    __printNameRecords("ManufacturerName", nameRecords);
    font.Name(9, nameRecords);  // Designer
    __printNameRecords("Designer", nameRecords);
    font.Name(10, nameRecords);  // Description
    __printNameRecords("Description", nameRecords);

    fprintf(stdout, "\n");
}

static void testPost(const OpenType_Font &font)
{
    fprintf(stdout, "Post:\n");
    fprintf(stdout, "  Version = 0x%08x\n", font.Post().Version);
    fprintf(stdout, "  IsFixedPitch = %u\n", (unsigned int)font.Post().IsFixedPitch);
    
    std::string name;
    for (int i = 0; i < 10; i++) {
        int index = std::rand() % font.GlyphCount();
        font.GlyphName(index, name);
        fprintf(stdout, "  GlyphName_%d = %s\n", index, name.c_str());
    }

    fprintf(stdout, "\n");
}

static void testGlyph(const OpenType_Font &font)
{
    fprintf(stdout, "Glyph:\n");
    fprintf(stdout, "  Count = %d\n", font.GlyphCount());
    for (int i = 0; i < 10; i++) {
        int index = std::rand() % font.GlyphCount();
        OpenType_GlyphHeader *pHeader = NULL;
        font.Glyph(index, &pHeader);
        if (pHeader == NULL) {
            fprintf(stdout, "  Glyph_%d = <NoOutline>\n", index);
        } else if (pHeader->NumberOfContours >= 0) {
            OpenType_GlyphSimple *pSimple = (OpenType_GlyphSimple*)pHeader;
            fprintf(stdout, "  Glyph_%d = Simple{ Contours=%d Points=%d }\n", 
                index, (int)pSimple->NumberOfContours, (int)pSimple->Points.size());
        } else {
            OpenType_GlyphComposite *pComposite = (OpenType_GlyphComposite*)pHeader;
            fprintf(stdout, "  Glyph_%d = Composite{", index);
            for (size_t j = 0; j < pComposite->SubGlyphs.size(); j++) {
                fprintf(stdout, " %d", (int)pComposite->SubGlyphs[j].GlyphIndex);
            }
            fprintf(stdout, " }\n");
        }
    }
    fprintf(stdout, "\n");
}

int main(int argc, char* argv[])
{
    const char *filename = "input.ttf";
    if (argc >= 2) {
        filename = argv[1];
    }
    fprintf(stdout, "filename = %s\n", filename);

    std::srand(std::time(0));

    OpenType_Font font;
    OpenType_Font_Parser parser;
    Status status = parser.Parse(filename, &font);
    if (status != kOk) {
        fprintf(stderr, "Parse failed, error=%d\n", status);
        return 1;
    }
    fprintf(stdout, "Parse succeed\n");
    fprintf(stdout, "\n");

    testBasicInfo(font);
    testName(font);
    testPost(font);
    testGlyph(font);

    return 0;
}
