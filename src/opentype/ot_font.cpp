#include "ot_font.h"
#include "mac_glyph_names.h"
#include <algorithm>
#include <cassert>
#include <functional>

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

Status OpenType_Font::Glyph(int index, const OpenType_GlyphHeader **ppGlyph) const
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
    auto entry = glyphNames_[index];
    if (entry.ID < 258) {
        name = GetMacGlyphName(entry.ID);
    } else {
        name = entry.Str;
    }
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
    ligatureSubstitutions_.clear();
    names_.clear();

    cvt_.clear();
    fpgm_.clear();
    prep_.clear();
}

Status OpenType_Font::AddGlyph(
    const OpenType_GlyphHeader *glyph, 
    const OpenType_LongHorMetric *mtx, 
    const OpenType_GlyphName &name,
    uint16_t &glyphIndex)
{
    assert(glyph);
    assert(mtx);

    glyphIndex = 0;

    if (glyphs_.size() == 65535) {
        return kError;
    }

    int appendedCompositeDepth = 0;
    if (glyph->NumberOfContours < 0) {
        const OpenType_GlyphComposite *source =
            (const OpenType_GlyphComposite*)glyph;
        std::vector<uint8_t> visiting(glyphs_.size(), 0);
        std::function<int(uint16_t)> depth = [&](uint16_t index) -> int {
            if (index >= glyphs_.size() || visiting[index]) return -1;
            const OpenType_GlyphHeader *component = glyphs_[index];
            if (component == NULL || component->NumberOfContours >= 0) return 0;
            visiting[index] = 1;
            const OpenType_GlyphComposite *composite =
                (const OpenType_GlyphComposite*)component;
            int result = 1;
            for (size_t i = 0; i < composite->SubGlyphs.size(); i++) {
                int childDepth = depth(composite->SubGlyphs[i].GlyphIndex);
                if (childDepth < 0) {
                    visiting[index] = 0;
                    return -1;
                }
                result = std::max(result, 1 + childDepth);
            }
            visiting[index] = 0;
            return result;
        };

        appendedCompositeDepth = 1;
        for (size_t i = 0; i < source->SubGlyphs.size(); i++) {
            int childDepth = depth(source->SubGlyphs[i].GlyphIndex);
            if (childDepth < 0) return kError;
            appendedCompositeDepth =
                std::max(appendedCompositeDepth, 1 + childDepth);
        }
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
    if (newGlyph->XMin < head_.XMin)
        head_.XMin = newGlyph->XMin;
    if (newGlyph->YMin < head_.YMin)
        head_.YMin = newGlyph->YMin;
    if (newGlyph->XMax > head_.XMax)
        head_.XMax = newGlyph->XMax;
    if (newGlyph->YMax > head_.YMax)
        head_.YMax = newGlyph->YMax;

    if (mtx->AdvanceWidth > hhea_.AdvanceWidthMax)
        hhea_.AdvanceWidthMax = mtx->AdvanceWidth;
    if (mtx->LSB < hhea_.MinLeftSideBearing)
        hhea_.MinLeftSideBearing = mtx->LSB;
    int16_t extent = (int16_t)(mtx->LSB + newGlyph->XMax - newGlyph->XMin);
    if (extent > hhea_.XMaxExtent)
        hhea_.XMaxExtent = extent;
    int16_t rsb = (int16_t)(mtx->AdvanceWidth - extent);
    if (rsb < hhea_.MinRightSideBearing)
        hhea_.MinRightSideBearing = rsb;

    if (newGlyph->NumberOfContours >= 0) {
        const OpenType_GlyphSimple *simple = (const OpenType_GlyphSimple*)newGlyph;
        if (simple->Points.size() > maxp_.MaxPoints)
            maxp_.MaxPoints = (uint16_t)simple->Points.size();
        if (simple->EndPtsOfContours.size() > maxp_.MaxContours)
            maxp_.MaxContours = (uint16_t)simple->EndPtsOfContours.size();
    } else {
        const OpenType_GlyphComposite *composite = (const OpenType_GlyphComposite*)newGlyph;
        if (composite->SubGlyphs.size() > maxp_.MaxComponentElements)
            maxp_.MaxComponentElements = (uint16_t)composite->SubGlyphs.size();
        if (appendedCompositeDepth > maxp_.MaxComponentDepth)
            maxp_.MaxComponentDepth = (uint16_t)appendedCompositeDepth;
    }

    assert(glyphs_.size() == hmtx_.size());
    assert(glyphs_.size() == glyphNames_.size());
    assert(glyphs_.size() == (size_t)maxp_.NumGlyphs);
    glyphIndex = (uint16_t)(glyphs_.size() - 1);

    return kOk;
}

Status OpenType_Font::SetCmap(
    const std::vector<CmapSequentialMapGroup> &groups)
{
    if (groups.empty()) {
        return kError;
    }
    char2index_ = groups;
    return kOk;
}

Status OpenType_Font::AddLigatureSubstitution(
    const std::vector<uint16_t> &components,
    uint16_t ligatureGlyph)
{
    if (components.size() < 2 || components.size() > 0xFFFF ||
        ligatureGlyph == 0 || ligatureGlyph >= glyphs_.size()) {
        return kInvalidArgs;
    }
    for (size_t i = 0; i < components.size(); i++) {
        if (components[i] == 0 || components[i] >= glyphs_.size()) {
            return kInvalidArgs;
        }
    }
    for (size_t i = 0; i < ligatureSubstitutions_.size(); i++) {
        if (ligatureSubstitutions_[i].Components == components) {
            return kInvalidArgs;
        }
    }
    OpenType_LigatureSubstitution rule;
    rule.Components = components;
    rule.LigatureGlyph = ligatureGlyph;
    ligatureSubstitutions_.push_back(rule);
    std::sort(
        ligatureSubstitutions_.begin(), ligatureSubstitutions_.end(),
        [](const OpenType_LigatureSubstitution &a,
           const OpenType_LigatureSubstitution &b) {
            if (a.Components[0] != b.Components[0])
                return a.Components[0] < b.Components[0];
            if (a.Components.size() != b.Components.size())
                return a.Components.size() > b.Components.size();
            return a.Components < b.Components;
        });
    return kOk;
}

//------------------------------------------------------------------------------
