#include "cmap_integrity.h"
#include "ot_cmap.h"
#include "scope_guard.h"
#include "utility.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

namespace {

struct ParsedCmap {
    std::map<uint32_t, uint16_t> mappings;
    bool invalid;

    ParsedCmap() : invalid(false) {}
};

static void collectMapping(void *userdata, CmapSequentialMapGroup group)
{
    ParsedCmap *parsed = static_cast<ParsedCmap *>(userdata);
    for (uint32_t code = group.startCharCode;
         code <= group.endCharCode; code++) {
        uint32_t glyph = group.startGlyphID + code - group.startCharCode;
        if (glyph > 0xFFFF) {
            parsed->invalid = true;
            return;
        }
        parsed->mappings[code] = (uint16_t)glyph;
        if (code == 0xFFFFFFFFu) break;
    }
}

static bool isUnicodeEncoding(uint16_t platform, uint16_t encoding)
{
    return platform == 0 ||
        (platform == 3 && (encoding == 1 || encoding == 10));
}

static Status parseSupportedSubtable(
    const uint8_t *start,
    size_t available,
    uint16_t platform,
    uint16_t encoding,
    uint16_t format,
    ParsedCmap &parsed)
{
    uint32_t declaredLength = 0;
    std::unique_ptr<CmapSubtable> subtable;
    if (format == 4) {
        if (available < 4) return kCorruption;
        declaredLength = u2(start + 2);
        if (declaredLength < 16) return kCorruption;
        subtable = CreateCmapSubtableFormat4(platform, encoding);
    } else if (format == 12) {
        if (available < 8 || u2(start + 2) != 0) return kCorruption;
        declaredLength = u4(start + 4);
        if (declaredLength < 16) return kCorruption;
        subtable = CreateCmapSubtableFormat12(platform, encoding);
    } else {
        return kNotSupported;
    }
    if (declaredLength > available) return kCorruption;

    Status status = subtable->Parse(
        start + 2, start + declaredLength, collectMapping, &parsed);
    if (status != kOk || parsed.invalid) return kCorruption;
    return kOk;
}

} // namespace

Status ValidateCmapTable(
    const uint8_t *data,
    size_t length,
    CmapValidationReport *report)
{
    if (data == NULL || length < 4) return kInvalidArgs;
    CmapValidationReport local = {};
    uint16_t numTables = u2(data + 2);
    if (length < 4 + (size_t)numTables * 8) return kCorruption;

    std::vector<ParsedCmap> parsedTables;
    for (uint16_t i = 0; i < numTables; i++) {
        const uint8_t *record = data + 4 + i * 8;
        uint16_t platform = u2(record);
        uint16_t encoding = u2(record + 2);
        uint32_t offset = u4(record + 4);
        if (offset >= length) return kCorruption;
        if (!isUnicodeEncoding(platform, encoding)) continue;
        if (length - offset < 2) return kCorruption;

        uint16_t format = u2(data + offset);
        if (format != 4 && format != 12) continue;
        ParsedCmap parsed;
        Status status = parseSupportedSubtable(
            data + offset, length - offset,
            platform, encoding, format, parsed);
        if (status != kOk) return status;
        local.unicodeSubtables++;
        if (format == 4) local.format4Subtables++;
        if (format == 12) local.format12Subtables++;
        parsedTables.push_back(parsed);
    }
    if (parsedTables.empty()) return kNotSupported;

    for (size_t left = 0; left < parsedTables.size(); left++) {
        for (size_t right = left + 1; right < parsedTables.size(); right++) {
            const std::map<uint32_t, uint16_t> &a =
                parsedTables[left].mappings;
            const std::map<uint32_t, uint16_t> &b =
                parsedTables[right].mappings;
            for (std::map<uint32_t, uint16_t>::const_iterator it = a.begin();
                 it != a.end() && it->first <= 0xFFFF; ++it) {
                std::map<uint32_t, uint16_t>::const_iterator other =
                    b.find(it->first);
                if (other != b.end() && other->second != it->second) {
                    local.bmpMismatches++;
                }
            }
        }
    }
    if (report != NULL) *report = local;
    return local.bmpMismatches == 0 ? kOk : kCorruption;
}

Status ValidateFontCmapFile(
    const char *filename,
    CmapValidationReport *report)
{
    if (filename == NULL) return kInvalidArgs;
    FILE *file = fopen(filename, "rb");
    if (file == NULL) return kFileError;
    auto fileGuard = scopeGuard([&file]{ fclose(file); });
    if (fseek(file, 0, SEEK_END) != 0) return kFileError;
    long fileSize = ftell(file);
    if (fileSize < 12 || fseek(file, 0, SEEK_SET) != 0) return kCorruption;
    std::vector<uint8_t> data((size_t)fileSize);
    if (fread(&data[0], 1, data.size(), file) != data.size()) {
        return kFileError;
    }

    uint16_t numTables = u2(&data[0] + 4);
    if (data.size() < 12 + (size_t)numTables * 16) return kCorruption;
    for (uint16_t i = 0; i < numTables; i++) {
        const uint8_t *record = &data[0] + 12 + i * 16;
        if (memcmp(record, "cmap", 4) != 0) continue;
        uint32_t offset = u4(record + 8);
        uint32_t length = u4(record + 12);
        if (offset > data.size() || length > data.size() - offset) {
            return kCorruption;
        }
        return ValidateCmapTable(&data[0] + offset, length, report);
    }
    return kNotFound;
}
