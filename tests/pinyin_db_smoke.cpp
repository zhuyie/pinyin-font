#include "pinyin_db.h"
#include <cstdio>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s pinyin-db\n", argv[0]);
        return 1;
    }

    PinyinDB db;
    Status status = db.Load(argv[1]);
    if (status != kOk) {
        std::fprintf(stderr, "failed to load pinyin db: %d\n", status);
        return 1;
    }

    if (db.Count() == 0) {
        std::fprintf(stderr, "pinyin db is empty\n");
        return 1;
    }

    PinyinRecord record;
    db.GetRecord(0, record);
    if (record.charcode == 0 || record.pinyin[0].empty()) {
        std::fprintf(stderr, "first pinyin record is invalid\n");
        return 1;
    }

    bool foundM = false;
    bool foundN = false;
    for (size_t i = 0; i < db.Count(); i++) {
        db.GetRecord(i, record);
        if (record.charcode == 0x5452) {
            foundM = record.pinyin[0] == L"m\x0301";
        } else if (record.charcode == 0x55EF) {
            foundN =
                record.pinyin[0] == L"n\x0301g" &&
                record.pinyin[1] == L"n\x030Cg" &&
                record.pinyin[2] == L"n\x0300g";
        }
    }
    if (!foundM || !foundN) {
        std::fprintf(stderr, "exceptional syllabic forms were not normalized\n");
        return 1;
    }

    return 0;
}
