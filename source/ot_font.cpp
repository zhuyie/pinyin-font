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
    if (char2index_) {
        return char2index_->Query(charcode);
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
    char2index_.reset(nullptr);
    names_.clear();
}

//------------------------------------------------------------------------------
