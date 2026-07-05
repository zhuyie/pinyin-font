#include "ot_font_parser.h"
#include "ot_font_writer.h"
#include "scope_guard.h"
#include "utility.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <set>
#include <string>
#include <vector>
using namespace std::chrono;

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <dirent.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

//------------------------------------------------------------------------------

struct Options {
    std::string input;
    std::string output;
    std::string table;
};

static void printUsage(const char *program)
{
    std::fprintf(stdout, "usage:\n");
    std::fprintf(stdout, "  %s info --input <font.ttf>\n", program);
    std::fprintf(stdout, "  %s bench-parse --input <font-directory>\n", program);
    std::fprintf(stdout, "  %s rewrite --input <font.ttf> [--output <out.ttf>]\n", program);
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

static bool requireTable(const Options &options)
{
    if (options.table.empty()) {
        std::fprintf(stderr, "missing required option: --table\n");
        return false;
    }
    return true;
}

static std::string defaultRewriteOutput(const std::string &fileName)
{
    std::string output = fileName;
    if (output.length() > 4 && output[output.size() - 4] == '.') {
        output.resize(output.length() - 4);
    }
    output += ".rewrite.ttf";
    return output;
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

#ifdef _WIN32
static bool walkDir(std::string dir, std::function<void(const char*)> fun)
{
    std::string searchPath = dir + "\\*";
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;

    do {
        if (ffd.cFileName[0] == '.' && ffd.cFileName[1] == '\0')
            continue;
        if (ffd.cFileName[0] == '.' && ffd.cFileName[1] == '.' && ffd.cFileName[2] == '\0')
            continue;

        std::string fullpath = dir + "\\" + ffd.cFileName;

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            walkDir(fullpath, fun);
        } else {
            size_t len = std::strlen(ffd.cFileName);
            if (len > 4 && _stricmp(&ffd.cFileName[len - 4], ".ttf") == 0) {
                fun(fullpath.c_str());
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
    return true;
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
            size_t len = std::strlen(entry->d_name);
            if (len > 4 && strcasecmp(&entry->d_name[len - 4], ".ttf") == 0) {
                fun(fullpath.c_str());
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
    long long totalTime = 0, minTime = -1, maxTime = -1;
    walkDir(path, [&](const char *filename) {
        std::fprintf(stdout, "%s\n", filename);
        totalFile++;

        OpenType_Font font;
        OpenType_Font_Parser parser;

        auto start = system_clock::now();
        Status status = parser.Parse(filename, &font);
        if (status != kOk) {
            std::fprintf(stdout, "  Parse failed, error=%d\n", status);
            failedFile++;
            return;
        }
        auto elapsedTime = duration_cast<microseconds>(system_clock::now() - start);
        std::fprintf(stdout, "  Parse succeeded, time=%.2fms\n", elapsedTime.count() / 1000.0);
        totalTime += elapsedTime.count();
        if (minTime == -1 || elapsedTime.count() < minTime) {
            minTime = elapsedTime.count();
        }
        if (maxTime == -1 || elapsedTime.count() > maxTime) {
            maxTime = elapsedTime.count();
        }
    });
    std::fprintf(stdout, "\n");
    std::fprintf(stdout, "totalFile = %d, failedFile = %d\n", totalFile, failedFile);
    if (totalFile > failedFile) {
        std::fprintf(stdout, "ParseTime: avg=%.2fms, min=%.2fms, max=%.2fms\n",
            totalTime / 1000.0 / (totalFile - failedFile),
            minTime / 1000.0,
            maxTime / 1000.0);
    }
    std::fprintf(stdout, "\n");
    return 0;
}

static int rewriteFont(const char *filename, const char *outputFile)
{
    OpenType_Font font;

    OpenType_Font_Parser parser;
    Status status = parser.Parse(filename, &font);
    if (status != kOk) {
        std::fprintf(stderr, "Parse failed, error=%d\n", status);
        return 1;
    }
    std::fprintf(stdout, "Parse succeeded\n");

    std::string derivedOutput;
    if (outputFile == nullptr) {
        derivedOutput = defaultRewriteOutput(filename);
        outputFile = derivedOutput.c_str();
    }

    OpenType_Font_Writer writer;
    status = writer.Write(outputFile, &font);
    if (status != kOk) {
        std::fprintf(stderr, "Write failed, error=%d\n", status);
        return 1;
    }
    std::fprintf(stdout, "Rewrite succeeded, outfile = %s\n", outputFile);
    std::fprintf(stdout, "\n");
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
    if (std::strcmp(command, "bench-parse") == 0) {
        if (!requireInput(options)) return 1;
        return benchParse(options.input.c_str());
    }
    if (std::strcmp(command, "rewrite") == 0) {
        if (!requireInput(options)) return 1;
        const char *output = options.output.empty() ? nullptr : options.output.c_str();
        return rewriteFont(options.input.c_str(), output);
    }
    if (std::strcmp(command, "table-dump") == 0 || std::strcmp(command, "table-purge") == 0) {
        if (!requireInput(options) || !requireTable(options)) return 1;
        return tableCommand(command, options);
    }

    std::fprintf(stderr, "unknown command: %s\n", command);
    printUsage(argv[0]);
    return 1;
}
