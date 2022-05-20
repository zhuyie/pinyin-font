#include "ot_font_writer.h"
#include "scope_guard.h"
#include "utility.h"
#include <cmath>
#include <cassert>

//------------------------------------------------------------------------------

OpenType_Font_Writer::OpenType_Font_Writer()
: font_(NULL), checksumAdjustment_(NULL)
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

    uint16_t glyphCount = font_->GlyphCount();
    if (glyphCount < 1000) {
        buf_.reserve(128 * 1024);
    } else if (glyphCount < 2500) {
        buf_.reserve(256 * 1024);
    } else if (glyphCount < 5000) {
        buf_.reserve(1 * 1024 * 1024);
    } else if (glyphCount < 10000) {
        buf_.reserve(2 * 1024 * 1024);
    } else {
        buf_.reserve(4 * 1024 * 1024);
    }

    uint16_t numTables = 10;
    if ((status = __writeFileHeader(numTables)) != kOk)
        return status;
    if ((status = __writeTableHead(0)) != kOk)
        return status;
    if ((status = __writeTableMaxp(1)) != kOk)
        return status;
    if ((status = __writeTablePost(2)) != kOk)
        return status;
    if ((status = __writeTableOS2(3)) != kOk)
        return status;
    if ((status = __writeTableName(4)) != kOk)
        return status;
    if ((status = __writeTableLocaGlyf(5)) != kOk)
        return status;
    if ((status = __writeTableHhea(7)) != kOk)
        return status;
    if ((status = __writeTableHmtx(8)) != kOk)
        return status;
    if ((status = __writeTableCmap(9)) != kOk)
        return status;
    __updateChecksumAdjustment(numTables);

    if (fwrite(&(buf_[0]), 1, buf_.size(), f) != buf_.size()) {
        return kFileError;
    }
    if (fflush(f) != 0) {
        return kFileError;
    }

    return kOk;
}

Status OpenType_Font_Writer::__writeFileHeader(uint16_t numTables)
{
    buf_.resize(12 + 16 * numTables);
    uint8_t *b = &(buf_[0]);

    // https://docs.microsoft.com/en-us/typography/opentype/spec/otff#table-directory
    uint32_t sfntVersion = 0x00010000;
    uint16_t entrySelector = (uint16_t)(std::floor(std::log2(numTables)));
    uint16_t searchRange = (uint16_t)(std::exp2(entrySelector) * 16);
    uint16_t rangeShift = numTables * 16 - searchRange;

    put_u4(b + 0,  sfntVersion);
    put_u2(b + 4,  numTables);
    put_u2(b + 6,  searchRange);
    put_u2(b + 8,  entrySelector);
    put_u2(b + 10, rangeShift);

    return kOk;
}

uint32_t OpenType_Font_Writer::__checksum(const uint8_t *buf, uint32_t length)
{
    assert (0 == (length & 3));
    const uint32_t *table = (const uint32_t*)buf;
    const uint32_t *tableEnd = table + length / 4;
    uint32_t sum = 0;
    while (table < tableEnd) {
        sum += *table;
        table++;
    }
    return sum;
}

Status OpenType_Font_Writer::__writeTableHead(uint16_t tableIndex)
{
    size_t offset = buf_.size();
    buf_.resize(offset + 56);  // 54 + 2

    uint8_t *b = &(buf_[offset]);
    const OpenType_Head &head = font_->head_;
    put_u4(b + 0,  head.Version);
    put_u4(b + 4,  head.FontRevision);
    put_u4(b + 8,  0);
    put_u4(b + 12, head.MagicNumber);
    put_u2(b + 16, head.Flags);
    put_u2(b + 18, head.UnitsPerEm);
    put_u8(b + 20, head.Created);
    put_u8(b + 28, head.Modified);
    put_i2(b + 36, head.XMin);
    put_i2(b + 38, head.YMin);
    put_i2(b + 40, head.XMax);
    put_i2(b + 42, head.YMax);
    put_u2(b + 44, head.MacStyle);
    put_u2(b + 46, head.LowestRecPPEM);
    put_i2(b + 48, head.FontDirectionHint);
    put_i2(b + 50, 1);  // Always use long format
    put_i2(b + 52, head.GlyphDataFormat);

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "head", 4);
    put_u4(t + 4,  __checksum(b, 56));
    put_u4(t + 8,  offset);
    put_u4(t + 12, 54);

    checksumAdjustment_ = b + 8;

    return kOk;
}

Status OpenType_Font_Writer::__writeTableMaxp(uint16_t tableIndex)
{
    // https://docs.microsoft.com/en-us/typography/opentype/spec/maxp
    // Fonts with TrueType outlines must use Version 1.0 of this table

    size_t offset = buf_.size();
    buf_.resize(offset + 32);

    uint8_t *b = &(buf_[offset]);
    const OpenType_Maxp &maxp = font_->maxp_;
    put_u4(b + 0,  0x00010000);
    put_u2(b + 4,  maxp.NumGlyphs);
    put_u2(b + 6,  maxp.MaxPoints);
    put_u2(b + 8,  maxp.MaxContours);
    put_u2(b + 10, maxp.MaxCompositePoints);
    put_u2(b + 12, maxp.MaxCompositeContours);
    put_u2(b + 14, maxp.MaxZones);
    put_u2(b + 16, maxp.MaxTwilightPoints);
    put_u2(b + 18, maxp.MaxStorage);
    put_u2(b + 20, maxp.MaxFunctionDefs);
    put_u2(b + 22, maxp.MaxInstructionDefs);
    put_u2(b + 24, maxp.MaxStackElements);
    put_u2(b + 26, maxp.MaxSizeOfInstructions);
    put_u2(b + 28, maxp.MaxComponentElements);
    put_u2(b + 30, maxp.MaxComponentDepth);

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "maxp", 4);
    put_u4(t + 4,  __checksum(b, 32));
    put_u4(t + 8,  offset);
    put_u4(t + 12, 32);

    return kOk;
}

Status OpenType_Font_Writer::__writeTablePost(uint16_t tableIndex)
{
    size_t offset = buf_.size();
    uint32_t length = 32 + 2 + font_->maxp_.NumGlyphs * 2, lengthWithPadding;
    buf_.resize(offset + length);
    uint8_t *b = &(buf_[offset]);

    const OpenType_Post &post = font_->post_;
    put_u4(b + 0,  0x00020000);  // version 2.0
    put_u4(b + 4,  post.ItalicAngle);
    put_i2(b + 8,  post.UnderlinePosition);
    put_i2(b + 10, post.UnderlineThickness);
    put_u4(b + 12, post.IsFixedPitch);
    put_u4(b + 16, post.MinMemType42);
    put_u4(b + 20, post.MaxMemType42);
    put_u4(b + 24, post.MinMemType1);
    put_u4(b + 28, post.MaxMemType1);
    b += 32;
    // numGlyphs
    put_u2(b + 0, font_->maxp_.NumGlyphs);
    b += 2;
    // glyphNameIndex[numGlyphs]
    for (uint16_t i = 0; i < font_->maxp_.NumGlyphs; i++) {
        put_u2(b + i * 2, 258 + i);
        length += 1 + (uint8_t)font_->glyphNames_[i].length();
    }
    // stringData
    lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);
    b = &(buf_[offset + 32 + 2 + font_->maxp_.NumGlyphs * 2]);
    for (uint16_t i = 0; i < font_->maxp_.NumGlyphs; i++) {
        const std::string &str = font_->glyphNames_[i];
        uint8_t len = (uint8_t)str.length();
        b[0] = len;
        memcpy(b + 1, str.c_str(), len);
        b += 1 + len;
    }

    b = &(buf_[offset]);
    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "post", 4);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    return kOk;
}

Status OpenType_Font_Writer::__writeTableOS2(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

Status OpenType_Font_Writer::__writeTableName(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

Status OpenType_Font_Writer::__writeTableLocaGlyf(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

Status OpenType_Font_Writer::__writeTableHhea(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

Status OpenType_Font_Writer::__writeTableHmtx(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

Status OpenType_Font_Writer::__writeTableCmap(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

void OpenType_Font_Writer::__updateChecksumAdjustment(uint16_t numTables)
{
    assert(checksumAdjustment_ != NULL);

    uint32_t sum = 0;
    const uint8_t *b = &(buf_[0]);
    uint32_t length = 12 + 16 * numTables;
    sum = __checksum(b, length);  // checksum of table directory
    b += length;
    for (uint16_t i = 0; i < numTables; i++) {
        sum += u4(b + 4);  // checksum of this table
        b += 16;
    }
    *checksumAdjustment_ = sum - 0xB1B0AFBA;
}
