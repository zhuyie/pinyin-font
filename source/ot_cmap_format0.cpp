#include "ot_cmap.h"
#include "utility.h"
#include <vector>

//------------------------------------------------------------------------------

class CmapSubtableFormat0 : public CmapSubtable
{
public:
    CmapSubtableFormat0(uint16_t platformId, uint16_t encodingId);
    virtual Status Parse(const uint8_t *start, const uint8_t *end);
    virtual uint16_t Query(uint32_t code);
    virtual void Dump();

private:
    std::vector<uint8_t> glyphIdArray_;
};

CmapSubtableFormat0::CmapSubtableFormat0(uint16_t platformId, uint16_t encodingId)
 : CmapSubtable(0, platformId, encodingId)
{
}

// https://docs.microsoft.com/en-us/typography/opentype/spec/cmap#format-0-byte-encoding-table
Status CmapSubtableFormat0::Parse(const uint8_t *start, const uint8_t *end)
{
    const uint8_t *b = start;
    if (b + 260 > end) {
        return kCorruption;
    }
    b += 4;
    glyphIdArray_.resize(256);
    for (int i = 0; i < 256; i++) {
        glyphIdArray_[i] = b[i];
    }
    return kOk;
}

uint16_t CmapSubtableFormat0::Query(uint32_t code)
{
    if (code < 256) {
        return glyphIdArray_[code];
    } else {
        return 0;
    }
}

void CmapSubtableFormat0::Dump()
{
    fprintf(stdout, "{\n");
    fprintf(stdout, "  platformId=%d\n", (int)platformId_);
    fprintf(stdout, "  encodingId=%d\n", (int)encodingId_);
    fprintf(stdout, "  format=%d\n", 0);
    fprintf(stdout, "  glyphIdArray=[\n");
    for (size_t start = 0, end = glyphIdArray_.size(); start < end;) {
        size_t lineEnd = start + 10;
        if (lineEnd > end) {
            lineEnd = end;
        }
        fprintf(stdout, "    ");
        while (start < lineEnd) {
            fprintf(stdout, "%3d ", (int)glyphIdArray_[start]);
            start++;
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "  ]\n");
    fprintf(stdout, "}\n");
}

std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat0(uint16_t platformId, uint16_t encodingId)
{
    return std::unique_ptr<CmapSubtable>(new CmapSubtableFormat0(platformId, encodingId));
}

//------------------------------------------------------------------------------
