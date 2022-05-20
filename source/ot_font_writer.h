#ifndef __PINYIN_FONT_OT_FONT_WRITER_H__
#define __PINYIN_FONT_OT_FONT_WRITER_H__

#include "ot_font.h"
#include <vector>

//------------------------------------------------------------------------------

class OpenType_Font_Writer
{
    const OpenType_Font *font_;
    std::vector<uint8_t> buf_;
    uint8_t *checksumAdjustment_;

public:
    OpenType_Font_Writer();
    ~OpenType_Font_Writer();

    Status Write(const char *filename, const OpenType_Font *font);

private:
    Status __writeFileHeader(uint16_t numTables);
    uint32_t __checksum(const uint8_t *table, uint32_t length);
    Status __writeTableHead(uint16_t tableIndex);
    Status __writeTableMaxp(uint16_t tableIndex);
    Status __writeTablePost(uint16_t tableIndex);
    Status __writeTableOS2(uint16_t tableIndex);
    Status __writeTableName(uint16_t tableIndex);
    Status __writeTableLocaGlyf(uint16_t tableIndex);
    Status __writeTableHhea(uint16_t tableIndex);
    Status __writeTableHmtx(uint16_t tableIndex);
    Status __writeTableCmap(uint16_t tableIndex);
    void __updateChecksumAdjustment(uint16_t numTables);
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_OT_FONT_WRITER_H__
