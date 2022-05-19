#include "ot_font_parser.h"
#include "scope_guard.h"
#include "utility.h"
#include "mac_glyph_names.h"
#include "ot_cmap.h"
#include <cassert>
#include <utility>
#include <memory>

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
    case 0x4F54544F: // 'OTTO'
    case 0x74727565: // 'true'
        break;
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
    // CFF font not supported yet
    if (loca_.length == 0 || glyf_.length == 0) {
        return kNotSupported;
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
    if ((status = __parseGlyph()) != kOk)
        return status;
    if ((status = __parseHmtx()) != kOk)
        return status;
    if ((status = __parseCmap()) != kOk)
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
                    font_->glyphNames_[i].assign((const char*)(p + 1), (const char*)(p + 1 + len));
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
    if (name_.length < 6) {
        return kCorruption;
    }
    const uint8_t *b = data_ + name_.offset;
    const uint8_t *bend = b + name_.length;
    uint16_t version       = u2(b);
    uint16_t count         = u2(b + 2);
    uint16_t storageOffset = i2(b + 4);
    for (uint16_t i = 0; i < count; i++) {
        const uint8_t *entry = b + 6 + i * 12;
        if (entry > bend) {
            return kCorruption;
        }
        uint16_t platformID   = u2(entry);
        uint16_t encodingID   = u2(entry + 2);
        uint16_t languageID   = u2(entry + 4);
        uint16_t nameID       = u2(entry + 6);
        uint16_t length       = u2(entry + 8);  // in bytes
        uint16_t stringOffset = u2(entry + 10);
        // Strings for the platform 0(Unicode) and 3(Windows) must be encoded in UTF-16BE.
        if (platformID == 0 || platformID == 3) {
            const uint8_t *str = b + storageOffset + stringOffset;
            if (str + length > bend) {
                return kCorruption;
            }
            auto it = font_->names_.insert(
                decltype(font_->names_)::value_type(nameID, OpenType_NameRecord())
            );
            OpenType_NameRecord &record = it->second;
            record.PlatformID = platformID;
            record.EncodingID = encodingID;
            record.LanguageID = languageID;
            record.NameID     = nameID;
            length /= 2;
            record.String.resize(length);
            for (uint16_t i = 0; i < length; i++) {
                record.String[i] = u2(str + i * 2);
            }
        }
    }
    return kOk;
}

Status OpenType_Font_Parser::__parseGlyph()
{
    uint32_t requiredLocaLen = 2 * font_->maxp_.NumGlyphs; // short version
    if (font_->head_.IndexToLocFormat != 0) {
        requiredLocaLen *= 2; // long version
    }
    if (loca_.length < requiredLocaLen) {
        return kCorruption;
    }

    font_->glyphs_.resize(font_->maxp_.NumGlyphs);

    for (uint16_t i = 0; i < font_->maxp_.NumGlyphs; i++) {
        uint32_t start, end;
        if (font_->head_.IndexToLocFormat == 0) {
            // short version
            start = 2 * (uint32_t)u2(data_ + loca_.offset + 2*i);
            end = 2 * (uint32_t)u2(data_ + loca_.offset + 2*(i+1));
        } else {
            // long version
            start = u4(data_ + loca_.offset + 4*i);
            end = u4(data_ + loca_.offset + 4*(i+1));
        }
        if (end < start || end > glyf_.length) {
            return kCorruption;
        }
        if (start == end) {
            // If a glyph has no outline, then loca[n] = loca [n+1].
            continue;
        }

        const uint8_t *glyphData = data_ + glyf_.offset + start;
        size_t glyphLen = end - start;
        OpenType_GlyphHeader header;
        Status status = __parseGlyphHeader(glyphData, glyphLen, header);
        if (status != kOk) {
            return status;
        }
        if (header.NumberOfContours >= 0) {
            // Simple
            std::unique_ptr<OpenType_GlyphSimple> pGlyph(new OpenType_GlyphSimple());
            pGlyph->NumberOfContours = header.NumberOfContours;
            pGlyph->XMin = header.XMin;
            pGlyph->YMin = header.YMin;
            pGlyph->XMax = header.XMax;
            pGlyph->YMax = header.YMax;
            status = __parseGlyphSimple(glyphData, glyphLen, *pGlyph);
            if (status != kOk) {
                return status;
            }
            font_->glyphs_[i] = pGlyph.release();
        } else {
            // Composite
            std::unique_ptr<OpenType_GlyphComposite> pGlyph(new OpenType_GlyphComposite());
            pGlyph->NumberOfContours = header.NumberOfContours;
            pGlyph->XMin = header.XMin;
            pGlyph->YMin = header.YMin;
            pGlyph->XMax = header.XMax;
            pGlyph->YMax = header.YMax;
            status = __parseGlyphComposite(glyphData, glyphLen, *pGlyph);
            if (status != kOk) {
                return status;
            }
            font_->glyphs_[i] = pGlyph.release();
        }
    }

    return kOk;
}

Status OpenType_Font_Parser::__parseGlyphHeader(const uint8_t *data, size_t len, OpenType_GlyphHeader &header)
{
    if (len < 10) {
        return kCorruption;
    }
    header.NumberOfContours = i2(data);
    header.XMin = i2(data + 2);
    header.YMin = i2(data + 4);
    header.XMax = i2(data + 6);
    header.YMax = i2(data + 8);
    return kOk;
}

Status OpenType_Font_Parser::__parseGlyphSimple(const uint8_t *data, size_t len, OpenType_GlyphSimple &simple)
{
    // skip the glyph header
    size_t parsed = 10;
    // endPtsOfContours
    if (simple.NumberOfContours > 0) {
        if (parsed + 2 * simple.NumberOfContours > len) {
            return kCorruption;
        }
        simple.EndPtsOfContours.reserve(simple.NumberOfContours);
        for (auto i = 0; i < simple.NumberOfContours; i++) {
            uint16_t n = u2(data + parsed + i * 2);
            simple.EndPtsOfContours.push_back(n);
        }
        parsed += 2 * simple.NumberOfContours;
    }
    // instructions
    if (parsed + 2 > len) {
        return kCorruption;
    }
    uint16_t instructionLength = u2(data + parsed);
    parsed += 2;
    if (instructionLength > 0) {
        if (parsed + instructionLength > len) {
            return kCorruption;
        }
        simple.Instructions.reserve(instructionLength);
        simple.Instructions.assign(data + parsed, data + parsed + instructionLength);
        parsed += instructionLength;
    }
    if (simple.NumberOfContours == 0) {
        return kOk;
    }
    uint16_t numberOfPoints = simple.EndPtsOfContours.back() + 1;
    simple.Points.resize(numberOfPoints);
    // flags
    for (auto i = 0; i < numberOfPoints; i++) {
        if (parsed + 1 > len) {
            return kCorruption;
        }
        uint8_t flags = data[parsed];
        parsed++;
        simple.Points[i].Flags = flags;
        if (flags & OpenType_FlagRepeat) {
            if (parsed + 1 > len) {
                return kCorruption;
            }
            uint8_t count = data[parsed];
            parsed++;
            for (; count > 0; count--) {
                i++;
                simple.Points[i].Flags = flags;
            }
        }
    }
    // xCoordinates
    int16_t x = 0;
    for (auto i = 0; i < numberOfPoints; i++) {
        int16_t dx = 0;
        uint8_t flags = simple.Points[i].Flags;
        if (flags & OpenType_FlagXShortVector) {
            if (parsed + 1 > len) {
                return kCorruption;
            }
            dx = data[parsed];
            parsed++;
            if ((flags & OpenType_FlagPositiveXShortVector) == 0) {
                dx = -dx;
            }
        } else if ((flags & OpenType_FlagXIsSame) == 0) {
            if (parsed + 2 > len) {
                return kCorruption;
            }
            dx = i2(data + parsed);
            parsed += 2;
        }
        x += dx;
        simple.Points[i].X = x;
    }
    // yCoordinates
    int16_t y = 0;
    for (auto i = 0; i < numberOfPoints; i++) {
        int16_t dy = 0;
        uint8_t flags = simple.Points[i].Flags;
        if (flags & OpenType_FlagYShortVector) {
            if (parsed + 1 > len) {
                return kCorruption;
            }
            dy = data[parsed];
            parsed++;
            if ((flags & OpenType_FlagPositiveYShortVector) == 0) {
                dy = -dy;
            }
        } else if ((flags & OpenType_FlagYIsSame) == 0) {
            if (parsed + 2 > len) {
                return kCorruption;
            }
            dy = i2(data + parsed);
            parsed += 2;
        }
        y += dy;
        simple.Points[i].Y = y;
    }
    return kOk;
}

Status OpenType_Font_Parser::__parseGlyphComposite(const uint8_t *data, size_t len, OpenType_GlyphComposite &composite)
{
    // skip the glyph header
    size_t parsed = 10;
    // components
    uint16_t flags = 0;
    for (;;) {
        OpenType_GlyphComponent subglyph;
        // flags && glyphIndex
        if (parsed + 4 > len) {
            return kCorruption;
        }
        flags = u2(data + parsed);
        subglyph.Flags = flags;
        parsed += 2;
        subglyph.GlyphIndex = u2(data + parsed);
        parsed += 2;
        // argument1 && argument2
        if (flags & OpenType_FlagArg1And2AreWords) {
            if (parsed + 4 > len) {
                return kCorruption;
            }
            subglyph.Arg1 = i2(data + parsed);
            subglyph.Arg2 = i2(data + parsed + 2);
            parsed += 4;
        } else {
            if (parsed + 2 > len) {
                return kCorruption;
            }
            subglyph.Arg1 = data[parsed];
            subglyph.Arg2 = data[parsed + 1];
            parsed += 2;
        }
        // transformation
        if (flags & OpenType_FlagWeHaveAScale) {
            if (parsed + 2 > len) {
                return kCorruption;
            }
            subglyph.Transform[0] = i2(data + parsed);
            subglyph.Transform[3] = subglyph.Transform[0];
            parsed += 2;
        } else if (flags & OpenType_FlagWeHaveAnXAndYScale) {
            if (parsed + 4 > len) {
                return kCorruption;
            }
            subglyph.Transform[0] = i2(data + parsed);
            subglyph.Transform[3] = i2(data + parsed + 2);
            parsed += 4;
        } else if (flags & OpenType_FlagWeHaveATwoByTwo) {
            if (parsed + 8 > len) {
                return kCorruption;
            }
            subglyph.Transform[0] = i2(data + parsed);
            subglyph.Transform[1] = i2(data + parsed + 2);
            subglyph.Transform[2] = i2(data + parsed + 4);
            subglyph.Transform[3] = i2(data + parsed + 6);
            parsed += 8;
        }

        composite.SubGlyphs.push_back(subglyph);

        if (!(flags & OpenType_FlagMoreComponents)) {
            break;
        }
    }
    // instructions
    if (flags & OpenType_FlagWeHaveInstructions) {
        if (parsed + 2 > len) {
            return kCorruption;
        }
        uint16_t instructionLen = u2(data + parsed);
        parsed += 2;
        if (instructionLen == 0 || parsed + instructionLen > len) {
            return kCorruption;
        }
        composite.Instructions.assign(data+parsed, data+parsed+instructionLen);
        parsed += instructionLen;
    }
    return kOk;
}

Status OpenType_Font_Parser::__parseHmtx()
{
    if (hhea_.length < 36) {
        return kCorruption;
    }
    const uint8_t *b = data_ + hhea_.offset;
    OpenType_Hhea &hhea = font_->hhea_;
    hhea.MajorVersion        = u2(b);
    hhea.MinorVersion        = u2(b + 2);
    hhea.Ascender            = i2(b + 4);
    hhea.Descender           = i2(b + 6);
    hhea.LineGap             = i2(b + 8);
    hhea.AdvanceWidthMax     = u2(b + 10);
    hhea.MinLeftSideBearing  = i2(b + 12);
    hhea.MinRightSideBearing = i2(b + 14);
    hhea.XMaxExtent          = i2(b + 16);
    hhea.CaretSlopeRise      = i2(b + 18);
    hhea.CaretSlopeRun       = i2(b + 20);
    hhea.CaretOffset         = i2(b + 22);
    hhea.Reserved0           = i2(b + 24);
    hhea.Reserved1           = i2(b + 26);
    hhea.Reserved2           = i2(b + 28);
    hhea.Reserved3           = i2(b + 30);
    hhea.MetricDataFormat    = i2(b + 32);
    hhea.NumberOfHMetrics    = u2(b + 34);

    if (hhea.NumberOfHMetrics < 1) {
        return kCorruption;
    }
    uint32_t requiredHmtxLen = font_->maxp_.NumGlyphs * 2 + hhea.NumberOfHMetrics * 2;
    if (hmtx_.length < requiredHmtxLen) {
        return kCorruption;
    }
    b = data_ + hmtx_.offset;
    font_->hmtx_.resize(font_->maxp_.NumGlyphs);
    for (uint16_t i = 0; i < hhea.NumberOfHMetrics; i++) {
        OpenType_LongHorMetric &entry = font_->hmtx_[i];
        entry.AdvanceWidth = u2(b);
        entry.LSB          = i2(b + 2);
        b += 4;
    }
    uint16_t lastAdvanceWidth = font_->hmtx_[hhea.NumberOfHMetrics - 1].AdvanceWidth;
    for (uint16_t i = hhea.NumberOfHMetrics; i < font_->maxp_.NumGlyphs; i++) {
        OpenType_LongHorMetric &entry = font_->hmtx_[i];
        entry.AdvanceWidth = lastAdvanceWidth;
        entry.LSB = i2(b);
        b += 2;
    }

    return kOk;
}

Status OpenType_Font_Parser::__parseCmap()
{
    if (cmap_.length < 4) {
        return kCorruption;
    }
    const uint16_t cbEncodingRecord = 8;
    const uint8_t *b = data_ + cmap_.offset;
    // skip table version
    uint16_t numTables = u2(b + 2);
    if (cmap_.length < 4 + numTables * cbEncodingRecord) {
        return kCorruption;
    }
    // search for best subtable
    int bestPriority = 0;
    uint16_t bestPlatform = 0;
    uint16_t bestEncoding = 0;
    uint32_t bestOffset = 0;
    for (uint16_t i = 0; i < numTables; i++) {
        uint16_t platformId = u2(b + 4 + i * cbEncodingRecord);
        uint16_t encodingId = u2(b + 4 + i * cbEncodingRecord + 2);
        uint32_t offset     = u4(b + 4 + i * cbEncodingRecord + 4);
        if (offset >= cmap_.length) {
            return kCorruption;
        }
        int priority = 0;
        if (platformId == 0 && encodingId == 3) {  // Platform=Unicode, Encoding=Unicode_BMP
            priority = 1;
        }
        if (platformId == 3 && encodingId == 1) {  // Platform=Windows, Encoding=Unicode_BMP
            priority = 2;
        }
        if (platformId == 0 && encodingId == 4) {  // Platform=Unicode, Encoding=Unicode_Full
            priority = 3;
        }
        if (platformId == 3 && encodingId == 10) { // Platform=Windows, Encoding=Unicode_Full
            priority = 4;
        }
        if (priority > bestPriority) {
            bestPriority = priority;
            bestPlatform = platformId;
            bestEncoding = encodingId;
            bestOffset   = offset;
        }
    }
    if (bestPriority == 0) {
        return kNotSupported;
    }
    const uint8_t *start = b + bestOffset;
    const uint8_t *end = b + cmap_.length;
    return __parseCmapSubtable(start, end, bestPlatform, bestEncoding);
}

Status OpenType_Font_Parser::__parseCmapSubtable(const uint8_t *start, const uint8_t *end, uint16_t platformId, uint16_t encodingId)
{
    const uint8_t *b = start;
    if (b + 2 > end) {
        return kCorruption;
    }
    uint16_t format = u2(b);
    b += 2;

    std::unique_ptr<CmapSubtable> subtable;
    if (format == 0) {
        subtable = CreateCmapSubtableFormat0(platformId, encodingId);
    } else if (format == 4) {
        subtable = CreateCmapSubtableFormat4(platformId, encodingId);
    } else if (format == 6) {
        subtable = CreateCmapSubtableFormat6(platformId, encodingId);
    } else if (format == 12) {
        subtable = CreateCmapSubtableFormat12(platformId, encodingId);
    } else {
        return kNotSupported;
    }

    Status status = subtable->Parse(b, end);
    if (status != kOk) {
        return status;
    }

    font_->char2index_ = std::move(subtable);
    return kOk;
}
