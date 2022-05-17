#include "ot_font_parser.h"
#include "scope_guard.h"
#include "utility.h"
#include "mac_glyph_names.h"
#include <cassert>

//------------------------------------------------------------------------------

OpenType_Font_Parser::OpenType_Font_Parser()
: font_(NULL), data_(NULL), len_(0)
{
}

OpenType_Font_Parser::~OpenType_Font_Parser()
{
    free(data_);
}

Status OpenType_Font_Parser::Parse(const char *filename, OpenType_Font *font)
{
    assert(font != NULL);
    assert(font_ == NULL);

    Status status;

    font_ = font;

    FILE *f = fopen(filename, "rb");
    if (f == NULL)
        return kNotFound;
    auto f_guard = scopeGuard([&f]{ fclose(f); });

    assert(data_ == NULL);
    assert(len_ == 0);
    status = __readWholeFile(f, &len_, &data_);
    if (status != kOk)
        return status;

    if (len_ < 12) {
        return kCorruption;
    }

    size_t offset = 0;
    uint32_t magic = u4(data_ + offset);
    offset += 4;
    switch (magic) {
    case 0x00010000:
        break;
    case 0x74727565: // 'true'
        break;
    case 0x4F54544F: // 'OTTO'
        return kNotSupported;
    default:
        return kCorruption;
    }

    int numTables = (int)u2(data_ + offset);
    offset += 2;
    offset += 6; // Skip the searchRange, entrySelector and rangeShift.
    if (numTables <= 0 || len_ < offset + 16*numTables) {
        return kCorruption;
    }

    for (int i = 0; i < numTables; i++) {
        size_t x = offset + i*16; // the offset of the current table start
        table t;
        memcpy(t.name, data_ + x, 4);
        t.offset = u4(data_ + x + 8);
        t.length = u4(data_ + x + 12);
        if (t.offset + t.length > len_) {
            return kCorruption;
        }

        if (memcmp(t.name, "head", 4) == 0) {
            head_ = t;
        } else if (memcmp(t.name, "maxp", 4) == 0) {
            maxp_ = t;
        } else if (memcmp(t.name, "post", 4) == 0) {
            post_ = t;
        } else if (memcmp(t.name, "OS/2", 4) == 0) {
            os2_ = t;
        } else if (memcmp(t.name, "name", 4) == 0) {
            name_ = t;
        } else if (memcmp(t.name, "loca", 4) == 0) {
            loca_ = t;
        } else if (memcmp(t.name, "glyf", 4) == 0) {
            glyf_ = t;
        } else if (memcmp(t.name, "hhea", 4) == 0) {
            hhea_ = t;
        } else if (memcmp(t.name, "hmtx", 4) == 0) {
            hmtx_ = t;
        } else if (memcmp(t.name, "cmap", 4) == 0) {
            cmap_ = t;
        }
    }

    if ((status = __parseHead()) != kOk)
        return status;
    if ((status = __parseMaxp()) != kOk)
        return status;
    if ((status = __parsePost()) != kOk)
        return status;
    if ((status = __parseOS2()) != kOk)
        return status;
    if ((status = __parseName()) != kOk)
        return status;

    return kOk;
}

//------------------------------------------------------------------------------

Status OpenType_Font_Parser::__readWholeFile(FILE *f, size_t *pLen, uint8_t **ppData)
{
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    if (fsize < 0)
        return kError;
    fseek(f, 0, SEEK_SET);

    uint8_t *buffer = (uint8_t*)malloc((size_t)fsize);
    if (!buffer)
        return kOutOfMemory;
    auto buffer_guard = scopeGuard([&buffer]{ free(buffer); });

    if (fread(buffer, 1, (size_t)fsize, f) != (size_t)fsize)
        return kError;

    *pLen = (size_t)fsize;
    *ppData = buffer;
    buffer_guard.dismiss();

    return kOk;
}

Status OpenType_Font_Parser::__parseHead()
{
    if (head_.length < 54) {
        return kCorruption;
    }
    const uint8_t *b = data_ + head_.offset;
    OpenType_Head &head = font_->head_;
    head.Version            = u4(b);
    head.FontRevision       = u4(b + 4);
    head.CheckSumAdjustment = u4(b + 8);
    head.MagicNumber        = u4(b + 12);
    if (head.MagicNumber != 0x5F0F3CF5) {
        return kCorruption;
    }
    head.Flags              = u2(b + 16);
    head.UnitsPerEm         = u2(b + 18);
    head.Created            = u8(b + 20);
    head.Modified           = u8(b + 28);
    head.XMin               = i2(b + 36);
    head.YMin               = i2(b + 38);
    head.XMax               = i2(b + 40);
    head.YMax               = i2(b + 42);
    head.MacStyle           = u2(b + 44);
    head.LowestRecPPEM      = u2(b + 46);
    head.FontDirectionHint  = i2(b + 48);
    head.IndexToLocFormat   = i2(b + 50);
    if (head.IndexToLocFormat != 0 && head.IndexToLocFormat != 1) {
        return kCorruption;
    }
    head.GlyphDataFormat    = i2(b + 52);
    return kOk;
}

Status OpenType_Font_Parser::__parseMaxp()
{
    if (maxp_.length < 6) {
        return kCorruption;
    }
    const uint8_t *b = data_ + maxp_.offset;
    OpenType_Maxp &maxp = font_->maxp_;
    maxp.Version   = u4(b);
    maxp.NumGlyphs = u2(b + 4);
    if (maxp.Version == 0x00005000) {
        return kOk;
    }
    // version 1.0 only
    if (maxp.Version != 0x00010000) {
        return kCorruption;
    }
    if (maxp_.length != 32) {
        return kCorruption;
    }
    maxp.MaxPoints             = u2(b + 6);
    maxp.MaxContours           = u2(b + 8);
    maxp.MaxCompositePoints    = u2(b + 10);
    maxp.MaxCompositeContours  = u2(b + 12);
    maxp.MaxZones              = u2(b + 14);
    maxp.MaxTwilightPoints     = u2(b + 16);
    maxp.MaxStorage            = u2(b + 18);
    maxp.MaxFunctionDefs       = u2(b + 20);
    maxp.MaxInstructionDefs    = u2(b + 22);
    maxp.MaxStackElements      = u2(b + 24);
    maxp.MaxSizeOfInstructions = u2(b + 26);
    maxp.MaxComponentElements  = u2(b + 28);
    maxp.MaxComponentDepth     = u2(b + 30);
    return kOk;
}

Status OpenType_Font_Parser::__parsePost()
{
    if (post_.length < 32) {
        return kCorruption;
    }
    const uint8_t *b = data_ + post_.offset;
    const uint8_t *bend = b + post_.length;
    OpenType_Post &post = font_->post_;
    post.Version            = u4(b);
    post.ItalicAngle        = u4(b + 4);
    post.UnderlinePosition  = i2(b + 8);
    post.UnderlineThickness = i2(b + 10);
    post.IsFixedPitch       = u4(b + 12);
    post.MinMemType42       = u4(b + 16);
    post.MaxMemType42       = u4(b + 20);
    post.MinMemType1        = u4(b + 24);
    post.MaxMemType1        = u4(b + 28);
    font_->glyphNames_.resize(font_->maxp_.NumGlyphs);  // initialize glyphNames_
    if (post.Version == 0x00010000) {
        // Use the Macintosh glyph name
        for (size_t i = 0; i < font_->glyphNames_.size() && i < 258; i++) {
            font_->glyphNames_[i] = GetMacGlyphName(i);
        }
        return kOk;
    }
    if (post.Version == 0x00020000) {
        b += 32;
        uint32_t glyphNameTableHeaderLen = (font_->maxp_.NumGlyphs + 1) * 2;
        if (b + glyphNameTableHeaderLen > bend) {
            return kCorruption;
        }
        // Should be the same as numGlyphs in 'maxp' table
        if (u2(b) != font_->maxp_.NumGlyphs) {
            return kCorruption;
        }
        b += 2;
        std::vector<uint16_t> glyphNameIndex;
        std::vector<const uint8_t*> stringData;
        glyphNameIndex.reserve(font_->maxp_.NumGlyphs);
        stringData.reserve(font_->maxp_.NumGlyphs);
        // Array of indices into the string data
        for (uint16_t i = 0; i < font_->maxp_.NumGlyphs; i++) {
            glyphNameIndex.push_back(u2(b));
            b += 2;
        }
        // Array of PASCAL strings
        while (b < bend) {
            uint8_t len = *b;
            if (b + 1 + len > bend) {
                return kCorruption;
            }
            stringData.push_back(b);
            b += (1 + len);
        }
        for (size_t i = 0; i < glyphNameIndex.size(); i++) {
            uint16_t index = glyphNameIndex[i];
            if (index <= 257) {
                // Use the Macintosh glyph name
                font_->glyphNames_[i] = GetMacGlyphName(index);
            } else {
                // Subtract 258 and use that to index into the list of Pascal strings
                index -= 258;
                if (index < stringData.size()) {
                    const uint8_t *p = stringData[index];
                    uint8_t len = *p;
                    font_->glyphNames_[i].assign((const char*)(p + 1), (const char*)(p + 1 + len + 1));
                }
            }
        }
    }
    return kOk;
}

Status OpenType_Font_Parser::__parseOS2()
{
    // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6OS2.html
    // The original TrueType specification had this table at 68 bytes long. 
    // The first OpenType version had it at 78 bytes long, and the current OpenType version is even larger. 
    // To determine which kind of table your software is dealing with, 
    // it is necessary both to consider the table's version and its size.
    if (os2_.length < 68) {
        return kCorruption;
    }
    const uint8_t *b = data_ + os2_.offset;
    OpenType_OS2 &os2 = font_->os2_;
    os2.version             = u2(b);
    os2.xAvgCharWidth       = i2(b + 2);
    os2.usWeightClass       = u2(b + 4);
    os2.usWidthClass        = u2(b + 6);
    os2.fsType              = u2(b + 8);
    os2.ySubscriptXSize     = i2(b + 10);
    os2.ySubscriptYSize     = i2(b + 12);
    os2.ySubscriptXOffset   = i2(b + 14);
    os2.ySubscriptYOffset   = i2(b + 16);
    os2.ySuperscriptXSize   = i2(b + 18);
    os2.ySuperscriptYSize   = i2(b + 20);
    os2.ySuperscriptXOffset = i2(b + 22);
    os2.ySuperscriptYOffset = i2(b + 24);
    os2.yStrikeoutSize      = i2(b + 26);
    os2.yStrikeoutPosition  = i2(b + 28);
    os2.sFamilyClass        = i2(b + 30);
    memcpy(os2.panose, b + 32, 10);
    os2.ulUnicodeRange1     = u4(b + 42);
    os2.ulUnicodeRange2     = u4(b + 46);
    os2.ulUnicodeRange3     = u4(b + 50);
    os2.ulUnicodeRange4     = u4(b + 54);
    os2.achVendID           = u4(b + 58);
    os2.fsSelection         = u2(b + 62);
    os2.usFirstCharIndex    = u2(b + 64);
    os2.usLastCharIndex     = u2(b + 66);
    if (os2_.length < 78) {
        return kOk;
    }
    os2.sTypoAscender       = i2(b + 68);
    os2.sTypoDescender      = i2(b + 70);
    os2.sTypoLineGap        = i2(b + 72);
    os2.usWinAscent         = u2(b + 74);
    os2.usWinDescent        = u2(b + 76);
    if (os2.version >= 0x0001) {
        if (os2_.length < 86) {
            return kCorruption;
        }
        os2.ulCodePageRange1 = u4(b + 78);
        os2.ulCodePageRange2 = u4(b + 82);
    }
    if (os2.version >= 0x0002) {
        if (os2_.length < 96) {
            return kCorruption;
        }
        os2.sxHeight      = i2(b + 86);
        os2.sCapHeight    = i2(b + 88);
        os2.usDefaultChar = u2(b + 90);
        os2.usBreakChar   = u2(b + 92);
        os2.usMaxContext  = u2(b + 94);
    }
    if (os2.version >= 0x0005) {
        if (os2_.length < 100) {
            return kCorruption;
        }
        os2.usLowerOpticalPointSize = u2(b + 96);
        os2.usUpperOpticalPointSize = u2(b + 98);
    }
    return kOk;
}

Status OpenType_Font_Parser::__parseName()
{
    return kOk;
}

Status OpenType_Font_Parser::__parseCmap()
{
    return kOk;
}
