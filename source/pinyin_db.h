#ifndef __PINYIN_FONT_PINYIN_DB_H__
#define __PINYIN_FONT_PINYIN_DB_H__

#include "status.h"
#include <string>
#include <vector>

//------------------------------------------------------------------------------

class PinyinRecord
{
public:
    wchar_t charcode;
    std::wstring pinyin0;
    std::wstring pinyin1;
    std::wstring pinyin2;
    std::wstring pinyin3;
};

class PinyinDB
{
    std::vector<PinyinRecord> records_;

public:
    PinyinDB();
    ~PinyinDB();

    Status Load(const char* dbFile);

    size_t Count() const;
    void GetRecord(size_t index, PinyinRecord &record) const;

private:
    void __addTestDataset();
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_DB_H__
