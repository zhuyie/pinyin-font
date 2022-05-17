#include "ot_font_parser.h"
#include "scope_guard.h"
#include "utility.h"

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
        } else if (memcmp(t.name, "os/2", 4) == 0) {
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
    head.Version = u4(b);
    head.FontRevision = u4(b + 4);
    head.CheckSumAdjustment = u4(b + 8);
    head.MagicNumber = u4(b + 12);
    if (head.MagicNumber != 0x5F0F3CF5) {
        return kCorruption;
    }
    head.Flags = u2(b + 16);
    head.UnitsPerEm = u2(b + 18);
    head.Created = u8(b + 20);
    head.Modified = u8(b + 28);
    head.XMin = i2(b + 36);
    head.YMin = i2(b + 38);
    head.XMax = i2(b + 40);
    head.YMax = i2(b + 42);
    head.MacStyle = u2(b + 44);
    head.LowestRecPPEM = u2(b + 46);
    head.FontDirectionHint = i2(b + 48);
    head.IndexToLocFormat = i2(b + 50);
    if (head.IndexToLocFormat != 0 && head.IndexToLocFormat != 1) {
        return kCorruption;
    }
    head.GlyphDataFormat = i2(b + 52);
    return kOk;
}

Status OpenType_Font_Parser::__parseMaxp()
{
    if (maxp_.length < 6) {
        return kCorruption;
    }
    const uint8_t *b = data_ + maxp_.offset;
    OpenType_Maxp &maxp = font_->maxp_;
    maxp.Version = u4(b);
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
    maxp.MaxPoints = u2(b + 6);
    maxp.MaxContours = u2(b + 8);
    maxp.MaxCompositePoints = u2(b + 10);
    maxp.MaxCompositeContours = u2(b + 12);
    maxp.MaxZones = u2(b + 14);
    maxp.MaxTwilightPoints = u2(b + 16);
    maxp.MaxStorage = u2(b + 18);
    maxp.MaxFunctionDefs = u2(b + 20);
    maxp.MaxInstructionDefs = u2(b + 22);
    maxp.MaxStackElements = u2(b + 24);
    maxp.MaxSizeOfInstructions = u2(b + 26);
    maxp.MaxComponentElements = u2(b + 28);
    maxp.MaxComponentDepth = u2(b + 30);
    return kOk;
}

Status OpenType_Font_Parser::__parsePost()
{
    return kOk;
}

Status OpenType_Font_Parser::__parseOS2()
{
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
