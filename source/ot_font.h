#ifndef __PINYIN_FONT_OT_FONT_H__
#define __PINYIN_FONT_OT_FONT_H__

#include "status.h"
#include "ot_cmap.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>

//------------------------------------------------------------------------------
#pragma pack(push, 1)

// head - Font Header Table
// https://docs.microsoft.com/en-us/typography/opentype/spec/head
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6head.html
typedef struct OpenType_Head {
    uint32_t Version;            // 0x00010000 for version 1.0.
    uint32_t FontRevision;       // Set by font manufacturer.
    uint32_t CheckSumAdjustment; // --see links--
    uint32_t MagicNumber;        // Set to 0x5F0F3CF5
    uint16_t Flags;              // --see links--
    uint16_t UnitsPerEm;         // Set to a value from 16 to 16384.
    uint64_t Created;            // Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone.
    uint64_t Modified;           // Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone.
    int16_t XMin;                // For all glyph bounding boxes.
    int16_t YMin;                // For all glyph bounding boxes.
    int16_t XMax;                // For all glyph bounding boxes.
    int16_t YMax;                // For all glyph bounding boxes.
    uint16_t MacStyle;           // --see links--
    uint16_t LowestRecPPEM;      // Smallest readable size in pixels.
    int16_t FontDirectionHint;   // --see links--
    int16_t IndexToLocFormat;    // 0 for short offsets (Offset16), 1 for long (Offset32).
    int16_t GlyphDataFormat;     // 0 for current format.
} OpenType_Head;

// maxp - Maximum Profile table.
// https://docs.microsoft.com/en-us/typography/opentype/spec/maxp
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6maxp.html
typedef struct OpenType_Maxp {
    uint32_t Version;    // 0x00005000 for version 0.5, 0x00010000 for version 1.0.
    uint16_t NumGlyphs;  // The number of glyphs in the font.
    // version 1.0 only
    uint16_t MaxPoints;             // Maximum points in a non-composite glyph.
    uint16_t MaxContours;           // Maximum contours in a non-composite glyph.
    uint16_t MaxCompositePoints;    // Maximum points in a composite glyph.
    uint16_t MaxCompositeContours;  // Maximum contours in a composite glyph.
    uint16_t MaxZones;              // 1 if instructions do not use the twilight zone (Z0), or 2 if instructions do use Z0; should be set to 2 in most cases.
    uint16_t MaxTwilightPoints;     // Maximum points used in Z0.
    uint16_t MaxStorage;            // Number of Storage Area locations.
    uint16_t MaxFunctionDefs;       // Number of FDEFs, equal to the highest function number + 1.
    uint16_t MaxInstructionDefs;    // Number of IDEFs.
    uint16_t MaxStackElements;      // Maximum stack depth across Font Program ('fpgm' table), CVT Program ('prep' table) and all glyph instructions (in the 'glyf' table).
    uint16_t MaxSizeOfInstructions; // Maximum byte count for glyph instructions.
    uint16_t MaxComponentElements;  // Maximum number of components referenced at "top level" for any composite glyph.
    uint16_t MaxComponentDepth;     // Maximum levels of recursion; 1 for simple components.
} OpenType_Maxp;

// GlyphHeader - each glyph description begins with this header.
// https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6glyf.html
typedef struct OpenType_GlyphHeader {
    int16_t NumberOfContours;
    int16_t XMin;
    int16_t YMin;
    int16_t XMax;
    int16_t YMax;
} OpenType_GlyphHeader;

// Simple Glyph Flags
#define OpenType_FlagOnCurve              0x01
#define OpenType_FlagXShortVector         0x02
#define OpenType_FlagYShortVector         0x04
#define OpenType_FlagRepeat               0x08
#define OpenType_FlagPositiveXShortVector 0x10
#define OpenType_FlagPositiveYShortVector 0x20
#define OpenType_FlagOverlapSimple        0x40

#define OpenType_FlagXIsSame OpenType_FlagPositiveXShortVector
#define OpenType_FlagYIsSame OpenType_FlagPositiveYShortVector

typedef struct OpenType_GlyphPoint {
    uint8_t Flags;
    int16_t X;
    int16_t Y;
} OpenType_GlyphPoint;

// GlyphSimple - a simple glyph.
typedef struct OpenType_GlyphSimple : OpenType_GlyphHeader {
    std::vector<uint16_t> EndPtsOfContours;
    std::vector<uint8_t> Instructions;
    std::vector<OpenType_GlyphPoint> Points;
} OpenType_GlyphSimple;

// Composite Glyph Flags
#define OpenType_FlagArg1And2AreWords        0x0001
#define OpenType_FlagArgsAreXYValues         0x0002
#define OpenType_FlagRoundXYToGrid           0x0004
#define OpenType_FlagWeHaveAScale            0x0008
#define OpenType_FlagMoreComponents          0x0020
#define OpenType_FlagWeHaveAnXAndYScale      0x0040
#define OpenType_FlagWeHaveATwoByTwo         0x0080
#define OpenType_FlagWeHaveInstructions      0x0100
#define OpenType_FlagUseMyMetrics            0x0200
#define OpenType_FlagOverlapCompound         0x0400
#define OpenType_FlagScaledComponentOffset   0x0800
#define OpenType_FlagUnscaledComponentOffset 0x1000

typedef struct OpenType_GlyphComponent {
    uint16_t Flags;
    uint16_t GlyphIndex;
    int16_t Arg1;
    int16_t Arg2;
    int16_t Transform[4];
} OpenType_GlyphComponent;

// GlyphComposite - a composite glyph.
typedef struct OpenType_GlyphComposite : OpenType_GlyphHeader {
    std::vector<OpenType_GlyphComponent> SubGlyphs;
    std::vector<uint8_t> Instructions;
} OpenType_GlyphComposite;

// post - PostScript Table.
// https://docs.microsoft.com/en-us/typography/opentype/spec/post
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6post.html
typedef struct OpenType_Post {
    uint32_t Version;       // In Version16Dot16, eg. 0x00025000 for version 2.5.
    uint32_t ItalicAngle;   // Italic angle in counter-clockwise degrees from the vertical.
    int16_t UnderlinePosition;  // Suggested distance of the top of the underline from the baseline (negative values indicate below baseline).
    int16_t UnderlineThickness; // Suggested values for the underline thickness.
    uint32_t IsFixedPitch;  // Set to 0 if the font is proportionally spaced, non-zero if the font is not proportionally spaced (i.e. monospaced).
    uint32_t MinMemType42;  // Minimum memory usage when an OpenType font is downloaded.
    uint32_t MaxMemType42;  // Maximum memory usage when an OpenType font is downloaded.
    uint32_t MinMemType1;   // Minimum memory usage when an OpenType font is downloaded as a Type 1 font.
    uint32_t MaxMemType1;   // Maximum memory usage when an OpenType font is downloaded as a Type 1 font.
    // version 2.0 only
} OpenType_Post;

// OS/2 - OS/2 and Windows Metrics Table
// https://docs.microsoft.com/en-us/typography/opentype/spec/os2
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6OS2.html
typedef struct OpenType_OS2 {
    uint16_t version;
    int16_t  xAvgCharWidth;
    uint16_t usWeightClass;
    uint16_t usWidthClass;
    uint16_t fsType;
    int16_t  ySubscriptXSize;
    int16_t  ySubscriptYSize;
    int16_t  ySubscriptXOffset;
    int16_t  ySubscriptYOffset;
    int16_t  ySuperscriptXSize;
    int16_t  ySuperscriptYSize;
    int16_t  ySuperscriptXOffset;
    int16_t  ySuperscriptYOffset;
    int16_t  yStrikeoutSize;
    int16_t  yStrikeoutPosition;
    int16_t  sFamilyClass;
    uint8_t  panose[10];
    uint32_t ulUnicodeRange1;
    uint32_t ulUnicodeRange2;
    uint32_t ulUnicodeRange3;
    uint32_t ulUnicodeRange4;
    uint32_t achVendID;
    uint16_t fsSelection;
    uint16_t usFirstCharIndex;
    uint16_t usLastCharIndex;
    int16_t  sTypoAscender;
    int16_t  sTypoDescender;
    int16_t  sTypoLineGap;
    uint16_t usWinAscent;
    uint16_t usWinDescent;
    // version >= 1
    uint32_t ulCodePageRange1;
    uint32_t ulCodePageRange2;
    // version >= 2
    int16_t  sxHeight;
    int16_t  sCapHeight;
    uint16_t usDefaultChar;
    uint16_t usBreakChar;
    uint16_t usMaxContext;
    // version >= 5
    uint16_t usLowerOpticalPointSize;
    uint16_t usUpperOpticalPointSize;
} OpenType_OS2;

// hhea - Horizontal Header Table
// https://docs.microsoft.com/en-us/typography/opentype/spec/hhea
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6hhea.html
typedef struct OpenType_Hhea {
    uint16_t MajorVersion;         // set to 1.
    uint16_t MinorVersion;         // set to 0.
    int16_t  Ascender;             // Typographic ascent.
    int16_t  Descender;            // Typographic descent.
    int16_t  LineGap;              // Typographic line gap.
    uint16_t AdvanceWidthMax;      // Maximum advance width value in 'hmtx' table.
    int16_t  MinLeftSideBearing;   // Minimum left sidebearing value in 'hmtx' table.
    int16_t  MinRightSideBearing;  // Minimum right sidebearing value; calculated as min(aw - (lsb + xMax - xMin)).
    int16_t  XMaxExtent;           // Max(lsb + (xMax - xMin)).
    int16_t  CaretSlopeRise;       // Used to calculate the slope of the cursor (rise/run); 1 for vertical.
    int16_t  CaretSlopeRun;        // 0 for vertical.
    int16_t  CaretOffset;          // The amount by which a slanted highlight on a glyph needs to be shifted to produce the best appearance.
    int16_t  Reserved0;            // set to 0.
    int16_t  Reserved1;            // set to 0.
    int16_t  Reserved2;            // set to 0.
    int16_t  Reserved3;            // set to 0.
    int16_t  MetricDataFormat;     // 0 for current format.
    uint16_t NumberOfHMetrics;     // Number of hMetric entries in 'hmtx' table
} OpenType_Hhea;

// The table hmtx use a longHorMetric record to give the advance width and left side bearing of a glyph.
// https://docs.microsoft.com/en-us/typography/opentype/spec/hmtx
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6hmtx.html
typedef struct OpenType_LongHorMetric {
    uint16_t AdvanceWidth;         // Advance width, in font design units.
    int16_t  LSB;                  // Glyph left side bearing, in font design units.
} OpenType_LongHorMetric;

// Each entry in the name table is referenced by a name record.
// https://docs.microsoft.com/en-us/typography/opentype/spec/name
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6name.html
typedef struct OpenType_NameRecord {
    uint16_t PlatformID;           // Platform ID.
    uint16_t EncodingID;           // Platform-specific encoding ID.
    uint16_t LanguageID;           // Language ID.
    uint16_t NameID;               // Name ID.
    std::wstring String;           // The string data.
} OpenType_NameRecord;

#pragma pack(pop)
//------------------------------------------------------------------------------

class OpenType_Font
{
    friend class OpenType_Font_Parser;
    friend class OpenType_Font_Writer;

public:
    OpenType_Font();
    ~OpenType_Font();

    const OpenType_Head& Head() const { return head_; }
    const OpenType_Maxp& Maxp() const { return maxp_; }
    const OpenType_Post& Post() const { return post_; }
    const OpenType_OS2&  OS2()  const { return os2_;  }
    const OpenType_Hhea& Hhea() const { return hhea_; }

    int GlyphCount() const;
    Status Glyph(int index, const OpenType_GlyphHeader **ppGlyph) const;
    Status GlyphName(int index, std::string &name) const;
    Status GlyphHorMetric(int index, OpenType_LongHorMetric &metric) const;

    uint16_t CharToGlyphIndex(uint32_t charcode) const;

    Status Name(uint16_t nameID, std::vector<OpenType_NameRecord> &records) const;

    void Clear();
    Status AddGlyph(const OpenType_GlyphHeader *glyph, const OpenType_LongHorMetric *mtx, const char* name);

private:
    OpenType_Head head_;
    OpenType_Maxp maxp_;
    OpenType_Post post_;
    OpenType_OS2  os2_;
    OpenType_Hhea hhea_;
    std::vector<OpenType_LongHorMetric> hmtx_;
    std::vector<OpenType_GlyphHeader*> glyphs_;
    std::vector<std::string> glyphNames_;
    std::vector<CmapSequentialMapGroup> char2index_;
    std::multimap<uint16_t, OpenType_NameRecord> names_;
};

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_OT_FONT_H__
