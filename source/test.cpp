#include "ot_font_parser.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
using namespace std::chrono;

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <dirent.h>
#endif

//------------------------------------------------------------------------------

static void dumpBasicInfo(const OpenType_Font &font)
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
    fprintf(stdout, "  MaxPoints = %d\n", (int)maxp.MaxPoints);
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

static void dumpName(const OpenType_Font &font)
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

static void dumpPost(const OpenType_Font &font)
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

static void dumpGlyph(const OpenType_Font &font)
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

static void dumpHmtx(const OpenType_Font &font)
{
    fprintf(stdout, "Hhea+Hmtx:\n");
    fprintf(stdout, "  Ascender = %d\n", (int)font.Hhea().Ascender);
    fprintf(stdout, "  Descender = %d\n", (int)font.Hhea().Descender);
    fprintf(stdout, "  LineGap = %d\n", (int)font.Hhea().LineGap);
    fprintf(stdout, "  AdvanceWidthMax = %d\n", (int)font.Hhea().AdvanceWidthMax);
    fprintf(stdout, "  MinLeftSideBearing = %d\n", (int)font.Hhea().MinLeftSideBearing);
    fprintf(stdout, "  MinRightSideBearing = %d\n", (int)font.Hhea().MinRightSideBearing);
    fprintf(stdout, "  XMaxExtent = %d\n", (int)font.Hhea().XMaxExtent);
    fprintf(stdout, "  NumberOfHMetrics = %d\n", (int)font.Hhea().NumberOfHMetrics);
    for (int i = 0; i < 10; i++) {
        int index = std::rand() % font.GlyphCount();
        OpenType_LongHorMetric mtx;
        font.GlyphHorMetric(index, mtx);
        fprintf(stdout, "  Glyph_%d = { Advance=%d, LSB=%d }\n", 
            index, (int)mtx.AdvanceWidth, (int)mtx.LSB);
    }
    fprintf(stdout, "\n");
}

static void dumpCmap(const OpenType_Font &font)
{
    fprintf(stdout, "Cmap:\n");
    fprintf(stdout, "  U+0061 -> %d\n", (int)font.CharToGlyphIndex(0x0061));  // Latin Small Letter A
    fprintf(stdout, "  U+0030 -> %d\n", (int)font.CharToGlyphIndex(0x0030));  // Digit Zero
    fprintf(stdout, "  U+4E2D -> %d\n", (int)font.CharToGlyphIndex(0x4E2D));  // Ideograph central; center, middle; in the midst of; hit (target); attain CJK
    fprintf(stdout, "  U+0304 -> %d\n", (int)font.CharToGlyphIndex(0x0304));  // Combining Macron
    fprintf(stdout, "\n");
}

static int dumpFont(const char *filename)
{
    OpenType_Font font;
    OpenType_Font_Parser parser;
    Status status = parser.Parse(filename, &font);
    if (status != kOk) {
        fprintf(stderr, "Parse failed, error=%d\n", status);
        return 1;
    }
    fprintf(stdout, "Parse succeeded\n");
    fprintf(stdout, "\n");

    dumpBasicInfo(font);
    dumpName(font);
    dumpPost(font);
    dumpGlyph(font);
    dumpHmtx(font);
    dumpCmap(font);

    return 0;
} 

//------------------------------------------------------------------------------

#ifdef _WIN32
static bool walkDir(std::string dir, std::function<void(const char*)> fun)
{
    // TODO
}
#else
static bool walkDir(std::string dir, std::function<void(const char*)> fun)
{
    DIR *d = opendir(dir.c_str());
    if (d == nullptr)
        return false;

    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (entry->d_name[0] == '.' && entry->d_name[1] == '\0')
            continue;
        if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0')
            continue;

        std::string fullpath = dir + "/" + entry->d_name;

        struct stat st = { 0 };
        stat(fullpath.c_str(), &st);
        if (S_ISDIR(st.st_mode)) {
            walkDir(fullpath, fun);
        } else {
            size_t len = strlen(entry->d_name);
            if (len > 4) {
                if (strcasecmp(&entry->d_name[len - 4], ".ttf") == 0) {
                    fun(fullpath.c_str());
                }
            }
        }
    }

    closedir(d);
    return true;
}
#endif

static int benchParse(const char *path)
{
    int totalFile = 0, failedFile = 0;
    long long totalTime = 0;
    walkDir(path, [&](const char *filename){ 
        fprintf(stdout, "%s\n", filename);
        totalFile++;

        OpenType_Font font;
        OpenType_Font_Parser parser;
        
        auto start = system_clock::now();
        Status status = parser.Parse(filename, &font);
        if (status != kOk) {
            fprintf(stdout, "  Parse failed, error=%d\n", status);
            failedFile++;
            return;
        }
        auto elapsedTime = duration_cast<microseconds>(system_clock::now() - start);
        fprintf(stdout, "  Parse succeeded, time=%.2fms\n", elapsedTime.count()/1000.0);
        totalTime += elapsedTime.count();
    });
    fprintf(stdout, "\n");
    fprintf(stdout, "totalFile = %d, failedFile = %d\n", totalFile, failedFile);
    if (totalFile > failedFile) {
        fprintf(stdout, "avgParseTime = %.2fms\n", totalTime/1000.0/(totalFile - failedFile));
    }
    return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stdout, "usage: %s mode path\n", argv[0]);
        return 1;
    }
    const char *mode = argv[1];
    const char *path = argv[2];
    fprintf(stdout, "mode = %s\n", mode);
    fprintf(stdout, "path = %s\n", path);
    fprintf(stdout, "\n");

    std::srand(std::time(0));

    if (strcmp(mode, "dump") == 0) {
        return dumpFont(path);
    } else if (strcmp(mode, "bench") == 0) {
        return benchParse(path);
    } else {
        fprintf(stdout, "unknown mode\n");
        return 1;
    }
}
