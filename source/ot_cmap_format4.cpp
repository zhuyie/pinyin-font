#include "ot_cmap.h"
#include "utility.h"
#include <vector>

//------------------------------------------------------------------------------

class CmapSubtableFormat4 : public CmapSubtable
{
    typedef struct {
        uint16_t startCode;
        uint16_t endCode;
        int16_t idDelta;
        uint16_t idRangeOffset;
    } segment;

    std::vector<segment> segments_;
    std::vector<uint16_t> glyphIdArray_;

public:
    CmapSubtableFormat4(uint16_t platformId, uint16_t encodingId);
    virtual Status Parse(const uint8_t *start, const uint8_t *end);
    virtual uint16_t Query(uint32_t code);
    virtual void Dump();
};

CmapSubtableFormat4::CmapSubtableFormat4(uint16_t platformId, uint16_t encodingId)
 : CmapSubtable(4, platformId, encodingId)
{
}

// https://docs.microsoft.com/en-us/typography/opentype/spec/cmap#format-4-segment-mapping-to-delta-values
Status CmapSubtableFormat4::Parse(const uint8_t *start, const uint8_t *end)
{
    const uint8_t *b = start;
    if (b + 12 > end) {
        return kCorruption;
    }
    uint16_t length = u2(b);
    const uint8_t *subtableEnd = start - 2 + length;  // 2B for `format`
    if (subtableEnd > end) {
        return kCorruption;
    }
    uint16_t segCountX2 = u2(b + 4);
    uint16_t segCount = segCountX2 / 2;
    b += 12;

    segments_.resize(segCount);

    if (b + 8*segCount + 2 > subtableEnd) {
        return kCorruption;
    }
    for (uint16_t i = 0; i < segCount; i++) {
        segments_[i].endCode = u2(b + 2*i);
    }
    b += 2*segCount + 2;
    for (uint16_t i = 0; i < segCount; i++) {
        segments_[i].startCode = u2(b + 2*i);
    }
    b += 2*segCount;
    for (uint16_t i = 0; i < segCount; i++) {
        segments_[i].idDelta = i2(b + 2*i);
    }
    b += 2*segCount;
    for (uint16_t i = 0; i < segCount; i++) {
        segments_[i].idRangeOffset = u2(b + 2*i);
    }
    b += 2*segCount;

    while (b + 2 <= subtableEnd) {
        uint16_t glyphId = u2(b);
        glyphIdArray_.push_back(glyphId);
        b += 2;
    }

    return kOk;
}

uint16_t CmapSubtableFormat4::Query(uint32_t code)
{
    size_t min = 0, max = segments_.size(), mid;
    while (min < max) {
        mid = (min + max) >> 1;
        const segment& seg = segments_[mid];
        if (code < seg.startCode) {
            max = mid;
        } else if (code > seg.endCode) {
            min = mid + 1;
        } else {
            int index;
            if (seg.idRangeOffset == 0) {
                index = (int)code + (int)seg.idDelta;
            } else {
                int offset = (int)(seg.idRangeOffset / 2) + (int)(code - seg.startCode) - (int)(segments_.size() - mid);
                if (offset < 0 || offset >= (int)glyphIdArray_.size()) {
                    return 0;
                }
                index = glyphIdArray_[offset];
                if (index != 0) {
                    index += (int)seg.idDelta;
                }
            }
            index = index % 65536;
            return (uint16_t)index;
        }
    }
    return 0;
}

void CmapSubtableFormat4::Dump()
{
    printf("{\n");
    printf("  platformId=%d\n", (int)platformId_);
    printf("  encodingId=%d\n", (int)encodingId_);
    printf("  format=%d\n", 4);
    printf("  segments=[\n");
    for (size_t i = 0; i < segments_.size(); i++) {
        const segment& seg = segments_[i];
        printf("    [%d]={%d,%d,%d,%d}\n", (int)i, (int)seg.startCode, (int)seg.endCode, (int)seg.idDelta, (int)seg.idRangeOffset);
    }
    printf("  ]\n");
    printf("  glyphIdArray=[\n");
    for (size_t start = 0, end = glyphIdArray_.size(); start < end;) {
        size_t lineEnd = start + 10;
        if (lineEnd > end) {
            lineEnd = end;
        }
        printf("    ");
        while (start < lineEnd) {
            printf("%5d ", (int)glyphIdArray_[start]);
            start++;
        }
        printf("\n");
    }
    printf("  ]\n");
    printf("}\n");
}

std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat4(uint16_t platformId, uint16_t encodingId)
{
    return std::unique_ptr<CmapSubtable>(new CmapSubtableFormat4(platformId, encodingId));
}

//------------------------------------------------------------------------------
