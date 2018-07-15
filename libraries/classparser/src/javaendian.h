#pragma once
#include <stdint.h>

/**
 * Swap bytes between big endian and local number representation
 */
namespace util
{
#ifdef MULTIMC_BIG_ENDIAN
inline uint64_t bigswap(uint64_t x)
{
    return x;
}
;
inline uint32_t bigswap(uint32_t x)
{
    return x;
}
;
inline uint16_t bigswap(uint16_t x)
{
    return x;
}
;
inline int64_t bigswap(int64_t x)
{
    return x;
}
;
inline int32_t bigswap(int32_t x)
{
    return x;
}
;
inline int16_t bigswap(int16_t x)
{
    return x;
}
;
#else
inline uint64_t bigswap(uint64_t x)
{
    return (x >> 56) | ((x << 40) & 0x00FF000000000000) | ((x << 24) & 0x0000FF0000000000) |
           ((x << 8) & 0x000000FF00000000) | ((x >> 8) & 0x00000000FF000000) |
           ((x >> 24) & 0x0000000000FF0000) | ((x >> 40) & 0x000000000000FF00) | (x << 56);
}

inline uint32_t bigswap(uint32_t x)
{
    return (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x << 24);
}

inline uint16_t bigswap(uint16_t x)
{
    return (x >> 8) | (x << 8);
}

inline int64_t bigswap(int64_t x)
{
    return (x >> 56) | ((x << 40) & 0x00FF000000000000) | ((x << 24) & 0x0000FF0000000000) |
           ((x << 8) & 0x000000FF00000000) | ((x >> 8) & 0x00000000FF000000) |
           ((x >> 24) & 0x0000000000FF0000) | ((x >> 40) & 0x000000000000FF00) | (x << 56);
}

inline int32_t bigswap(int32_t x)
{
    return (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x << 24);
}

inline int16_t bigswap(int16_t x)
{
    return (x >> 8) | (x << 8);
}

#endif
}
