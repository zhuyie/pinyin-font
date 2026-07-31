#include "ot_cmap.h"
#include "cmap_integrity.h"
#include "ot_cmap_encoder.h"
#include "test_runner.h"
#include "utility.h"
#include <cstdio>
#include <map>
#include <vector>

static bool expect(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        return false;
    }
    return true;
}

static void collect(
    void *userdata,
    CmapSequentialMapGroup group)
{
    std::map<uint32_t, uint16_t> *mappings =
        static_cast<std::map<uint32_t, uint16_t> *>(userdata);
    for (uint32_t code = group.startCharCode;
         code <= group.endCharCode; code++) {
        (*mappings)[code] = (uint16_t)(
            group.startGlyphID + code - group.startCharCode);
        if (code == 0xFFFFFFFFu) break;
    }
}

static bool parseFormat4(
    const std::vector<uint8_t> &data,
    std::map<uint32_t, uint16_t> &mappings)
{
    std::unique_ptr<CmapSubtable> subtable =
        CreateCmapSubtableFormat4(3, 1);
    return subtable->Parse(
        &data[0] + 2, &data[0] + data.size(),
        collect, &mappings) == kOk;
}

static std::vector<uint8_t> makeCmapTable(
    const std::vector<uint8_t> &format4,
    const std::vector<CmapSequentialMapGroup> &format12Groups)
{
    uint32_t format12Length =
        16 + (uint32_t)format12Groups.size() * 12;
    std::vector<uint8_t> table(
        20 + format4.size() + format12Length, 0);
    put_u2(&table[0] + 2, 2);
    put_u2(&table[0] + 4, 3);
    put_u2(&table[0] + 6, 1);
    put_u4(&table[0] + 8, 20);
    put_u2(&table[0] + 12, 3);
    put_u2(&table[0] + 14, 10);
    put_u4(&table[0] + 16, 20 + (uint32_t)format4.size());
    std::copy(format4.begin(), format4.end(), table.begin() + 20);

    uint8_t *format12 = &table[0] + 20 + format4.size();
    put_u2(format12, 12);
    put_u4(format12 + 4, format12Length);
    put_u4(format12 + 12, (uint32_t)format12Groups.size());
    for (size_t i = 0; i < format12Groups.size(); i++) {
        const CmapSequentialMapGroup &group = format12Groups[i];
        uint8_t *record = format12 + 16 + i * 12;
        put_u4(record, group.startCharCode);
        put_u4(record + 4, group.endCharCode);
        put_u4(record + 8, group.startGlyphID);
    }
    return table;
}

static std::vector<CmapSequentialMapGroup> alternatingGroups(size_t count)
{
    std::vector<CmapSequentialMapGroup> groups;
    groups.reserve(count);
    for (size_t i = 0; i < count; i++) {
        CmapSequentialMapGroup group = {
            (uint32_t)i, (uint32_t)i, (uint32_t)(i & 1 ? 3 : 1)
        };
        groups.push_back(group);
    }
    return groups;
}

PINYINFONT_TEST(cmap_format4)
{
    bool ok = true;

    std::vector<CmapSequentialMapGroup> mixed;
    mixed.push_back({ 0x20, 0x22, 10 });
    mixed.push_back({ 0x100, 0x100, 20 });
    mixed.push_back({ 0x102, 0x102, 22 });
    std::vector<uint8_t> format4;
    CmapFormat4Stats stats = {};
    ok = expect(
        EncodeCmapFormat4(mixed, format4, &stats) == kOk,
        "mixed format 4 build failed") && ok;
    ok = expect(stats.glyphIdCount == 3,
        "short sparse gap was not encoded in glyphIdArray") && ok;
    std::map<uint32_t, uint16_t> decoded;
    ok = expect(parseFormat4(format4, decoded),
        "mixed format 4 did not parse") && ok;
    ok = expect(decoded[0x20] == 10 && decoded[0x22] == 12,
        "delta segment did not round trip") && ok;
    ok = expect(decoded[0x100] == 20 && decoded[0x101] == 0 &&
        decoded[0x102] == 22,
        "glyph array mapping or zero-filled gap did not round trip") && ok;

    std::vector<CmapSequentialMapGroup> compact = alternatingGroups(10000);
    ok = expect(
        EncodeCmapFormat4(compact, format4, &stats) == kOk,
        "compact overflow regression build failed") && ok;
    ok = expect(stats.segmentCount == 2 && stats.length < 65536,
        "fragmented mappings were not compacted") && ok;
    ok = expect(parseFormat4(format4, decoded),
        "compacted format 4 did not parse") && ok;

    std::vector<CmapSequentialMapGroup> boundary =
        alternatingGroups(32751);
    ok = expect(
        EncodeCmapFormat4(boundary, format4, &stats) == kOk,
        "largest legal format 4 build failed") && ok;
    ok = expect(stats.length == 65534 && u2(&format4[0] + 2) == 65534,
        "largest legal format 4 length is incorrect") && ok;

    std::vector<CmapSequentialMapGroup> overflow =
        alternatingGroups(32752);
    ok = expect(
        EncodeCmapFormat4(overflow, format4, &stats) == kNotSupported,
        "format 4 overflow did not fail explicitly") && ok;
    ok = expect(format4.empty(),
        "failed format 4 build retained partial output") && ok;

    std::vector<CmapSequentialMapGroup> consistent;
    consistent.push_back({ 0x100, 0x100, 20 });
    consistent.push_back({ 0x102, 0x102, 22 });
    ok = expect(
        EncodeCmapFormat4(consistent, format4, NULL) == kOk,
        "validation fixture format 4 build failed") && ok;
    std::vector<uint8_t> cmap = makeCmapTable(format4, consistent);
    CmapValidationReport report = {};
    ok = expect(ValidateCmapTable(&cmap[0], cmap.size(), &report) == kOk,
        "consistent cmap table failed validation") && ok;
    ok = expect(report.format4Subtables == 1 &&
        report.format12Subtables == 1 && report.bmpMismatches == 0,
        "consistent cmap validation report is incorrect") && ok;

    std::vector<uint8_t> corrupt = cmap;
    put_u2(&corrupt[0] + 22, 14);
    ok = expect(
        ValidateCmapTable(&corrupt[0], corrupt.size(), NULL) == kCorruption,
        "malformed sibling format 4 was not detected") && ok;

    std::vector<CmapSequentialMapGroup> inconsistent = consistent;
    inconsistent[0].startGlyphID = 21;
    std::vector<uint8_t> mismatch = makeCmapTable(format4, inconsistent);
    report = CmapValidationReport();
    ok = expect(
        ValidateCmapTable(&mismatch[0], mismatch.size(), &report) ==
            kCorruption,
        "inconsistent BMP mappings were not rejected") && ok;
    ok = expect(report.bmpMismatches == 1,
        "BMP mismatch was not reported") && ok;

    return ok ? 0 : 1;
}
