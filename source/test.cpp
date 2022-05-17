#include "ot_font_parser.h"
#include <cstdio>

int main(int argc, char* argv[])
{
    const char *filename = "input.ttf";
    if (argc >= 2) {
        filename = argv[1];
    }
    fprintf(stdout, "filename = %s\n", filename);

    OpenType_Font font;
    OpenType_Font_Parser parser;
    Status status = parser.Parse(filename, &font);
    if (status != kOk) {
        fprintf(stderr, "Parse failed, error=%d\n", status);
        return 1;
    }
    fprintf(stdout, "Parse succeed\n");
    fprintf(stdout, "\n");
    
    fprintf(stdout, "GlyphCount = %d\n", font.GlyphCount());

    return 0;
}
