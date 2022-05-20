#include "ot_font_writer.h"
#include "scope_guard.h"
#include "utility.h"
#include <cmath>
#include <cassert>

//------------------------------------------------------------------------------

OpenType_Font_Writer::OpenType_Font_Writer()
: font_(NULL)
{
}

OpenType_Font_Writer::~OpenType_Font_Writer()
{
}

Status OpenType_Font_Writer::Write(const char *filename, const OpenType_Font *font)
{
    assert(font != NULL);
    assert(font_ == NULL);

    Status status;

    font_ = font;

    FILE *f = fopen(filename, "wb");
    if (f == NULL)
        return kFileError;
    auto f_guard = scopeGuard([&f]{ fclose(f); });

    if ((status = __writeFileHeader(f, 10)) != kOk)
        return status;

    if (fflush(f) != 0) {
        return kFileError;
    }

    return kOk;
}

Status OpenType_Font_Writer::__writeFileHeader(FILE *f, uint16_t numTables)
{
    // https://docs.microsoft.com/en-us/typography/opentype/spec/otff#table-directory
    uint8_t buf[12 + 16] = { 0 };
    uint32_t sfntVersion = 0x00010000;
    uint16_t entrySelector = (uint16_t)(std::floor(std::log2(numTables)));
    uint16_t searchRange = (uint16_t)(std::exp2(entrySelector) * 16);
    uint16_t rangeShift = numTables * 16 - searchRange;

    put_u4(buf + 0,  sfntVersion);
    put_u2(buf + 4,  numTables);
    put_u2(buf + 6,  searchRange);
    put_u2(buf + 8,  entrySelector);
    put_u2(buf + 10, rangeShift);
    if (fwrite(buf, 1, 12, f) != 12) {
        return kFileError;
    }

    for (uint16_t i = 0; i < numTables; i++) {
        if (fwrite(buf + 12, 1, 16, f) != 16) {
            return kFileError;
        }
    }

    return kOk;
}
