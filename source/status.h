#ifndef __PINYIN_FONT_STATUS_H__
#define __PINYIN_FONT_STATUS_H__

//------------------------------------------------------------------------------

typedef enum Status {
    kOk = 0,                // OK
    kError = 1,             // generic error
    kInvalidArgs = 2,       // invalid arguments
    kOutOfMemory = 3,       // out of memory
    kNotFound = 4,          // file/entry not found
    kCorruption = 5,        // data corruption
    kNotSupported = 6,      // not supported
} Status;

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_STATUS_H__
