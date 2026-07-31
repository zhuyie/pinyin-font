#ifndef PINYIN_FONT_TEST_RUNNER_H
#define PINYIN_FONT_TEST_RUNNER_H

struct PinyinFontTestContext
{
    const char *FixtureDirectory;
};

typedef int (*PinyinFontTestFunction)(const PinyinFontTestContext &context);

class PinyinFontTestRegistrar
{
public:
    PinyinFontTestRegistrar(
        const char *name,
        PinyinFontTestFunction function);
};

#define PINYINFONT_TEST(name) \
    static int name(const PinyinFontTestContext &context); \
    static PinyinFontTestRegistrar name##_registrar(#name, name); \
    static int name(const PinyinFontTestContext &context)

#endif
