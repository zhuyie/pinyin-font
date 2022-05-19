#ifndef __PINYIN_FONT_OT_FONT_PARSER_H__
#define __PINYIN_FONT_OT_FONT_PARSER_H__

#include "ot_font.h"
#include <cstdio>
#include <cstdlib>

//------------------------------------------------------------------------------

class OpenType_Font_Parser
{
    typedef struct table {
        char name[4];
        uint32_t offset;
        uint32_t length;
        table() { 
            memset(name, 0, sizeof(name)); 
            offset = 0; 
            length = 0; 
        }
    } table;

    OpenType_Font *font_;
    uint8_t *data_;
    size_t len_;
    table head_;
    table maxp_;
    table post_;
    table os2_;
    table name_;
    table loca_;
    table glyf_;
    table hhea_;
    table hmtx_;
    table cmap_;

public:
    OpenType_Font_Parser();
    ~OpenType_Font_Parser();

    Status Parse(const char *filename, OpenType_Font *font);

private:
    Status __readWholeFile(FILE *f, size_t *pLen, uint8_t **ppData);
    Status __parseHead();
    Status __parseMaxp();
    Status __parsePost();
    Status __parseOS2();
    Status __parseName();
    Status __parseGlyph();
    Status __parseGlyphHeader(const uint8_t *data, size_t len, OpenType_GlyphHeader &header);
    Status __parseGlyphSimple(const uint8_t *data, size_t len, OpenType_GlyphSimple &simple);
    Status __parseGlyphComposite(const uint8_t *data, size_t len, OpenType_GlyphComposite &composite);
    Status __parseHmtx();
    Status __parseCmap();
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_OT_FONT_PARSER_H__
