#ifndef __PINYIN_FONT_OT_CMAP_H__
#define __PINYIN_FONT_OT_CMAP_H__

#include "status.h"
#include <cstdint>
#include <memory>

//------------------------------------------------------------------------------

typedef struct {
    uint32_t startCharCode;
    uint32_t endCharCode;
    uint32_t startGlyphID;
} CmapSequentialMapGroup;

typedef void (*CmapParseCallback)(void *userdata, CmapSequentialMapGroup group);

class CmapSubtable
{
protected:
    uint16_t format_;
    uint16_t platformId_;
    uint16_t encodingId_;

public:
    CmapSubtable(uint16_t format, uint16_t platformId, uint16_t encodingId)
     : format_(format), platformId_(platformId), encodingId_(encodingId) { }
    virtual ~CmapSubtable() {}

    virtual uint16_t Format() { return format_; }
    virtual uint16_t PlatformId() { return platformId_; }
    virtual uint16_t EncodingId() { return encodingId_; }

    virtual Status Parse(const uint8_t *start, const uint8_t *end, CmapParseCallback cb, void *cbUserdata) = 0;
    virtual uint16_t Query(uint32_t code) = 0;
    virtual void Dump() = 0;
};

std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat0(uint16_t platformId, uint16_t encodingId);
std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat4(uint16_t platformId, uint16_t encodingId);
std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat6(uint16_t platformId, uint16_t encodingId);
std::unique_ptr<CmapSubtable> CreateCmapSubtableFormat12(uint16_t platformId, uint16_t encodingId);

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_OT_CMAP_H__
