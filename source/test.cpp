#include "ot_font_parser.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>

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
    
    testName(font);
    testPost(font);
    testGlyph(font);

    return 0;
}
