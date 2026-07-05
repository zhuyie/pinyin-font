#ifndef __PINYIN_FONT_UTILITY_H__
#define __PINYIN_FONT_UTILITY_H__

#include <cstdint>

//------------------------------------------------------------------------------

inline uint8_t u1(const uint8_t *b)
{
    return b[0];
}

inline uint16_t u2(const uint8_t *b)
{
    return ((uint16_t)b[0] << 8) | (uint16_t)b[1];
}

inline int16_t i2(const uint8_t *b)
{
    return (int16_t)u2(b);
}

inline uint32_t u4(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

inline int32_t i4(const uint8_t *b)
{
    return (int32_t)u4(b);
}

inline uint64_t u8(const uint8_t *b)
{
    return ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32) | 
           ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) | ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);
}

inline int64_t i8(const uint8_t *b)
{
    return (int64_t)u8(b);
}

inline void put_u1(uint8_t *b, uint8_t v)
{
    b[0] = v;
}

inline void put_u2(uint8_t *b, uint16_t v)
{
    b[0] = (uint8_t)(v >> 8);
    b[1] = (uint8_t)(v);
}

inline void put_i2(uint8_t *b, int16_t v)
{
    put_u2(b, (uint16_t)v);
}

inline void put_u4(uint8_t *b, uint32_t v)
{
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)(v);
}

inline void put_i4(uint8_t *b, int32_t v)
{
    put_u4(b, (uint32_t)v);
}

inline void put_u8(uint8_t *b, uint64_t v)
{
    b[0] = (uint8_t)(v >> 56);
    b[1] = (uint8_t)(v >> 48);
    b[2] = (uint8_t)(v >> 40);
    b[3] = (uint8_t)(v >> 32);
    b[4] = (uint8_t)(v >> 24);
    b[5] = (uint8_t)(v >> 16);
    b[6] = (uint8_t)(v >> 8);
    b[7] = (uint8_t)(v);
}

inline void put_i8(uint8_t *b, int64_t v)
{
    put_u8(b, (uint64_t)v);
}

//------------------------------------------------------------------------------

#endif // !__PINYIN_FONT_UTILITY_H__
