#include "pinyin_db.h"
#include "pinyin_font_builder.h"
#include <cstdio>
#include <cstring>
#include <string>

//------------------------------------------------------------------------------

static void printUsage(const char *program)
{
    std::fprintf(stdout,
        "usage: %s --input <font.ttf> --pinyin-db <pinyin-db.txt> [--output <out.ttf>]\n",
        program);
}

static bool isOption(const char *arg, const char *name)
{
    return std::strcmp(arg, name) == 0;
}

static bool parseArgs(
    int argc,
    char *argv[],
    const char *&input,
    const char *&pinyinDB,
    const char *&output)
{
    input = nullptr;
    pinyinDB = nullptr;
    output = nullptr;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (isOption(arg, "--help") || isOption(arg, "-h")) {
            return false;
        }
        if (i + 1 >= argc) {
            std::fprintf(stderr, "missing value for %s\n", arg);
            return false;
        }
        if (isOption(arg, "--input")) {
            input = argv[++i];
        } else if (isOption(arg, "--pinyin-db")) {
            pinyinDB = argv[++i];
        } else if (isOption(arg, "--output")) {
            output = argv[++i];
        } else {
            std::fprintf(stderr, "unknown option: %s\n", arg);
            return false;
        }
    }

    if (input == nullptr) {
        std::fprintf(stderr, "missing required option: --input\n");
        return false;
    }
    if (pinyinDB == nullptr) {
        std::fprintf(stderr, "missing required option: --pinyin-db\n");
        return false;
    }
    return true;
}

static int buildFont(const char *filename, const char *dbFile, const char *outputFile)
{
    std::string derivedOutput;
    if (outputFile == nullptr) {
        derivedOutput = filename;
        derivedOutput += ".pinyin.ttf";
        outputFile = derivedOutput.c_str();
    }

    PinyinDB db;
    std::fprintf(stdout, "Input = %s\n", filename);
    std::fprintf(stdout, "PinyinDB = %s\n", dbFile);
    std::fprintf(stdout, "Output = %s\n", outputFile);
    Status status = db.Load(dbFile);
    if (status != kOk) {
        std::fprintf(stderr, "Load PinyinDB failed, error=%d\n", status);
        return 1;
    }
    std::fprintf(stdout, "PinyinDB loaded, records = %u\n", (unsigned int)db.Count());
    std::fprintf(stdout, "\n");

    PinyinFontBuilder builder;
    status = builder.Build(filename, outputFile, db);
    if (status != kOk) {
        std::fprintf(stderr, "Build failed, error=%d\n", status);
        return 1;
    }
    std::fprintf(stdout, "Build succeeded\n");

    uint16_t glyphCountOld, glyphCountAddOK, glyphCountAddFailed;
    uint32_t parseTime, synthesisTime, writeTime;
    builder.GetStats(
        glyphCountOld, glyphCountAddOK, glyphCountAddFailed,
        parseTime, synthesisTime, writeTime);
    std::fprintf(stdout, "  GlyphCountOld = %d\n", (int)glyphCountOld);
    std::fprintf(stdout, "          AddOK = %d\n", (int)glyphCountAddOK);
    std::fprintf(stdout, "      AddFailed = %d\n", (int)glyphCountAddFailed);
    std::fprintf(stdout, "      ParseTime = %.2fms\n", parseTime / 1000.0);
    std::fprintf(stdout, "  SynthesisTime = %.2fms\n", synthesisTime / 1000.0);
    std::fprintf(stdout, "      WriteTime = %.2fms\n", writeTime / 1000.0);

    std::fprintf(stdout, "\n");
    return 0;
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    const char *input = nullptr;
    const char *pinyinDB = nullptr;
    const char *output = nullptr;

    if (!parseArgs(argc, argv, input, pinyinDB, output)) {
        printUsage(argv[0]);
        return 1;
    }

    return buildFont(input, pinyinDB, output);
}
