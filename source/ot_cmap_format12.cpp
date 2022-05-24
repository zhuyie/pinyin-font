#include "ot_cmap.h"
#include "utility.h"
#include <vector>

//------------------------------------------------------------------------------

class CmapSubtableFormat12 : public CmapSubtable
{
    std::vector<CmapSequentialMapGroup> groups_;

public:
    CmapSubtableFormat12(uint16_t platformId, uint16_t encodingId);
    virtual Status Parse(const uint8_t *start, const uint8_t *end, CmapParseCallback cb, void *cbUserdata);
    virtual uint16_t Query(uint32_t code);
    virtual void Dump();
};

CmapSubtableFormat12::CmapSubtableFormat12(uint16_t platformId, uint16_t encodingId)
 : CmapSubtable(12, platformId, encodingId)
{
}

// https://docs.microsoft.com/en-us/typography/opentype/spec/cmap#format-12-segmented-coverage
Status CmapSubtableFormat12::Parse(const uint8_t *start, const uint8_t *end, CmapParseCallback cb, void *cbUserdata)
{
    const uint8_t *b = start;
    if (b + 14 > end) {
        return kCorruption;
    }
    uint32_t numGroups = u4(b + 10);
    b += 14;
    if (b + numGroups*12 > end) {
        return kCorruption;
    }
    groups_.reserve(numGroups);
    for (uint32_t i = 0; i < numGroups; i++) {
        CmapSequentialMapGroup group;
        group.startCharCode = u4(b);
        group.endCharCode   = u4(b + 4);
        group.startGlyphID  = u4(b + 8);
        groups_.push_back(group);
        b += 12;
        if (cb) {
            cb(cbUserdata, group);
        }
    }
    return kOk;
}

uint16_t CmapSubtableFormat12::Query(uint32_t code)
{
    size_t min = 0, max = groups_.size(), mid;
    while (min < max) {
        mid = (min + max) >> 1;
        const CmapSequentialMapGroup& group = groups_[mid];
        if (code < group.startCharCode) {
            max = mid;
        } else if (code > group.endCharCode) {
            min = mid + 1;
        } else {
            uint32_t offset = code - group.startCharCode;
            return group.startGlyphID + offset;
        }
    }
    return 0;
}

void CmapSubtableFormat12::Dump()
{
    printf("{\n");
    printf("  platformId=%d\n", (int)platformId_);
    printf("  encodingId=%d\n", (int)encodingId_);
    printf("  format=%d\n", 12);
    printf("  groups=[\n");
    for (size_t i = 0; i < groups_.size(); i++) {
        const CmapSequentialMapGroup& group = groups_[i];
        printf("    [%d]={%u,%u,%u}\n", (int)i, group.startCharCode, group.endCharCode, group.startGlyphID);
    }
    printf("  ]\n");
    printf("}\n");
}

std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat12(uint16_t platformId, uint16_t encodingId)
{
    return std::unique_ptr<CmapSubtable>(new CmapSubtableFormat12(platformId, encodingId));
}

//------------------------------------------------------------------------------
