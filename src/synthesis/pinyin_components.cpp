#include "pinyin_components.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {

int16_t clampInt16(int value, int minimum, int maximum)
{
    if (value < minimum) return (int16_t)minimum;
    if (value > maximum) return (int16_t)maximum;
    return (int16_t)value;
}

struct ContourBounds {
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    uint16_t start;
    uint16_t end;
    double area;
};

struct ContourGroup {
    std::vector<size_t> contours;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    double area;
};

bool boxesOverlap(const ContourBounds &a, const ContourBounds &b)
{
    return a.xMin <= b.xMax && b.xMin <= a.xMax &&
           a.yMin <= b.yMax && b.yMin <= a.yMax;
}

size_t findRoot(std::vector<size_t> &parents, size_t index)
{
    while (parents[index] != index) {
        parents[index] = parents[parents[index]];
        index = parents[index];
    }
    return index;
}

void join(std::vector<size_t> &parents, size_t a, size_t b)
{
    a = findRoot(parents, a);
    b = findRoot(parents, b);
    if (a != b) parents[b] = a;
}

double contourArea(
    const std::vector<OpenType_GlyphPoint> &points,
    uint16_t start,
    uint16_t end)
{
    if (end < start || end >= points.size()) return 0.0;
    double area = 0.0;
    uint16_t previous = end;
    for (uint16_t i = start; i <= end; i++) {
        area += (double)points[previous].X * points[i].Y -
                (double)points[i].X * points[previous].Y;
        previous = i;
    }
    return area * 0.5;
}

} // namespace

PinyinComponents::PinyinComponents(OpenType_Font &font)
: font_(font),
  xHeight_(resolveXHeight()),
  markGap_(0),
  dotlessIStatus_(kPinyinDotlessIUnknown),
  dotlessIGlyphIndex_(0),
  dotlessIUseCount_(0)
{
    int unitsPerEm = font_.Head().UnitsPerEm;
    // The complete pinyin cluster is scaled to roughly 35% in the generated
    // Han glyph. A conventional unscaled accent gap becomes nearly invisible
    // after that transform, so retain a larger source-space gap here.
    int minimum = std::max(1, unitsPerEm * 50 / 1000);
    int maximum = std::max(minimum, unitsPerEm * 120 / 1000);
    markGap_ = clampInt16((int)std::lround(xHeight_ * 0.14), minimum, maximum);
}

int16_t PinyinComponents::resolveXHeight() const
{
    int unitsPerEm = font_.Head().UnitsPerEm;
    int16_t tableXHeight = font_.OS2().sxHeight;
    if (tableXHeight >= unitsPerEm * 20 / 100 &&
        tableXHeight <= unitsPerEm * 90 / 100) {
        return tableXHeight;
    }

    static const char representatives[] = {
        'a', 'c', 'e', 'm', 'n', 'o', 'r', 's', 'u', 'v', 'w', 'x', 'z'
    };
    std::vector<int16_t> heights;
    for (size_t i = 0; i < sizeof(representatives); i++) {
        uint16_t glyphIndex = font_.CharToGlyphIndex(representatives[i]);
        if (glyphIndex == 0) continue;
        const OpenType_GlyphHeader *glyph = nullptr;
        if (font_.Glyph(glyphIndex, &glyph) != kOk || glyph == nullptr) continue;
        int height = glyph->YMax - std::max<int16_t>(0, glyph->YMin);
        if (height > 0) heights.push_back((int16_t)height);
    }
    if (heights.size() >= 3) {
        std::sort(heights.begin(), heights.end());
        return heights[heights.size() / 2];
    }

    int fallback = unitsPerEm / 2;
    return clampInt16(fallback, std::max(1, unitsPerEm / 3), std::max(1, unitsPerEm * 2 / 3));
}

bool PinyinComponents::alternativeMark(wchar_t mark, wchar_t &alternative)
{
    switch (mark) {
    case 0x0304: alternative = 0x00AF; return true;
    case 0x0301: alternative = 0x00B4; return true;
    case 0x030C: alternative = 0x02C7; return true;
    case 0x0300: alternative = 0x0060; return true;
    case 0x0308: alternative = 0x00A8; return true;
    default: return false;
    }
}

Status PinyinComponents::ResolveMark(wchar_t mark, PinyinComponentGlyph &component)
{
    uint16_t glyphIndex = font_.CharToGlyphIndex(mark);
    if (glyphIndex == 0) {
        wchar_t alternative = 0;
        if (alternativeMark(mark, alternative)) {
            glyphIndex = font_.CharToGlyphIndex(alternative);
        }
    }

    bool generated = false;
    if (glyphIndex == 0) {
        auto found = generatedMarks_.find(mark);
        if (found == generatedMarks_.end()) {
            Status status = createMark(mark, glyphIndex);
            if (status != kOk) return status;
            generatedMarks_[mark] = glyphIndex;
        } else {
            glyphIndex = found->second;
        }
        generated = true;
        generatedMarkUseCounts_[mark]++;
    }

    const OpenType_GlyphHeader *glyph = nullptr;
    Status status = font_.Glyph(glyphIndex, &glyph);
    if (status != kOk || glyph == nullptr) return kNotFound;
    component.GlyphIndex = glyphIndex;
    component.Bounds = { glyph->XMin, glyph->YMin, glyph->XMax, glyph->YMax };
    component.Generated = generated;
    return kOk;
}

uint32_t PinyinComponents::GeneratedMarkUseCount(wchar_t mark) const
{
    auto found = generatedMarkUseCounts_.find(mark);
    return found == generatedMarkUseCounts_.end() ? 0 : found->second;
}

void PinyinComponents::addContour(
    OpenType_GlyphSimple &glyph,
    const int16_t coordinates[][2],
    size_t count)
{
    for (size_t i = 0; i < count; i++) {
        OpenType_GlyphPoint point = {};
        point.Flags = OpenType_FlagOnCurve;
        point.X = coordinates[i][0];
        point.Y = coordinates[i][1];
        glyph.Points.push_back(point);
    }
    glyph.EndPtsOfContours.push_back((uint16_t)(glyph.Points.size() - 1));
}

bool PinyinComponents::CalculateBounds(
    const std::vector<OpenType_GlyphPoint> &points,
    PinyinComponentBounds &bounds)
{
    if (points.empty()) return false;
    bounds.XMin = bounds.XMax = points[0].X;
    bounds.YMin = bounds.YMax = points[0].Y;
    for (size_t i = 1; i < points.size(); i++) {
        bounds.XMin = std::min(bounds.XMin, points[i].X);
        bounds.YMin = std::min(bounds.YMin, points[i].Y);
        bounds.XMax = std::max(bounds.XMax, points[i].X);
        bounds.YMax = std::max(bounds.YMax, points[i].Y);
    }
    return true;
}

void PinyinComponents::finalizeGlyphBounds(OpenType_GlyphSimple &glyph)
{
    PinyinComponentBounds bounds = {};
    if (CalculateBounds(glyph.Points, bounds)) {
        glyph.XMin = bounds.XMin;
        glyph.YMin = bounds.YMin;
        glyph.XMax = bounds.XMax;
        glyph.YMax = bounds.YMax;
    } else {
        glyph.XMin = glyph.YMin = glyph.XMax = glyph.YMax = 0;
    }
    glyph.NumberOfContours = (int16_t)glyph.EndPtsOfContours.size();
}

Status PinyinComponents::addSimpleGlyph(
    const OpenType_GlyphSimple &glyph,
    const OpenType_LongHorMetric &metric,
    const char *name,
    uint16_t &glyphIndex)
{
    OpenType_GlyphName glyphName;
    glyphName.ID = 258;
    glyphName.Str = name;
    return font_.AddGlyph(&glyph, &metric, glyphName, glyphIndex);
}

Status PinyinComponents::createMark(wchar_t mark, uint16_t &glyphIndex)
{
    int unitsPerEm = font_.Head().UnitsPerEm;
    int stroke = clampInt16(
        (int)std::lround(xHeight_ * 0.08),
        std::max(1, unitsPerEm * 25 / 1000),
        std::max(1, unitsPerEm * 90 / 1000));
    int width = std::max(stroke * 2, (int)std::lround(xHeight_ * 0.42));
    int height = std::max(stroke * 2, (int)std::lround(xHeight_ * 0.20));
    int halfWidth = width / 2;

    OpenType_GlyphSimple glyph = {};
    std::string name;
    switch (mark) {
    case 0x0304: {
        const int16_t contour[][2] = {
            { (int16_t)-halfWidth, 0 },
            { (int16_t)halfWidth, 0 },
            { (int16_t)halfWidth, (int16_t)stroke },
            { (int16_t)-halfWidth, (int16_t)stroke }
        };
        addContour(glyph, contour, 4);
        name = "pinyin.mark.macron";
        break;
    }
    case 0x0301: {
        int diagonalStroke = std::max(
            stroke, (int)std::lround(xHeight_ * 0.16));
        int diagonalWidth = std::max(
            diagonalStroke * 2, (int)std::lround(xHeight_ * 0.30));
        int diagonalHeight = std::max(
            diagonalStroke * 2, (int)std::lround(xHeight_ * 0.32));
        int diagonalHalfWidth = diagonalWidth / 2;
        const int16_t contour[][2] = {
            { (int16_t)-diagonalHalfWidth, 0 },
            { (int16_t)(-diagonalHalfWidth + diagonalStroke), 0 },
            { (int16_t)diagonalHalfWidth, (int16_t)diagonalHeight },
            { (int16_t)(diagonalHalfWidth - diagonalStroke), (int16_t)diagonalHeight }
        };
        addContour(glyph, contour, 4);
        name = "pinyin.mark.acute";
        break;
    }
    case 0x0300: {
        int diagonalStroke = std::max(
            stroke, (int)std::lround(xHeight_ * 0.16));
        int diagonalWidth = std::max(
            diagonalStroke * 2, (int)std::lround(xHeight_ * 0.30));
        int diagonalHeight = std::max(
            diagonalStroke * 2, (int)std::lround(xHeight_ * 0.32));
        int diagonalHalfWidth = diagonalWidth / 2;
        const int16_t contour[][2] = {
            { (int16_t)(diagonalHalfWidth - diagonalStroke), 0 },
            { (int16_t)diagonalHalfWidth, 0 },
            { (int16_t)(-diagonalHalfWidth + diagonalStroke), (int16_t)diagonalHeight },
            { (int16_t)-diagonalHalfWidth, (int16_t)diagonalHeight }
        };
        addContour(glyph, contour, 4);
        name = "pinyin.mark.grave";
        break;
    }
    case 0x030C: {
        int overlap = std::max(1, stroke / 3);
        const int16_t left[][2] = {
            { (int16_t)-halfWidth, (int16_t)height },
            { (int16_t)(-halfWidth + stroke), (int16_t)height },
            { (int16_t)overlap, 0 },
            { (int16_t)-overlap, 0 }
        };
        const int16_t right[][2] = {
            { (int16_t)-overlap, 0 },
            { (int16_t)overlap, 0 },
            { (int16_t)halfWidth, (int16_t)height },
            { (int16_t)(halfWidth - stroke), (int16_t)height }
        };
        addContour(glyph, left, 4);
        addContour(glyph, right, 4);
        name = "pinyin.mark.caron";
        break;
    }
    case 0x0308: {
        int diameter = std::max(stroke, (int)std::lround(xHeight_ * 0.14));
        int radius = diameter / 2;
        int spacing = std::max(stroke, (int)std::lround(xHeight_ * 0.12));
        int centers[2] = { -(spacing + diameter) / 2, (spacing + diameter) / 2 };
        for (size_t i = 0; i < 2; i++) {
            int cx = centers[i];
            int edge = std::max(1, radius * 2 / 3);
            const int16_t dot[][2] = {
                { (int16_t)(cx - edge), 0 },
                { (int16_t)(cx + edge), 0 },
                { (int16_t)(cx + radius), (int16_t)(radius - edge) },
                { (int16_t)(cx + radius), (int16_t)(radius + edge) },
                { (int16_t)(cx + edge), (int16_t)(diameter) },
                { (int16_t)(cx - edge), (int16_t)(diameter) },
                { (int16_t)(cx - radius), (int16_t)(radius + edge) },
                { (int16_t)(cx - radius), (int16_t)(radius - edge) }
            };
            addContour(glyph, dot, 8);
        }
        name = "pinyin.mark.diaeresis";
        break;
    }
    default:
        return kNotSupported;
    }

    finalizeGlyphBounds(glyph);
    OpenType_LongHorMetric metric = {};
    metric.AdvanceWidth = (uint16_t)std::max(1, glyph.XMax - glyph.XMin);
    metric.LSB = glyph.XMin;
    return addSimpleGlyph(glyph, metric, name.c_str(), glyphIndex);
}

bool PinyinComponents::DeriveDotlessI(
    const OpenType_GlyphSimple &source,
    int16_t xHeight,
    OpenType_GlyphSimple &derived)
{
    derived = OpenType_GlyphSimple();
    if (source.NumberOfContours < 2 ||
        source.EndPtsOfContours.size() != (size_t)source.NumberOfContours ||
        source.Points.empty()) {
        return false;
    }

    std::vector<ContourBounds> contours;
    uint16_t start = 0;
    for (size_t i = 0; i < source.EndPtsOfContours.size(); i++) {
        uint16_t end = source.EndPtsOfContours[i];
        if (end < start || end >= source.Points.size()) return false;
        ContourBounds bounds = {
            source.Points[start].X, source.Points[start].Y,
            source.Points[start].X, source.Points[start].Y,
            start, end, contourArea(source.Points, start, end)
        };
        for (uint16_t point = start + 1; point <= end; point++) {
            bounds.xMin = std::min(bounds.xMin, source.Points[point].X);
            bounds.yMin = std::min(bounds.yMin, source.Points[point].Y);
            bounds.xMax = std::max(bounds.xMax, source.Points[point].X);
            bounds.yMax = std::max(bounds.yMax, source.Points[point].Y);
        }
        contours.push_back(bounds);
        start = end + 1;
    }
    if (start != source.Points.size()) return false;

    std::vector<size_t> parents(contours.size());
    for (size_t i = 0; i < parents.size(); i++) parents[i] = i;
    for (size_t i = 0; i < contours.size(); i++) {
        for (size_t j = i + 1; j < contours.size(); j++) {
            if (boxesOverlap(contours[i], contours[j])) join(parents, i, j);
        }
    }

    std::map<size_t, ContourGroup> grouped;
    for (size_t i = 0; i < contours.size(); i++) {
        size_t root = findRoot(parents, i);
        auto found = grouped.find(root);
        if (found == grouped.end()) {
            ContourGroup group;
            group.xMin = contours[i].xMin;
            group.yMin = contours[i].yMin;
            group.xMax = contours[i].xMax;
            group.yMax = contours[i].yMax;
            group.area = 0.0;
            found = grouped.insert(std::make_pair(root, group)).first;
        }
        ContourGroup &group = found->second;
        group.contours.push_back(i);
        group.xMin = std::min(group.xMin, contours[i].xMin);
        group.yMin = std::min(group.yMin, contours[i].yMin);
        group.xMax = std::max(group.xMax, contours[i].xMax);
        group.yMax = std::max(group.yMax, contours[i].yMax);
        group.area += contours[i].area;
    }
    if (grouped.size() < 2) return false;

    std::vector<ContourGroup> groups;
    for (auto iter = grouped.begin(); iter != grouped.end(); ++iter) {
        iter->second.area = std::fabs(iter->second.area);
        groups.push_back(iter->second);
    }
    size_t bodyIndex = 0;
    for (size_t i = 1; i < groups.size(); i++) {
        if (groups[i].yMin < groups[bodyIndex].yMin ||
            (groups[i].yMin == groups[bodyIndex].yMin &&
             groups[i].area > groups[bodyIndex].area)) {
            bodyIndex = i;
        }
    }

    const ContourGroup &body = groups[bodyIndex];
    int bodyCenter = (body.xMin + body.xMax) / 2;
    int minimumGap = std::max(1, xHeight * 2 / 100);
    std::vector<size_t> candidates;
    for (size_t i = 0; i < groups.size(); i++) {
        if (i == bodyIndex) continue;
        const ContourGroup &candidate = groups[i];
        int width = candidate.xMax - candidate.xMin;
        int height = candidate.yMax - candidate.yMin;
        int center = (candidate.xMin + candidate.xMax) / 2;
        if (candidate.yMin <= body.yMax + minimumGap) continue;
        if (std::abs(center - bodyCenter) > std::max(xHeight * 3 / 10, (body.xMax - body.xMin) * 3 / 4)) continue;
        if (width > xHeight * 55 / 100 || height > xHeight * 40 / 100) continue;
        if (body.area > 0.0 && candidate.area > body.area * 0.65) continue;
        candidates.push_back(i);
    }
    if (candidates.size() != 1) return false;

    std::vector<uint8_t> removeContour(contours.size(), 0);
    const ContourGroup &dot = groups[candidates[0]];
    for (size_t i = 0; i < dot.contours.size(); i++) {
        removeContour[dot.contours[i]] = 1;
    }

    for (size_t i = 0; i < contours.size(); i++) {
        if (removeContour[i]) continue;
        const ContourBounds &contour = contours[i];
        for (uint16_t point = contour.start; point <= contour.end; point++) {
            OpenType_GlyphPoint copy = source.Points[point];
            // Coordinate compression flags describe the source glyph's delta
            // encoding and must not be reused after contours are removed.
            copy.Flags &= OpenType_FlagOnCurve;
            derived.Points.push_back(copy);
        }
        derived.EndPtsOfContours.push_back((uint16_t)(derived.Points.size() - 1));
    }
    if (derived.EndPtsOfContours.empty()) return false;
    finalizeGlyphBounds(derived);
    derived.Instructions.clear();
    return true;
}

Status PinyinComponents::ResolveDotlessI(PinyinComponentGlyph &component)
{
    if (dotlessIStatus_ == kPinyinDotlessIUnavailable) return kNotSupported;
    if (dotlessIStatus_ == kPinyinDotlessIUnknown) {
        uint16_t sourceIndex = font_.CharToGlyphIndex('i');
        const OpenType_GlyphHeader *header = nullptr;
        if (sourceIndex == 0 ||
            font_.Glyph(sourceIndex, &header) != kOk ||
            header == nullptr ||
            header->NumberOfContours <= 0) {
            dotlessIStatus_ = kPinyinDotlessIUnavailable;
            return kNotSupported;
        }
        const OpenType_GlyphSimple *source = (const OpenType_GlyphSimple*)header;
        OpenType_GlyphSimple derived;
        if (!DeriveDotlessI(*source, xHeight_, derived)) {
            dotlessIStatus_ = kPinyinDotlessIUnavailable;
            return kNotSupported;
        }
        OpenType_LongHorMetric metric = {};
        if (font_.GlyphHorMetric(sourceIndex, metric) != kOk) {
            dotlessIStatus_ = kPinyinDotlessIUnavailable;
            return kError;
        }
        metric.LSB = derived.XMin;
        Status status = addSimpleGlyph(
            derived, metric, "pinyin.dotless_i", dotlessIGlyphIndex_);
        if (status != kOk) {
            dotlessIStatus_ = kPinyinDotlessIUnavailable;
            return status;
        }
        dotlessIStatus_ = kPinyinDotlessIReady;
    }

    const OpenType_GlyphHeader *glyph = nullptr;
    Status status = font_.Glyph(dotlessIGlyphIndex_, &glyph);
    if (status != kOk || glyph == nullptr) return kError;
    component.GlyphIndex = dotlessIGlyphIndex_;
    component.Bounds = { glyph->XMin, glyph->YMin, glyph->XMax, glyph->YMax };
    component.Generated = true;
    dotlessIUseCount_++;
    return kOk;
}
