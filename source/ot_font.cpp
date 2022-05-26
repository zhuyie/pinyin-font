#include "ot_font.h"
#include <cassert>

//------------------------------------------------------------------------------

OpenType_Font::OpenType_Font()
{
    memset(&head_, 0, sizeof(head_));
    memset(&maxp_, 0, sizeof(maxp_));
    memset(&post_, 0, sizeof(post_));
    memset(&os2_,  0, sizeof(os2_) );
    memset(&hhea_, 0, sizeof(hhea_));
}

OpenType_Font::~OpenType_Font()
{
    Clear();
}

int OpenType_Font::GlyphCount() const
{
    return (int)glyphs_.size();
}

Status OpenType_Font::Glyph(int index, OpenType_GlyphHeader **ppGlyph) const
{
    assert(ppGlyph);
    if (index < 0 || index >= (int)glyphs_.size())
        return kInvalidArgs;
    *ppGlyph = glyphs_[index];
    return kOk;
}

Status OpenType_Font::GlyphName(int index, std::string &name) const
{
    if (index < 0 || index >= (int)glyphNames_.size())
        return kInvalidArgs;
    name = glyphNames_[index];
    return kOk;
}

Status OpenType_Font::GlyphHorMetric(int index, OpenType_LongHorMetric &metric) const
{
    if (index < 0 || index >= (int)hmtx_.size())
        return kInvalidArgs;
    metric = hmtx_[index];
    return kOk;
}

uint16_t OpenType_Font::CharToGlyphIndex(uint32_t charcode) const
{
    size_t min = 0, max = char2index_.size(), mid;
    while (min < max) {
        mid = (min + max) >> 1;
        const CmapSequentialMapGroup& group = char2index_[mid];
        if (charcode < group.startCharCode) {
            max = mid;
        } else if (charcode > group.endCharCode) {
            min = mid + 1;
        } else {
            uint32_t offset = charcode - group.startCharCode;
            return group.startGlyphID + offset;
        }
    }
    return 0;
}

Status OpenType_Font::Name(uint16_t nameID, std::vector<OpenType_NameRecord> &records) const
{
    records.clear();
    auto range = names_.equal_range(nameID);
    for (auto i = range.first; i != range.second; i++) {
        records.push_back(i->second);
    }
    return records.empty() ? kNotFound : kOk;
}

void OpenType_Font::Clear()
{
    memset(&head_, 0, sizeof(head_));
    memset(&maxp_, 0, sizeof(maxp_));
    memset(&post_, 0, sizeof(post_));
    memset(&os2_,  0, sizeof(os2_) );
    memset(&hhea_, 0, sizeof(hhea_));

    hmtx_.clear();
    for (auto i = glyphs_.begin(); i != glyphs_.end(); i++) {
        OpenType_GlyphHeader *header = *i;
        if (header == NULL) {  // glyphs which have no outline
            continue;
        }
        if (header->NumberOfContours >= 0) {
            OpenType_GlyphSimple *simple = (OpenType_GlyphSimple*)header;
            delete simple;
        } else {
            OpenType_GlyphComposite *composite = (OpenType_GlyphComposite*)header;
            delete composite;
        }
    }
    glyphs_.clear();
    glyphNames_.clear();
    char2index_.clear();
    names_.clear();
}

Status OpenType_Font::AddGlyph(const OpenType_GlyphHeader *glyph, const OpenType_LongHorMetric *mtx, const char* name)
{
    assert(glyph);
    assert(mtx);

    if (glyphs_.size() == 65535) {
        return kError;
    }

    OpenType_GlyphHeader *newGlyph = NULL;
    if (glyph->NumberOfContours >= 0) {
        const OpenType_GlyphSimple *source = (const OpenType_GlyphSimple*)glyph;
        OpenType_GlyphSimple *simple = new OpenType_GlyphSimple;
        simple->NumberOfContours = source->NumberOfContours;
        simple->XMin = source->XMin;
        simple->XMax = source->XMax;
        simple->YMin = source->YMin;
        simple->YMax = source->YMax;
        simple->EndPtsOfContours = source->EndPtsOfContours;
        simple->Instructions = source->Instructions;
        simple->Points = source->Points;
        newGlyph = simple;
    } else {
        const OpenType_GlyphComposite *source = (const OpenType_GlyphComposite*)glyph;
        OpenType_GlyphComposite *composite = new OpenType_GlyphComposite;
        composite->NumberOfContours = source->NumberOfContours;
        composite->XMin = source->XMin;
        composite->XMax = source->XMax;
        composite->YMin = source->YMin;
        composite->YMax = source->YMax;
        composite->SubGlyphs = source->SubGlyphs;
        composite->Instructions = source->Instructions;
        newGlyph = composite;
    }
    glyphs_.push_back(newGlyph);

    hmtx_.push_back(*mtx);
    glyphNames_.push_back(name);

    maxp_.NumGlyphs++;

    return kOk;
}

//------------------------------------------------------------------------------
