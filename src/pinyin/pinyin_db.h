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
    std::wstring pinyin[4];
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
    void __sort();
    bool __extractPinyins(const std::wstring &pinyins, PinyinRecord &record);
    void __normalize(std::wstring &pinyin);
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_PINYIN_DB_H__
