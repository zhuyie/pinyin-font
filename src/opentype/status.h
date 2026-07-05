#ifndef __PINYIN_FONT_STATUS_H__
#define __PINYIN_FONT_STATUS_H__

//------------------------------------------------------------------------------

typedef enum Status {
    kOk = 0,                // OK
    kError = 1,             // generic error
    kInvalidArgs = 2,       // invalid arguments
    kOutOfMemory = 3,       // out of memory
    kFileError = 4,         // file system related errors
    kNotFound = 5,          // entry not found
    kCorruption = 6,        // data corruption
    kNotSupported = 7,      // not supported
} Status;

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_STATUS_H__
