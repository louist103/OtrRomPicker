#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#include <immintrin.h>
#elif defined(__GNUC__) && defined(__x86_64__) || defined(__i386__)
#include <nmmintrin.h>
#else
#error Not supported yet
#endif

#define INTRIN_CRC32_64(crc, data) _mm_crc32_u64(crc, data)
#define INTRIN_CRC32_32(crc, data) _mm_crc32_u32(crc, data)
#define INTRIN_CRC32_16(crc, data) _mm_crc32_u16(crc, data)
#define INTRIN_CRC32_8(crc, data) _mm_crc32_u8(crc, data)

uint32_t CRC32C(unsigned char* data, size_t dataSize) {
    uint32_t ret = -1;
    int64_t sizeSigned = dataSize;

#if defined(_M_X64) || defined(__x86_64__)
    while ((sizeSigned -= sizeof(uint64_t)) >= 0) {
        ret = (uint32_t)_mm_crc32_u64(ret, *(uint64_t*)data);
        data += sizeof(uint64_t);
    }

    if (sizeSigned & sizeof(uint32_t)) {
        ret = _mm_crc32_u32(ret, *(uint32_t*)data);
        data += sizeof(uint32_t);
    }
#elif defined(_M_IX86) || defined(__i386__)
    while ((sizeSigned -= sizeof(uint32_t)) >= 0) {
        ret = (uint32_t)_mm_crc32_u32(ret, *(uint32_t*)data);
        data += sizeof(uint32_t);
    }
#endif
    if (sizeSigned & sizeof(uint16_t)) {
        ret = _mm_crc32_u16(ret, *(uint16_t*)data);
        data += sizeof(uint16_t);
    }

    if (sizeSigned & sizeof(uint8_t)) {
        ret = _mm_crc32_u8(ret, *data);
    }

    return ~ret;
}