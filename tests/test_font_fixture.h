#ifndef PINYIN_FONT_TEST_FONT_FIXTURE_H
#define PINYIN_FONT_TEST_FONT_FIXTURE_H

#include "ot_font.h"
#include <map>
#include <set>

class OpenType_TestFontFixture
{
public:
    static Status Write(
        const char *path,
        const std::set<uint32_t> &extraCharacters,
        const std::set<uint32_t> &toneMarks,
        bool simpleI,
        bool compositeI,
        const std::set<uint32_t> &compositeCharacters =
            std::set<uint32_t>(),
        const std::set<uint32_t> &emptyCharacters =
            std::set<uint32_t>(),
        const std::map<uint32_t, uint32_t> &sharedMappings =
            std::map<uint32_t, uint32_t>(),
        bool addExtremeOutlier = false);
};

#endif
