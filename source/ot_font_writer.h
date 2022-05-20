#ifndef __PINYIN_FONT_OT_FONT_WRITER_H__
#define __PINYIN_FONT_OT_FONT_WRITER_H__

#include "ot_font.h"

//------------------------------------------------------------------------------

class OpenType_Font_Writer
{
    const OpenType_Font *font_;

public:
    OpenType_Font_Writer();
    ~OpenType_Font_Writer();

    Status Write(const char *filename, const OpenType_Font *font);

private:
    Status __writeFileHeader(FILE *f, uint16_t numTables);
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_OT_FONT_WRITER_H__
