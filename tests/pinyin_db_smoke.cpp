#include "pinyin_db.h"
#include "test_runner.h"
#include <cstdio>
#include <string>

static bool writeDB(const std::string &path)
{
    FILE *file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const char *contents =
        "一\t4E00\tyī\n"
        "呒\t5452\tḿ\n"
        "嗯\t55EF\tńg,ňg,ǹg\n";
    bool ok = std::fputs(contents, file) >= 0;
    std::fclose(file);
    return ok;
}

PINYINFONT_TEST(pinyin_db)
{
    std::string databasePath =
        std::string(context.FixtureDirectory) + "/pinyin-db-smoke.txt";
    if (!writeDB(databasePath)) {
        std::fprintf(stderr, "failed to create pinyin db fixture\n");
        return 1;
    }

    PinyinDB db;
    Status status = db.Load(databasePath.c_str());
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
