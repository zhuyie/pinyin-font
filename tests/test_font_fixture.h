#ifndef PINYIN_FONT_TEST_FONT_FIXTURE_H
#define PINYIN_FONT_TEST_FONT_FIXTURE_H

#include "ot_font.h"
#include <set>

class OpenType_TestFontFixture
{
public:
    static Status Write(
        const char *path,
        const std::set<uint32_t> &extraCharacters,
        const std::set<uint32_t> &toneMarks,
        bool simpleI,
        bool compositeI);
};

#endif
