#ifndef __PINYIN_FONT_PINYIN_FONT_BUILDER_H__
#define __PINYIN_FONT_PINYIN_FONT_BUILDER_H__

#include "status.h"

//------------------------------------------------------------------------------

class PinyinFontBuilder
{
public:
    PinyinFontBuilder();
    ~PinyinFontBuilder();

    Status Build(const char *sourceFont);
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_FONT_BUILDER_H__
