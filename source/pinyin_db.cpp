#include "pinyin_db.h"
#include <algorithm>
#include <fstream>
#include <locale>
#include <codecvt>
#include <cwchar>

//------------------------------------------------------------------------------

PinyinDB::PinyinDB()
{
}

PinyinDB::~PinyinDB()
{
}

Status PinyinDB::Load(const char* dbFile)
{
    if (dbFile == nullptr || dbFile[0] == '\0') {
        __addTestDataset();
        return kOk;
    }

    std::wifstream file(dbFile);
    if (!file.is_open()) {
        return kFileError;
    }
    file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));

    std::wstring line, hanzi, code, pinyins;
    PinyinRecord record;
    while (std::getline(file, line)) {
        // format: hanzi \t code \t pinyins
        // 伟	4F1F	wěi
        // 传	4F20	chuán,zhuàn
        size_t tab0 = line.find(L'\t', 0);
        if (tab0 == std::wstring::npos)
            continue;
        size_t tab1 = line.find(L'\t', tab0 + 1);
        if (tab1 == std::wstring::npos)
            continue;
        hanzi   = line.substr(0, tab0);
        code    = line.substr(tab0 + 1, tab1 - tab0 - 1);
        pinyins = line.substr(tab1 + 1);

        unsigned int x = std::wcstoul(code.c_str(), nullptr, 16);
        if (x == 0)
            continue;
        if (x >= 0x10000)  // TODO
            continue;

        record.charcode = x;
        if (!__extractPinyins(pinyins, record))
            continue;
        records_.push_back(record);
    }

    __sort();

    return kOk;
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
    record.pinyin[0] = L"ha\u0300n";
    records_.push_back(record);

    record.charcode = 0x8BED;
    record.pinyin[0] = L"yu\u030C";
    records_.push_back(record);

    record.charcode = 0x62FC;
    record.pinyin[0] = L"pi\u0304n";
    records_.push_back(record);

    record.charcode = 0x97F3;
    record.pinyin[0] = L"yi\u0304n";
    records_.push_back(record);

    record.charcode = 0x7EFF;
    record.pinyin[0] = L"lu\u0308\u0300";
    records_.push_back(record);

    record.charcode = 0x6C34;
    record.pinyin[0] = L"shui\u030C";
    records_.push_back(record);

    record.charcode = 0x9752;
    record.pinyin[0] = L"qi\u0304ng";
    records_.push_back(record);

    record.charcode = 0x5C71;
    record.pinyin[0] = L"sha\u0304n";
    records_.push_back(record);

    __sort();
}

void PinyinDB::__sort()
{
    std::sort(
        records_.begin(), records_.end(), 
        [](const PinyinRecord &a, const PinyinRecord &b) -> bool {
            return a.charcode < b.charcode;
        }
    );
}

bool PinyinDB::__extractPinyins(const std::wstring &pinyins, PinyinRecord &record)
{
    record.pinyin[0].clear();
    record.pinyin[1].clear();
    record.pinyin[2].clear();
    record.pinyin[3].clear();

    int count = 0;
    size_t start = 0, end = pinyins.length(), tab;
    while (start < end && count < 4) {
        tab = pinyins.find(L',', start);
        if (tab != std::wstring::npos) {
            record.pinyin[count] = pinyins.substr(start, tab - start);
            start = tab + 1;
        } else {
            record.pinyin[count] = pinyins.substr(start, end - start);
            start = end;
        }
        __normalize(record.pinyin[count]);
        count++;
    }

    return !record.pinyin[0].empty();
}

void PinyinDB::__normalize(std::wstring &pinyin)
{
    static const wchar_t* d[][2] = {
        { L"\x0101", L"a\x0304" },  // ā
        { L"\x00E1", L"a\x0301" },  // á
        { L"\x01CE", L"a\x030C" },  // ǎ
        { L"\x00E0", L"a\x0300" },  // à

        { L"\x0113", L"e\x0304" },  // ē
        { L"\x00E9", L"e\x0301" },  // é
        { L"\x011B", L"e\x030C" },  // ě
        { L"\x00E8", L"e\x0300" },  // è

        { L"\x012B", L"i\x0304" },  // ī
        { L"\x00ED", L"i\x0301" },  // í
        { L"\x01D0", L"i\x030C" },  // ǐ
        { L"\x00EC", L"i\x0300" },  // ì

        { L"\x014D", L"o\x0304" },  // ō
        { L"\x00F3", L"o\x0301" },  // ó
        { L"\x01D2", L"o\x030C" },  // ǒ
        { L"\x00F2", L"o\x0300" },  // ò

        { L"\x016B", L"u\x0304" },  // ū
        { L"\x00FA", L"u\x0301" },  // ú
        { L"\x01D4", L"u\x030C" },  // ǔ
        { L"\x00F9", L"u\x0300" },  // ù

        { L"\x00FC", L"u\x0308" },  // ü
        { L"\x01D6", L"u\x0308\x0304" },  // ǖ
        { L"\x01D8", L"u\x0308\x0301" },  // ǘ
        { L"\x01DA", L"u\x0308\x030C" },  // ǚ
        { L"\x01DC", L"u\x0308\x0300" },  // ù
    };
    static const int dcount = sizeof(d) / sizeof(d[0]);

    for (size_t i = 0; i < pinyin.size(); i++) {
        wchar_t c = pinyin[i];
        for (int j = 0; j < dcount; j++) {
            if (c == d[j][0][0]) {
                pinyin.replace(i, 1, d[j][1]);
                i += wcslen(d[j][1]) - 1;
                break;
            }
        }
    }
}
