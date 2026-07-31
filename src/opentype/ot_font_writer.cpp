#include "ot_font_writer.h"
#include "ot_cmap_encoder.h"
#include "scope_guard.h"
#include "utility.h"
#include <cmath>
#include <cassert>
#include <algorithm>

//------------------------------------------------------------------------------

OpenType_Font_Writer::OpenType_Font_Writer()
: font_(NULL), checksumAdjustmentOffset_(0)
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
    numTables += __numOptionalTables();
    if ((status = __writeFileHeader(numTables)) != kOk)
        return status;
    // The records in the array must be sorted in ascending order by tag.
    uint16_t tableIndex = 0;
    if ((status = __writeTableGsub(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableOS2(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableCmap(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableCvt(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableFpgm(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableGlyf(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableHead(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableHhea(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableHmtx(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableLoca(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableMaxp(tableIndex)) != kOk)
        return status;
    if ((status = __writeTableName(tableIndex)) != kOk)
        return status;
    if ((status = __writeTablePost(tableIndex)) != kOk)
        return status;
    if ((status = __writeTablePrep(tableIndex)) != kOk)
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

uint32_t OpenType_Font_Writer::__checksum(const uint8_t *table, uint32_t length)
{
    assert (0 == (length & 3));
    const uint8_t *tableEnd = table + length;
    uint32_t sum = 0;
    while ((table + 4) <= tableEnd) {
        sum += u4(table);
        table += 4;
    }
    return sum;
}

Status OpenType_Font_Writer::__writeTableHead(uint16_t &tableIndex)
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

    checksumAdjustmentOffset_ = offset + 8;

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableMaxp(uint16_t &tableIndex)
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

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTablePost(uint16_t &tableIndex)
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
        const OpenType_GlyphName &entry = font_->glyphNames_[i];
        if (entry.ID < 258) {
            put_u2(b + i * 2, (uint16_t)entry.ID);
        } else if (entry.Str.length() > 0 && nameIndex < 65535) {
            put_u2(b + i * 2, nameIndex);
            nameIndex++;
            length += 1 + (uint8_t)entry.Str.length();
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
        const OpenType_GlyphName &entry = font_->glyphNames_[i];
        if (entry.ID < 258 || entry.Str.length() == 0) {
            continue;
        }
        uint8_t len = (uint8_t)entry.Str.length();
        b[0] = len;
        memcpy(b + 1, entry.Str.c_str(), len);
        b += 1 + len;
        if (nameIndex < 65535) {
            nameIndex++;
        } else {
            break;
        }
    }

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "post", 4);
    b = &(buf_[offset]);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableOS2(uint16_t &tableIndex)
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

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableName(uint16_t &tableIndex)
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

    put_u2(b + 0,  0);  // version 0
    put_u2(b + 2,  (uint16_t)records.size());
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

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableGlyf(uint16_t &tableIndex)
{
    Status status;
    size_t offset;
    uint32_t length, lengthWithPadding, glyphDataLen;
    uint8_t *ploca, *t, *b;

    // The `loca` table is an array of n offsets where n is the number of glyphs in the font plus one
    loca_.clear();
    loca_.resize(font_->glyphs_.size() + 1);

    // The offset of table `glyf`
    offset = buf_.size();

    // Write the glyphs one by one
    length = 0;
    for (size_t i = 0; i < font_->glyphs_.size(); i++) {
        ploca = (uint8_t*)&(loca_[i]);
        put_u4(ploca, length);  // uint32 in big endian

        const OpenType_GlyphHeader *header = font_->glyphs_[i];
        if (header == nullptr) {  // glyphs which have no outline
            continue;
        }
        if (header->NumberOfContours >= 0) {
            const OpenType_GlyphSimple *simple = (const OpenType_GlyphSimple*)header;
            status = __writeGlyphSimple(simple, &glyphDataLen);
            if (status != kOk) {
                return status;
            }
        } else {
            const OpenType_GlyphComposite *composite = (const OpenType_GlyphComposite*)header;
            status = __writeGlyphComposite(composite, &glyphDataLen);
            if (status != kOk) {
                return status;
            }
        }
        length += glyphDataLen;
    }
    ploca = (uint8_t*)&(loca_[font_->glyphs_.size()]);
    put_u4(ploca, length);  // uint32 in big endian

    lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);
    b = &(buf_[offset]);

    // Write the `glyf` table directory
    t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "glyf", 4);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeGlyphSimple(const OpenType_GlyphSimple *simple, uint32_t *glyphDataLen)
{
    *glyphDataLen = 0;

    // Calculate the length
    uint32_t length = 10;  // header
    length += (uint32_t)simple->EndPtsOfContours.size() * 2;  // endPtsOfContours
    length += 2;                                              // instructionLength
    length += (uint16_t)simple->Instructions.size();          // instructions
    uint8_t lastFlags = 0x00;
    for (size_t i = 0; i < simple->Points.size(); i++) {
        uint8_t flags = simple->Points[i].Flags;
        // flags
        if ((flags & OpenType_FlagRepeat) == 0) {
            length += 1;
        } else if (flags != lastFlags) {
            length += 2;
        }
        lastFlags = flags;
        // xCoordinates
        if (flags & OpenType_FlagXShortVector) {
            length += 1;
        } else if ((flags & OpenType_FlagXIsSame) == 0) {
            length += 2;
        }
        // yCoordinates
        if (flags & OpenType_FlagYShortVector) {
            length += 1;
        } else if ((flags & OpenType_FlagYIsSame) == 0) {
            length += 2;
        }
    }
    length = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries

    *glyphDataLen = length;

    size_t offset = buf_.size();
    buf_.resize(offset + length);
    uint8_t *b = &(buf_[offset]);

    // header
    put_i2(b + 0, simple->NumberOfContours);
    put_i2(b + 2, simple->XMin);
    put_i2(b + 4, simple->YMin);
    put_i2(b + 6, simple->XMax);
    put_i2(b + 8, simple->YMax);
    b += 10;
    // endPtsOfContours
    for (size_t i = 0; i < simple->EndPtsOfContours.size(); i++) {
        put_u2(b, simple->EndPtsOfContours[i]);
        b += 2;
    }
    // instructionLength
    put_u2(b, (uint16_t)simple->Instructions.size());
    b += 2;
    if (simple->Instructions.size() > 0) {
        // instructions
        memcpy(b, &(simple->Instructions[0]), simple->Instructions.size());
        b += simple->Instructions.size();
    }
    if (simple->NumberOfContours == 0) {
        return kOk;
    }
    // flags
    lastFlags = 0x00;
    for (size_t i = 0; i < simple->Points.size(); i++) {
        uint8_t flags = simple->Points[i].Flags;
        if ((flags & OpenType_FlagRepeat) == 0) {
            b[0] = flags;
            b += 1;
        } else if (flags != lastFlags) {
            b[0] = flags;
            b[1] = 0;  // number of additional entries
            b += 2;
        } else {
            b[-1]++;
        }
        lastFlags = flags;
    }
    // xCoordinates
    int16_t x = 0;
    for (size_t i = 0; i < simple->Points.size(); i++) {
        const OpenType_GlyphPoint &p = simple->Points[i];
        uint8_t flags = p.Flags;
        int16_t dx = p.X - x;
        x = p.X;
        if (flags & OpenType_FlagXShortVector) {
            if ((flags & OpenType_FlagPositiveXShortVector) == 0) {
                dx = -dx;
            }
            put_u1(b, (uint8_t)dx);
            b += 1;
        } else if ((flags & OpenType_FlagXIsSame) == 0) {
            put_i2(b, dx);
            b += 2;
        }
    }
    // yCoordinates
    int16_t y = 0;
    for (size_t i = 0; i < simple->Points.size(); i++) {
        const OpenType_GlyphPoint &p = simple->Points[i];
        uint8_t flags = p.Flags;
        int16_t dy = p.Y - y;
        y = p.Y;
        if (flags & OpenType_FlagYShortVector) {
            if ((flags & OpenType_FlagPositiveYShortVector) == 0) {
                dy = -dy;
            }
            put_u1(b, (uint8_t)dy);
            b += 1;
        } else if ((flags & OpenType_FlagYIsSame) == 0) {
            put_i2(b, dy);
            b += 2;
        }
    }

    return kOk;
}

Status OpenType_Font_Writer::__writeGlyphComposite(const OpenType_GlyphComposite *composite, uint32_t *glyphDataLen)
{
    *glyphDataLen = 0;

    // Calculate the length
    uint32_t length = 10;  // header
    uint16_t flags = 0;
    for (size_t i = 0; i < composite->SubGlyphs.size(); i++) {
        flags = composite->SubGlyphs[i].Flags;
        // flags && glyphIndex
        length += 4;
        // argument1 && argument2
        if (flags & OpenType_FlagArg1And2AreWords) {
            length += 4;
        } else {
            length += 2;
        }
        // transformation option
        if (flags & OpenType_FlagWeHaveAScale) {
            length += 2;
        } else if (flags & OpenType_FlagWeHaveAnXAndYScale) {
            length += 4;
        } else if (flags & OpenType_FlagWeHaveATwoByTwo) {
            length += 8;
        }
    }
    if (composite->Instructions.size()) {
        assert(flags & OpenType_FlagWeHaveInstructions);
        // instructionLength && instructions
        length += 2;
        length += (uint16_t)composite->Instructions.size();
    }
    length = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries

    *glyphDataLen = length;

    size_t offset = buf_.size();
    buf_.resize(offset + length);
    uint8_t *b = &(buf_[offset]);

    // header
    put_i2(b + 0, composite->NumberOfContours);
    put_i2(b + 2, composite->XMin);
    put_i2(b + 4, composite->YMin);
    put_i2(b + 6, composite->XMax);
    put_i2(b + 8, composite->YMax);
    b += 10;
    // components
    for (size_t i = 0; i < composite->SubGlyphs.size(); i++) {
        const OpenType_GlyphComponent &component = composite->SubGlyphs[i];
        // flags && glyphIndex
        put_u2(b + 0, component.Flags);
        put_u2(b + 2, component.GlyphIndex);
        b += 4;
        // argument1 && argument2
        if (component.Flags & OpenType_FlagArg1And2AreWords) {
            put_i2(b + 0, component.Arg1);
            put_i2(b + 2, component.Arg2);
            b += 4;
        } else {
            put_u1(b + 0, (uint8_t)component.Arg1);
            put_u1(b + 1, (uint8_t)component.Arg2);
            b += 2;
        }
        // transformation option
        if (component.Flags & OpenType_FlagWeHaveAScale) {
            put_i2(b + 0, component.Transform[0]);
            b += 2;
        } else if (component.Flags & OpenType_FlagWeHaveAnXAndYScale) {
            put_i2(b + 0, component.Transform[0]);
            put_i2(b + 2, component.Transform[3]);
            b += 4;
        } else if (component.Flags & OpenType_FlagWeHaveATwoByTwo) {
            put_i2(b + 0, component.Transform[0]);
            put_i2(b + 2, component.Transform[1]);
            put_i2(b + 4, component.Transform[2]);
            put_i2(b + 6, component.Transform[3]);
            b += 8;
        }
    }
    if (composite->Instructions.size()) {
        // instructionLength
        put_u2(b, (uint16_t)composite->Instructions.size());
        b += 2;
        // instructions
        memcpy(b, &(composite->Instructions[0]), composite->Instructions.size());
        b += composite->Instructions.size();
    }

    return kOk;
}

Status OpenType_Font_Writer::__writeTableLoca(uint16_t &tableIndex)
{
    size_t offset;
    uint32_t length;
    uint8_t *b, *t;

    offset = buf_.size();
    length = loca_.size() * 4;
    buf_.resize(offset + length);
    b = &(buf_[offset]);
    memcpy(b, &loca_[0], length);

    t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "loca", 4);
    put_u4(t + 4, __checksum(b, length));
    put_u4(t + 8, offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableHhea(uint16_t &tableIndex)
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

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableHmtx(uint16_t &tableIndex)
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

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableCmap(uint16_t &tableIndex)
{
    std::vector<uint8_t> format4;
    Status status = EncodeCmapFormat4(font_->char2index_, format4);
    if (status != kOk) {
        return status;
    }
    uint32_t format4Len = (uint32_t)format4.size();
    uint32_t format12Len = 16 + (uint32_t)font_->char2index_.size() * 12;
    uint32_t length = 20 + format4Len + format12Len;
    uint32_t paddedLength = (length + 3) & ~uint32_t(3);

    size_t offset = buf_.size();
    buf_.resize(offset + paddedLength, 0);
    uint8_t *b = &(buf_[offset]);
    uint8_t *bstart = b;

    put_u2(b + 0,  0);  // version = 0
    put_u2(b + 2,  2);  // numTables = 2
    // EncodingRecord: 3,1
    put_u2(b + 4,  3);
    put_u2(b + 6,  1);
    put_u4(b + 8,  20);
    // EncodingRecord: 3,10
    put_u2(b + 12, 3);
    put_u2(b + 14, 10);
    put_u4(b + 16, 20 + format4Len);
    b += 20;

    memcpy(b, &format4[0], format4Len);
    b += format4Len;

    put_u2(b + 0,  12);           // format 12
    put_u2(b + 2,  0);            // reserved
    put_u4(b + 4,  format12Len);  // subtable length
    put_u4(b + 8,  0);            // language
    put_u4(b + 12, (uint32_t)font_->char2index_.size());   // numGroups
    b += 16;
    for (size_t i = 0; i < font_->char2index_.size(); i++) {
        const CmapSequentialMapGroup &group = font_->char2index_[i];
        put_u4(b + 0, group.startCharCode);
        put_u4(b + 4, group.endCharCode);
        put_u4(b + 8, group.startGlyphID);
        b += 12;
    }

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "cmap", 4);
    put_u4(t + 4,  __checksum(bstart, paddedLength));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

void OpenType_Font_Writer::__updateChecksumAdjustment(uint16_t numTables)
{
    assert(checksumAdjustmentOffset_ != 0);

    uint32_t sum = 0;
    const uint8_t *b = &(buf_[0]);
    uint32_t length = 12 + 16 * numTables;
    sum = __checksum(b, length);  // checksum of table directory
    b += 12;
    for (uint16_t i = 0; i < numTables; i++) {
        sum += u4(b + 4);  // checksum of this table
        b += 16;
    }

    uint8_t* checksumAdjustment = &buf_[0] + checksumAdjustmentOffset_;
    put_u4(checksumAdjustment, 0xB1B0AFBAu - sum);
}

uint16_t OpenType_Font_Writer::__numOptionalTables()
{
    uint16_t n = 0;
    if (font_->ligatureSubstitutions_.size() > 0)
        n++;
    if (font_->cvt_.size() > 0)
        n++;
    if (font_->fpgm_.size() > 0)
        n++;
    if (font_->prep_.size() > 0)
        n++;
    return n;
}

Status OpenType_Font_Writer::__writeTableCvt(uint16_t &tableIndex)
{
    if (font_->cvt_.size() == 0) {
        return kOk;
    }

    size_t offset = buf_.size();
    uint32_t length = (uint32_t)font_->cvt_.size();
    uint32_t lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);

    uint8_t *b = &(buf_[offset]);
    const uint8_t *src = &(font_->cvt_[0]);
    memcpy(b, src, length);

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "cvt ", 4);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableFpgm(uint16_t &tableIndex)
{
    if (font_->fpgm_.size() == 0) {
        return kOk;
    }

    size_t offset = buf_.size();
    uint32_t length = (uint32_t)font_->fpgm_.size();
    uint32_t lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);

    uint8_t *b = &(buf_[offset]);
    const uint8_t *src = &(font_->fpgm_[0]);
    memcpy(b, src, length);

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "fpgm", 4);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTablePrep(uint16_t &tableIndex)
{
    if (font_->prep_.size() == 0) {
        return kOk;
    }

    size_t offset = buf_.size();
    uint32_t length = (uint32_t)font_->prep_.size();
    uint32_t lengthWithPadding = ((length - 1) / 4 + 1) * 4;  // padding to 4-byte boundaries
    buf_.resize(offset + lengthWithPadding);

    uint8_t *b = &(buf_[offset]);
    const uint8_t *src = &(font_->prep_[0]);
    memcpy(b, src, length);

    uint8_t *t = &(buf_[12 + tableIndex * 16]);
    memcpy(t, "prep", 4);
    put_u4(t + 4,  __checksum(b, lengthWithPadding));
    put_u4(t + 8,  offset);
    put_u4(t + 12, length);

    tableIndex++;
    return kOk;
}

Status OpenType_Font_Writer::__writeTableGsub(uint16_t &tableIndex)
{
    if (font_->ligatureSubstitutions_.empty()) return kOk;

    typedef std::vector<OpenType_LigatureSubstitution> RuleList;
    std::map<uint16_t, RuleList> grouped;
    for (size_t i = 0; i < font_->ligatureSubstitutions_.size(); i++) {
        const OpenType_LigatureSubstitution &rule =
            font_->ligatureSubstitutions_[i];
        if (rule.Components.size() < 2 ||
            rule.LigatureGlyph >= font_->glyphs_.size()) {
            return kError;
        }
        grouped[rule.Components[0]].push_back(rule);
    }

    std::vector<uint8_t> table;
    auto appendU2 = [&table](uint16_t value) {
        size_t p = table.size();
        table.resize(p + 2);
        put_u2(&table[p], value);
    };
    auto appendU4 = [&table](uint32_t value) {
        size_t p = table.size();
        table.resize(p + 4);
        put_u4(&table[p], value);
    };
    auto patchU2 = [&table](size_t p, size_t value) -> bool {
        if (value > 0xFFFF || p + 2 > table.size()) return false;
        put_u2(&table[p], (uint16_t)value);
        return true;
    };

    appendU4(0x00010000);
    appendU2(10);
    appendU2(0);
    appendU2(0);

    // ScriptList: DFLT and hani share one Script table and default LangSys.
    size_t scriptList = table.size();
    appendU2(2);
    table.insert(table.end(), {'D','F','L','T'});
    appendU2(0);
    table.insert(table.end(), {'h','a','n','i'});
    appendU2(0);
    size_t scriptTable = table.size();
    appendU2(4);
    appendU2(0);
    appendU2(0);
    appendU2(0xFFFF);
    appendU2(1);
    appendU2(0);
    if (!patchU2(scriptList + 6, scriptTable - scriptList) ||
        !patchU2(scriptList + 12, scriptTable - scriptList)) return kError;

    size_t featureList = table.size();
    if (!patchU2(6, featureList)) return kError;
    appendU2(1);
    table.insert(table.end(), {'l','i','g','a'});
    appendU2(8);
    appendU2(0);
    appendU2(1);
    appendU2(0);

    size_t lookupList = table.size();
    if (!patchU2(8, lookupList)) return kError;
    appendU2(1);
    appendU2(4);
    size_t lookupTable = table.size();
    appendU2(4);
    appendU2(0);
    appendU2(1);
    appendU2(8);
    size_t subtable = table.size();
    appendU2(1);
    appendU2(0);
    appendU2((uint16_t)grouped.size());
    std::vector<size_t> setOffsetPositions;
    for (size_t i = 0; i < grouped.size(); i++) {
        setOffsetPositions.push_back(table.size());
        appendU2(0);
    }

    size_t setIndex = 0;
    for (std::map<uint16_t, RuleList>::const_iterator group = grouped.begin();
         group != grouped.end(); ++group, ++setIndex) {
        size_t setStart = table.size();
        if (!patchU2(setOffsetPositions[setIndex], setStart - subtable))
            return kError;
        appendU2((uint16_t)group->second.size());
        std::vector<size_t> ligOffsetPositions;
        for (size_t i = 0; i < group->second.size(); i++) {
            ligOffsetPositions.push_back(table.size());
            appendU2(0);
        }
        for (size_t i = 0; i < group->second.size(); i++) {
            const OpenType_LigatureSubstitution &rule = group->second[i];
            size_t ligStart = table.size();
            if (!patchU2(ligOffsetPositions[i], ligStart - setStart))
                return kError;
            appendU2(rule.LigatureGlyph);
            appendU2((uint16_t)rule.Components.size());
            for (size_t j = 1; j < rule.Components.size(); j++)
                appendU2(rule.Components[j]);
        }
    }

    size_t coverage = table.size();
    if (!patchU2(subtable + 2, coverage - subtable)) return kError;
    appendU2(1);
    appendU2((uint16_t)grouped.size());
    for (std::map<uint16_t, RuleList>::const_iterator group = grouped.begin();
         group != grouped.end(); ++group) {
        appendU2(group->first);
    }

    if (table.size() > 0xFFFF) return kError;
    size_t offset = buf_.size();
    uint32_t length = (uint32_t)table.size();
    uint32_t paddedLength = (length + 3u) & ~3u;
    buf_.resize(offset + paddedLength, 0);
    memcpy(&buf_[offset], &table[0], length);

    uint8_t *record = &buf_[12 + tableIndex * 16];
    memcpy(record, "GSUB", 4);
    put_u4(record + 4, __checksum(&buf_[offset], paddedLength));
    put_u4(record + 8, (uint32_t)offset);
    put_u4(record + 12, length);
    tableIndex++;
    return kOk;
}
