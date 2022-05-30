#include "pinyin_db.h"

//------------------------------------------------------------------------------

PinyinDB::PinyinDB()
{
}

PinyinDB::~PinyinDB()
{
}

Status PinyinDB::Load(const char* dbFile)
{
    if (dbFile == nullptr) {
        __addTestDataset();
        return kOk;
    }

    return kError;
}

size_t PinyinDB::Count() const
{
    return records_.size();
}

void PinyinDB::GetRecord(size_t index, PinyinRecord &record) const
{
    record = records_[index];
}

void PinyinDB::__addTestDataset()
{
    PinyinRecord record;

    record.charcode = 0x6C49;
    record.pinyin0 = L"ha\u0300n";
    records_.push_back(record);

    record.charcode = 0x8BED;
    record.pinyin0 = L"yu\u030C";
    records_.push_back(record);

    record.charcode = 0x62FC;
    record.pinyin0 = L"pi\u0304n";
    records_.push_back(record);

    record.charcode = 0x97F3;
    record.pinyin0 = L"yi\u0304n";
    records_.push_back(record);

    record.charcode = 0x7EFF;
    record.pinyin0 = L"lu\u0308\u0300";
    records_.push_back(record);

    record.charcode = 0x6C34;
    record.pinyin0 = L"shui\u030C";
    records_.push_back(record);

    record.charcode = 0x9752;
    record.pinyin0 = L"qi\u0304ng";
    records_.push_back(record);

    record.charcode = 0x5C71;
    record.pinyin0 = L"sha\u0304n";
    records_.push_back(record);
}
