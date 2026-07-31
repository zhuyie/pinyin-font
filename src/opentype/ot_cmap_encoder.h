#ifndef __PINYIN_FONT_OT_CMAP_ENCODER_H__
#define __PINYIN_FONT_OT_CMAP_ENCODER_H__

#include "ot_cmap.h"
#include "status.h"
#include <cstdint>
#include <vector>

struct CmapFormat4Stats {
    uint16_t segmentCount;
    uint32_t glyphIdCount;
    uint32_t length;
};

Status EncodeCmapFormat4(
    const std::vector<CmapSequentialMapGroup> &groups,
    std::vector<uint8_t> &output,
    CmapFormat4Stats *stats = NULL);

#endif
