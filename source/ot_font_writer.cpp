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
    // https://docs.microsoft.com/en-us/typography/opentype/spec/post#version-20
    // - If the glyph name index is between 0 and 257 (inclusive), 
    //   treat that index as a glyph index in the Macintosh standard glyph set and use the Macintosh glyph name. 
    // - If the glyph name index is between 258 and 65535, then subtract 258 and use that to index 
    //   into the list of Pascal strings at the end of the table.
    // - If you do not want to associate a PostScript name with a particular glyph, use 0, 
    //   which refers to the name .notdef, as the glyphNameIndex entry for that glyph ID.

    size_t offset = buf_.size();
    uint32_t length = 32 + 2 + (uint32_t)font_->glyphNames_.size() * 2, lengthWithPadding;
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
    put_u2(b + 0, (uint16_t)font_->glyphNames_.size());
    b += 2;
    // glyphNameIndex[numGlyphs]
    uint16_t nameIndex = 258;
    for (size_t i = 0; i < font_->glyphNames_.size(); i++) {
        const std::string &str = font_->glyphNames_[i];
        if (str.length() > 0 && nameIndex < 65535) {
            put_u2(b + i * 2, nameIndex);
            nameIndex++;
            length += 1 + (uint8_t)str.length();
        } else {
            put_u2(b + i * 2, 0);
        }
    }
    // stringData
    lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);
    b = &(buf_[offset + 32 + 2 + font_->glyphNames_.size() * 2]);
    nameIndex = 258;
    for (size_t i = 0; i < font_->glyphNames_.size(); i++) {
        const std::string &str = font_->glyphNames_[i];
        if (str.length() == 0 || nameIndex == 65535) {
            continue;
        }
        uint8_t len = (uint8_t)str.length();
        b[0] = len;
        memcpy(b + 1, str.c_str(), len);
        b += 1 + len;
        nameIndex++;
    }

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "post", 4);
    b = &(buf_[offset]);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    return kOk;
}

Status OpenType_Font_Writer::__writeTableOS2(uint16_t tableIndex)
{
    size_t offset = buf_.size();
    buf_.resize(offset + 96);

    uint8_t *b = &(buf_[offset]);
    const OpenType_OS2 &os2 = font_->os2_;
    put_u2(b + 0,  4);  // Version 4
    put_i2(b + 2,  os2.xAvgCharWidth);
    put_u2(b + 4,  os2.usWeightClass);
    put_u2(b + 6,  os2.usWidthClass);
    put_u2(b + 8,  os2.fsType);
    put_i2(b + 10, os2.ySubscriptXSize);
    put_i2(b + 12, os2.ySubscriptYSize);
    put_i2(b + 14, os2.ySubscriptXOffset);
    put_i2(b + 16, os2.ySubscriptYOffset);
    put_i2(b + 18, os2.ySuperscriptXSize);
    put_i2(b + 20, os2.ySuperscriptYSize);
    put_i2(b + 22, os2.ySuperscriptXOffset);
    put_i2(b + 24, os2.ySuperscriptYOffset);
    put_i2(b + 26, os2.yStrikeoutSize);
    put_i2(b + 28, os2.yStrikeoutPosition);
    put_i2(b + 30, os2.sFamilyClass);
    memcpy(b + 32, os2.panose, 10);
    put_u4(b + 42, os2.ulUnicodeRange1);
    put_u4(b + 46, os2.ulUnicodeRange2);
    put_u4(b + 50, os2.ulUnicodeRange3);
    put_u4(b + 54, os2.ulUnicodeRange4);
    put_u4(b + 58, os2.achVendID);
    put_u2(b + 62, os2.fsSelection);
    put_u2(b + 64, os2.usFirstCharIndex);
    put_u2(b + 66, os2.usLastCharIndex);
    put_i2(b + 68, os2.sTypoAscender);
    put_i2(b + 70, os2.sTypoDescender);
    put_i2(b + 72, os2.sTypoLineGap);
    put_u2(b + 74, os2.usWinAscent);
    put_u2(b + 76, os2.usWinDescent);
    put_u4(b + 78, os2.ulCodePageRange1);
    put_u4(b + 82, os2.ulCodePageRange2);
    put_i2(b + 86, os2.sxHeight);
    put_i2(b + 88, os2.sCapHeight);
    put_u2(b + 90, os2.usDefaultChar);
    put_u2(b + 92, os2.usBreakChar);
    put_u2(b + 94, os2.usMaxContext);

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "OS/2", 4);
    put_u4(t + 4,  __checksum(b, 96));
    put_u4(t + 8,  offset);
    put_u4(t + 12, 96);

    return kOk;
}

Status OpenType_Font_Writer::__writeTableName(uint16_t tableIndex)
{
    // https://docs.microsoft.com/en-us/typography/opentype/spec/name
    // name records must be sorted first by platform ID, then by platform-specific ID, 
    // then by language ID, and then by name ID.
    std::vector<OpenType_NameRecord> records;
    records.reserve(font_->names_.size());
    for (auto iter = font_->names_.begin(); iter != font_->names_.end(); iter++) {
        records.push_back(iter->second);
    }
    std::sort(
        records.begin(), 
        records.end(), 
        [](const OpenType_NameRecord &a, const OpenType_NameRecord &b) -> bool {
            if (a.PlatformID != b.PlatformID)
                return a.PlatformID < b.PlatformID;
            if (a.EncodingID != b.EncodingID)
                return a.EncodingID < b.EncodingID;
            if (a.LanguageID != b.LanguageID)
                return a.LanguageID < b.LanguageID;
            return a.NameID < b.NameID;
        }
    );

    size_t offset = buf_.size();
    uint32_t length = 6 + (uint32_t)records.size() * 12, lengthWithPadding;
    buf_.resize(offset + length);
    uint8_t *b = &(buf_[offset]);

    put_u4(b + 0,  0);  // version 0
    put_u4(b + 2,  (uint16_t)records.size());
    put_i2(b + 4,  (uint16_t)length);
    b += 6;

    uint16_t stringOffset = 0;
    for (size_t i = 0; i < records.size(); i++) {
        const OpenType_NameRecord &record = records[i];
        put_u2(b +  0, record.PlatformID);
        put_u2(b +  2, record.EncodingID);
        put_u2(b +  4, record.LanguageID);
        put_u2(b +  6, record.NameID);
        put_u2(b +  8, (uint16_t)record.String.length() * 2);  // in bytes
        put_u2(b + 10, stringOffset);
        stringOffset += (uint16_t)record.String.length() * 2;
        b += 12;
    }

    length += stringOffset;
    lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);
    b = &(buf_[offset + 6 + records.size() * 12]);

    for (size_t i = 0; i < records.size(); i++) {
        const OpenType_NameRecord &record = records[i];
        for (size_t j = 0; j < record.String.length(); j++) {
            put_u2(b, (uint16_t)record.String[j]);  // UTF-16BE
            b += 2;
        }
    }

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "name", 4);
    b = &(buf_[offset]);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    return kOk;
}

Status OpenType_Font_Writer::__writeTableLocaGlyf(uint16_t tableIndex)
{
    // TODO
    return kOk;
}

Status OpenType_Font_Writer::__writeTableHhea(uint16_t tableIndex)
{
    size_t offset = buf_.size();
    buf_.resize(offset + 36);

    uint8_t *b = &(buf_[offset]);
    const OpenType_Hhea &hhea = font_->hhea_;
    put_u2(b + 0,  hhea.MajorVersion);
    put_u2(b + 2,  hhea.MinorVersion);
    put_i2(b + 4,  hhea.Ascender);
    put_i2(b + 6,  hhea.Descender);
    put_i2(b + 8,  hhea.LineGap);
    put_u2(b + 10, hhea.AdvanceWidthMax);
    put_u2(b + 12, hhea.MinLeftSideBearing);
    put_u2(b + 14, hhea.MinRightSideBearing);
    put_i2(b + 16, hhea.XMaxExtent);
    put_i2(b + 18, hhea.CaretSlopeRise);
    put_i2(b + 20, hhea.CaretSlopeRun);
    put_i2(b + 22, hhea.CaretOffset);
    put_i2(b + 24, hhea.Reserved0);
    put_i2(b + 26, hhea.Reserved1);
    put_i2(b + 28, hhea.Reserved2);
    put_i2(b + 30, hhea.Reserved3);
    put_i2(b + 32, hhea.MetricDataFormat);
    put_u2(b + 34, (uint16_t)font_->hmtx_.size());

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "hhea", 4);
    put_u4(t + 4,  __checksum(b, 36));
    put_u4(t + 8,  offset);
    put_u4(t + 12, 36);

    return kOk;
}

Status OpenType_Font_Writer::__writeTableHmtx(uint16_t tableIndex)
{
    size_t offset = buf_.size();
    uint32_t length = 4 * (uint32_t)font_->hmtx_.size();
    buf_.resize(offset + length);

    uint8_t *b = &(buf_[offset]);
    for (size_t i = 0; i < font_->hmtx_.size(); i++) {
        const OpenType_LongHorMetric &mtx = font_->hmtx_[i];
        put_u2(b + i * 4 + 0, mtx.AdvanceWidth);
        put_i2(b + i * 4 + 2, mtx.LSB);
    }

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "hmtx", 4);
    put_u4(t + 4,  __checksum(b, length));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

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
