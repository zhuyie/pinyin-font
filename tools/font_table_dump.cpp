#include <cstdio>
#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
#include "scope_guard.h"
#include "utility.h"

static bool __readWholeFile(FILE *f, std::vector<uint8_t> &data)
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

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("usage: %s filename tablename\n", argv[0]);
        return 1;
    }
    std::string filename = argv[1];
    std::string tablename = argv[2];

    FILE *inFile = fopen(filename.c_str(), "rb");
    if (inFile == NULL) {
        printf("can't open file %s\n", filename.c_str());
        return 1;
    }
    auto inFile_guard = scopeGuard([&inFile]{ fclose(inFile); });

    std::vector<uint8_t> fontData;
    if (!__readWholeFile(inFile, fontData)) {
        printf("read file failed\n");
        return 1;
    }
    if (fontData.size() < 12) {
        printf("bad font format\n");
        return 1;
    }
    const uint8_t *data = &(fontData[0]);
    size_t len = fontData.size();
    size_t offset = 0;
    uint32_t magic = u4(data + offset);
    offset += 4;
    switch (magic) {
    case 0x00010000:
    case 0x4F54544F: // 'OTTO'
    case 0x74727565: // 'true'
        break;
    default:
        printf("bad font format\n");
        return 1;
    }

    int numTables = (int)u2(data + offset);
    offset += 2;
    offset += 6; // Skip the searchRange, entrySelector and rangeShift.
    if (numTables <= 0 || len < offset + 16*numTables) {
        printf("bad font format\n");
        return 1;
    }

    uint32_t tableOffset = 0;
    uint32_t tableLength = 0;
    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i*16; // the offset of the current table start
        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        if (strcmp(name, tablename.c_str()) == 0) {
            tableOffset = u4(data + x + 8);
            tableLength = u4(data + x + 12);
            break;
        }
    }

    if (tableOffset == 0) {
        printf("table '%s' not found\n", tablename.c_str());
        return 1;
    }
    if (tableOffset + tableLength > len) {
        printf("bad font format\n");
        return 1;
    }

    std::string outFileName = filename;
    outFileName += ".";
    if (strcmp(tablename.c_str(), "OS/2") == 0) {
        outFileName += "OS2";
    } else {
        outFileName += tablename;
    }
    outFileName += ".dat";
    
    FILE *outFile = fopen(outFileName.c_str(), "wb");
    if (outFile == NULL) {
        printf("can't open output file\n");
        return 1;
    }
    auto outFile_guard = scopeGuard([&outFile]{ fclose(outFile); });
    
    if ((fwrite(data + tableOffset, 1, tableLength, outFile) != tableLength) || fflush(outFile) != 0) {
        printf("write table data failed\n");
        return 1;
    }

    printf("dump OK\n");
    return 0;
}
