#include "ot_cmap.h"
#include "utility.h"
#include <vector>

//------------------------------------------------------------------------------

class CmapSubtableFormat6 : public CmapSubtable
{
public:
    CmapSubtableFormat6(uint16_t platformId, uint16_t encodingId);
    virtual Status Parse(const uint8_t *start, const uint8_t *end, CmapParseCallback cb, void *cbUserdata);
    virtual uint16_t Query(uint32_t code);
    virtual void Dump();

private:
    uint16_t firstCode_;
    uint16_t entryCount_;
    std::vector<uint16_t> glyphIdArray_;
};

CmapSubtableFormat6::CmapSubtableFormat6(uint16_t platformId, uint16_t encodingId)
 : CmapSubtable(6, platformId, encodingId), firstCode_(0), entryCount_(0)
{
}

// https://docs.microsoft.com/en-us/typography/opentype/spec/cmap#format-6-trimmed-table-mapping
Status CmapSubtableFormat6::Parse(const uint8_t *start, const uint8_t *end, CmapParseCallback cb, void *cbUserdata)
{
    const uint8_t *b = start;
    if (b + 8 > end) {
        return kCorruption;
    }
    firstCode_ = u2(b + 4);
    entryCount_ = u2(b + 6);
    b += 8;
    if (b + entryCount_*2 > end) {
        return kCorruption;
    }
    glyphIdArray_.resize(entryCount_);
    for (uint16_t i = 0; i < entryCount_; i++) {
        uint16_t id = u2(b + i*2);
        glyphIdArray_[i] = id;
    }
    if (cb) {
        // TODO: needs to be optimized
        for (uint16_t i = 0; i < entryCount_; i++) {
            CmapSequentialMapGroup group;
            group.startCharCode = firstCode_ + i;
            group.endCharCode = firstCode_ + i;
            group.startGlyphID = glyphIdArray_[i];
            cb(cbUserdata, group);
        }
    }
    return kOk;
}

uint16_t CmapSubtableFormat6::Query(uint32_t code)
{
    if (code < firstCode_) {
        return 0;
    }
    uint32_t offset = code - firstCode_;
    if (offset >= entryCount_) {
        return 0;
    }
    return glyphIdArray_[offset];
}

void CmapSubtableFormat6::Dump()
{
    printf("{\n");
    printf("  platformId=%d\n", (int)platformId_);
    printf("  encodingId=%d\n", (int)encodingId_);
    printf("  format=%d\n", 6);
    printf("  firstCode=%d\n", (int)firstCode_);
    printf("  entryCount=%d\n", (int)entryCount_);
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

std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat6(uint16_t platformId, uint16_t encodingId)
{
    return std::unique_ptr<CmapSubtable>(new CmapSubtableFormat6(platformId, encodingId));
}

//------------------------------------------------------------------------------
