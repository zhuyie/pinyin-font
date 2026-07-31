#include "ot_cmap_encoder.h"
#include "utility.h"
#include <algorithm>
#include <limits>

namespace {

struct Format4Segment {
    uint16_t startCode;
    uint16_t endCode;
    int16_t idDelta;
    std::vector<uint16_t> glyphIds;

    bool UsesGlyphArray() const { return !glyphIds.empty(); }
    uint32_t EncodedCost() const {
        return UsesGlyphArray()
            ? 8 + (uint32_t)glyphIds.size() * 2
            : 8;
    }
};

static void appendDeltaSegments(
    uint32_t start,
    uint32_t end,
    const std::vector<uint16_t> &glyphs,
    std::vector<Format4Segment> &segments)
{
    uint32_t segmentStart = start;
    uint16_t previousGlyph = glyphs[start];
    for (uint32_t code = start + 1; code <= end; code++) {
        uint16_t expected = (uint16_t)(previousGlyph + 1);
        if (glyphs[code] != expected) {
            Format4Segment segment = {};
            segment.startCode = (uint16_t)segmentStart;
            segment.endCode = (uint16_t)(code - 1);
            segment.idDelta =
                (int16_t)(glyphs[segmentStart] - segmentStart);
            segments.push_back(segment);
            segmentStart = code;
        }
        previousGlyph = glyphs[code];
    }

    Format4Segment segment = {};
    segment.startCode = (uint16_t)segmentStart;
    segment.endCode = (uint16_t)end;
    segment.idDelta = (int16_t)(glyphs[segmentStart] - segmentStart);
    segments.push_back(segment);
}

static Status planSegments(
    const std::vector<CmapSequentialMapGroup> &groups,
    std::vector<Format4Segment> &segments)
{
    const uint32_t bmpLimit = 0xFFFF;
    std::vector<uint16_t> glyphs(bmpLimit, 0);
    std::vector<uint8_t> mapped(bmpLimit, 0);

    uint32_t previousEnd = 0;
    bool havePrevious = false;
    for (size_t i = 0; i < groups.size(); i++) {
        const CmapSequentialMapGroup &group = groups[i];
        if (group.endCharCode < group.startCharCode ||
            (havePrevious && group.startCharCode <= previousEnd)) {
            return kInvalidArgs;
        }
        previousEnd = group.endCharCode;
        havePrevious = true;
        if (group.startCharCode >= bmpLimit) {
            continue;
        }
        uint32_t end = std::min(group.endCharCode, bmpLimit - 1);
        for (uint32_t code = group.startCharCode; code <= end; code++) {
            uint64_t glyph =
                (uint64_t)group.startGlyphID + code - group.startCharCode;
            if (glyph > std::numeric_limits<uint16_t>::max()) {
                return kInvalidArgs;
            }
            glyphs[code] = (uint16_t)glyph;
            mapped[code] = 1;
        }
    }

    uint32_t code = 0;
    while (code < bmpLimit) {
        while (code < bmpLimit && !mapped[code]) code++;
        if (code == bmpLimit) break;
        uint32_t runStart = code;
        while (code + 1 < bmpLimit && mapped[code + 1]) code++;
        uint32_t runEnd = code;

        std::vector<Format4Segment> deltaSegments;
        appendDeltaSegments(runStart, runEnd, glyphs, deltaSegments);
        uint32_t runLength = runEnd - runStart + 1;
        uint32_t deltaCost = (uint32_t)deltaSegments.size() * 8;
        uint32_t arrayCost = 8 + runLength * 2;
        if (arrayCost < deltaCost) {
            Format4Segment segment = {};
            segment.startCode = (uint16_t)runStart;
            segment.endCode = (uint16_t)runEnd;
            segment.idDelta = 0;
            segment.glyphIds.insert(
                segment.glyphIds.end(),
                glyphs.begin() + runStart,
                glyphs.begin() + runEnd + 1);
            segments.push_back(segment);
        } else {
            segments.insert(
                segments.end(), deltaSegments.begin(), deltaSegments.end());
        }
        code++;
    }

    std::vector<Format4Segment> compacted;
    for (size_t i = 0; i < segments.size(); i++) {
        compacted.push_back(segments[i]);
        while (compacted.size() >= 2) {
            const Format4Segment &left =
                compacted[compacted.size() - 2];
            const Format4Segment &right = compacted.back();
            uint32_t span =
                (uint32_t)right.endCode - left.startCode + 1;
            uint32_t mergedCost = 8 + span * 2;
            if (mergedCost >= left.EncodedCost() + right.EncodedCost()) {
                break;
            }

            Format4Segment merged = {};
            merged.startCode = left.startCode;
            merged.endCode = right.endCode;
            merged.glyphIds.reserve(span);
            for (uint32_t mergedCode = merged.startCode;
                 mergedCode <= merged.endCode; mergedCode++) {
                merged.glyphIds.push_back(
                    mapped[mergedCode] ? glyphs[mergedCode] : 0);
            }
            compacted.pop_back();
            compacted.back() = merged;
        }
    }
    segments.swap(compacted);

    Format4Segment sentinel = {};
    sentinel.startCode = 0xFFFF;
    sentinel.endCode = 0xFFFF;
    sentinel.idDelta = 1;
    segments.push_back(sentinel);
    return kOk;
}

} // namespace

Status EncodeCmapFormat4(
    const std::vector<CmapSequentialMapGroup> &groups,
    std::vector<uint8_t> &output,
    CmapFormat4Stats *stats)
{
    output.clear();
    std::vector<Format4Segment> segments;
    Status status = planSegments(groups, segments);
    if (status != kOk) return status;

    if (segments.empty() ||
        segments.size() > std::numeric_limits<uint16_t>::max() / 2) {
        return kNotSupported;
    }

    uint32_t glyphIdCount = 0;
    for (size_t i = 0; i < segments.size(); i++) {
        glyphIdCount += (uint32_t)segments[i].glyphIds.size();
    }
    uint32_t length =
        16 + (uint32_t)segments.size() * 8 + glyphIdCount * 2;
    if (length > std::numeric_limits<uint16_t>::max()) {
        return kNotSupported;
    }

    uint16_t segCount = (uint16_t)segments.size();
    uint16_t entrySelector = 0;
    while ((uint32_t(1) << (entrySelector + 1)) <= segCount) {
        entrySelector++;
    }
    uint16_t searchRange = (uint16_t)((uint32_t(1) << entrySelector) * 2);
    uint16_t rangeShift = (uint16_t)(segCount * 2 - searchRange);

    output.resize(length, 0);
    uint8_t *base = &output[0];
    put_u2(base + 0, 4);
    put_u2(base + 2, (uint16_t)length);
    put_u2(base + 4, 0);
    put_u2(base + 6, (uint16_t)(segCount * 2));
    put_u2(base + 8, searchRange);
    put_u2(base + 10, entrySelector);
    put_u2(base + 12, rangeShift);

    uint8_t *endCodes = base + 14;
    uint8_t *startCodes = endCodes + segCount * 2 + 2;
    uint8_t *idDeltas = startCodes + segCount * 2;
    uint8_t *idRangeOffsets = idDeltas + segCount * 2;
    uint8_t *glyphIdArray = idRangeOffsets + segCount * 2;
    uint32_t glyphOffset = 0;

    for (uint16_t i = 0; i < segCount; i++) {
        const Format4Segment &segment = segments[i];
        put_u2(endCodes + i * 2, segment.endCode);
        put_u2(startCodes + i * 2, segment.startCode);
        put_i2(idDeltas + i * 2, segment.idDelta);
        if (segment.UsesGlyphArray()) {
            uint32_t wordOffset =
                (uint32_t)(segCount - i) + glyphOffset;
            uint32_t byteOffset = wordOffset * 2;
            if (byteOffset > std::numeric_limits<uint16_t>::max()) {
                output.clear();
                return kNotSupported;
            }
            put_u2(idRangeOffsets + i * 2, (uint16_t)byteOffset);
            for (size_t g = 0; g < segment.glyphIds.size(); g++) {
                put_u2(
                    glyphIdArray + (glyphOffset + g) * 2,
                    segment.glyphIds[g]);
            }
            glyphOffset += (uint32_t)segment.glyphIds.size();
        }
    }

    if (stats != NULL) {
        stats->segmentCount = segCount;
        stats->glyphIdCount = glyphIdCount;
        stats->length = length;
    }
    return kOk;
}
