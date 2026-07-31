#ifndef __PINYIN_FONT_CMAP_INTEGRITY_H__
#define __PINYIN_FONT_CMAP_INTEGRITY_H__

#include "status.h"
#include <cstddef>
#include <cstdint>

struct CmapValidationReport {
    uint32_t unicodeSubtables;
    uint32_t format4Subtables;
    uint32_t format12Subtables;
    uint32_t bmpMismatches;
};

Status ValidateCmapTable(
    const uint8_t *data,
    size_t length,
    CmapValidationReport *report);

Status ValidateFontCmapFile(
    const char *filename,
    CmapValidationReport *report);

#endif
