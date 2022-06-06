#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include "scope_guard.h"
#include "utility.h"

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
    const std::vector<uint8_t> &fontData, 
    uint32_t offset, 
    uint32_t length)
{
    if ((offset + length) > (uint32_t)fontData.size()) {
        printf("bad font format\n");
        return 1;
    }

    std::string outFileName = fileName;
    outFileName += ".";
    if (strcmp(tableName.c_str(), "OS/2") == 0) {
        outFileName += "OS2";
    } else {
        outFileName += tableName;
    }
    outFileName += ".dat";

    FILE *outFile = fopen(outFileName.c_str(), "wb");
    if (outFile == NULL) {
        printf("can't open output file\n");
        return 1;
    }
    auto outFile_guard = scopeGuard([&outFile]{ fclose(outFile); });

    const uint8_t *data = &(fontData[0]);
    if ((fwrite(data + offset, 1, length, outFile) != length) || fflush(outFile) != 0) {
        printf("write table data failed\n");
        return 1;
    }

    printf("dump OK\n");
    return 0;
}

static uint32_t __checksum(const uint8_t *table, uint32_t length)
{
    assert (0 == (length & 3));
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
    std::vector<uint8_t> &fontData)
{
    if (tableName == "head") {
        printf("can't purge table 'head'\n");
        return 1;
    }

    uint8_t *data = &(fontData[0]);
    size_t offset = 4;  // skip magic
    int numTables = (int)u2(data + offset);
    offset += 2;
    offset += 6; // Skip the searchRange, entrySelector and rangeShift.

    uint32_t tableOffset = 0, tableLength = 0;
    bool deleted = false;
    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i*16; // the offset of the current table start
        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        tableOffset = u4(data + x + 8);
        tableLength = u4(data + x + 12);

        if (strcmp(name, tableName.c_str()) == 0) {
            tableLength = ((tableLength - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries

            // 1. erase table content
            fontData.erase(fontData.begin() + tableOffset, fontData.begin() + tableOffset + tableLength);
            // 2. erase corresponding tableRecord
            fontData.erase(fontData.begin() + x, fontData.begin() + x + 16);

            numTables--;
            deleted = true;
            break;
        }
    }
    if (!deleted) {
        printf("table '%s' not found\n", tableName.c_str());
        return 1;
    }

    uint32_t sum = 0;
    uint32_t checksumAdjustmentOffset = 0;
    data = &(fontData[0]);

    // 3. update numTables/searchRange/entrySelector/rangeShift
    uint16_t entrySelector = (uint16_t)(std::floor(std::log2(numTables)));
    uint16_t searchRange = (uint16_t)(std::exp2(entrySelector) * 16);
    uint16_t rangeShift = numTables * 16 - searchRange;
    put_u2(data + 4,  numTables);
    put_u2(data + 6,  searchRange);
    put_u2(data + 8,  entrySelector);
    put_u2(data + 10, rangeShift);

    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i*16; // the offset of the current table start

        uint32_t checksum = u4(data + x + 4);
        sum += checksum;

        // 4. update table offset
        uint32_t tableOffset1 = u4(data + x + 8);
        if (tableOffset1 >= (tableOffset + tableLength)) {
            tableOffset1 = tableOffset1 - tableLength - 16;
        } else {
            tableOffset1 = tableOffset1 - 16;
        }
        put_u4(data + x + 8, tableOffset1);

        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        if (strcmp(name, "head") == 0) {
            checksumAdjustmentOffset = tableOffset1 + 8;
        }
    }

    // 5. update head.checksumAdjustment
    if (checksumAdjustmentOffset != 0) {
        uint32_t length = 12 + 16 * numTables;
        sum += __checksum(data, length);  // checksum of table directory

        uint8_t* checksumAdjustment = data + checksumAdjustmentOffset;
        put_u4(checksumAdjustment, 0xB1B0AFBAu - sum);
    }

    std::string outFileName = fileName;
    outFileName += ".purged.ttf";

    FILE *outFile = fopen(outFileName.c_str(), "wb");
    if (outFile == NULL) {
        printf("can't open output file\n");
        return 1;
    }
    auto outFile_guard = scopeGuard([&outFile]{ fclose(outFile); });

    if ((fwrite(data, 1, fontData.size(), outFile) != fontData.size()) || fflush(outFile) != 0) {
        printf("write new file failed\n");
        return 1;
    }

    printf("purge OK\n");
    return 0;
}

int main(int argc, char* argv[])
{
    std::string mode, fileName, tableName;
    if (argc < 3) {
        printf("usage: %s mode filename [tablename]\n", argv[0]);
        printf("       mode: dump, purge\n");
        printf("   filename: file path of the font file\n");
        printf("  tablename: sub-table name\n");
        printf("\n");
        return 1;
    }
    mode = argv[1];
    fileName = argv[2];
    if (argc > 3) {
        tableName = argv[3];
    }

    FILE *inFile = fopen(fileName.c_str(), "rb");
    if (inFile == NULL) {
        printf("can't open file %s\n", fileName.c_str());
        return 1;
    }
    auto inFile_guard = scopeGuard([&inFile]{ fclose(inFile); });

    std::vector<uint8_t> fontData;
    if (!readWholeFile(inFile, fontData)) {
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
    printf("[tag]\t[checksum]\t[offset]\t[length]\n");
    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i*16; // the offset of the current table start
        char name[5] = { 0 };
        memcpy(name, data + x, 4);
        uint32_t checksum = u4(data + x + 4);
        uint32_t offset = u4(data + x + 8);
        uint32_t length = u4(data + x + 12);
        printf("%s\t%08x\t%08x\t%08x\n", name, checksum, offset, length);

        if (strcmp(name, tableName.c_str()) == 0) {
            tableOffset = offset;
            tableLength = length;
        }
    }
    printf("\n");

    if (tableName.empty()) {
        return 0;
    }
    if (tableOffset == 0) {
        printf("table '%s' not found\n", tableName.c_str());
        return 1;
    }

    if (mode == "dump") {
        return dumpTable(fileName, tableName, fontData, tableOffset, tableLength);
    } else if (mode == "purge") {
        return purgeTable(fileName, tableName, fontData);
    } else {
        printf("unknown mode\n");
        return 1;
    }
}
