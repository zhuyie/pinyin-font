#include "test_runner.h"

#include <cstdio>
#include <string>
#include <vector>

struct PinyinFontTestCase
{
    const char *Name;
    PinyinFontTestFunction Function;
};

static std::vector<PinyinFontTestCase> &testCases()
{
    static std::vector<PinyinFontTestCase> cases;
    return cases;
}

PinyinFontTestRegistrar::PinyinFontTestRegistrar(
    const char *name,
    PinyinFontTestFunction function)
{
    PinyinFontTestCase testCase = { name, function };
    testCases().push_back(testCase);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s fixture-directory\n", argv[0]);
        return 2;
    }

    const PinyinFontTestContext context = { argv[1] };
    const std::vector<PinyinFontTestCase> &cases = testCases();
    size_t failed = 0;
    for (size_t i = 0; i < cases.size(); i++) {
        std::printf("[ RUN      ] %s\n", cases[i].Name);
        int result = cases[i].Function(context);
        if (result == 0) {
            std::printf("[       OK ] %s\n", cases[i].Name);
        } else {
            std::printf("[  FAILED  ] %s\n", cases[i].Name);
            failed++;
        }
    }

    std::printf(
        "[==========] %zu test(s) ran, %zu failed\n",
        cases.size(), failed);
    return failed == 0 ? 0 : 1;
}
