#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#include <immintrin.h>
#elif defined(__GNUC__) && defined(__x86_64__) || defined(__i386__)
#include <nmmintrin.h>
#elif defined (__aarch64__)
//Nothing cause its a compiler builtin
#else
#error Not supported yet
#endif

#if defined(__aarch64__)
#define INTRIN_CRC32_64(crc, value, ret)  __asm__("crc32cx %w[c], %w[c], %x[v]":[c]"+r"(crc):[v]"r"(value))
#define INTRIN_CRC32_32(crc, value, ret)  __asm__("crc32cw %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define INTRIN_CRC32_16(crc, value, ret)  __asm__("crc32ch %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define INTRIN_CRC32_8(crc, value, ret)  __asm__("crc32cb %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#else
#define INTRIN_CRC32_64(crc, data, ret) ret =  _mm_crc32_u64(crc, data)
#define INTRIN_CRC32_32(crc, data, ret) ret = _mm_crc32_u32(crc, data)
#define INTRIN_CRC32_16(crc, data, ret) ret = _mm_crc32_u16(crc, data)
#define INTRIN_CRC32_8(crc, data, ret) ret = _mm_crc32_u8(crc, data)
#endif

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CRC32C(unsigned char* data, size_t dataSize) {
    uint32_t ret = 0xFFFFFFFF;
    int64_t sizeSigned = dataSize;

#if defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__)
    while ((sizeSigned -= sizeof(uint64_t)) >= 0) {
        INTRIN_CRC32_64(ret, *(uint64_t*)data, ret);
        data += sizeof(uint64_t);
    }

    if (sizeSigned & sizeof(uint32_t)) {
        INTRIN_CRC32_32(ret, *(uint32_t*)data, ret);

        data += sizeof(uint32_t);
    }
#elif defined(_M_IX86) || defined(__i386__)
    while ((sizeSigned -= sizeof(uint32_t)) >= 0) {
        INTRIN_CRC32_32(ret, *(uint32_t*)data, ret);
        data += sizeof(uint32_t);
    }
#endif
    if (sizeSigned & sizeof(uint16_t)) {
        INTRIN_CRC32_16(ret, *(uint16_t*)data, ret);
        data += sizeof(uint16_t);
    }

    if (sizeSigned & sizeof(uint8_t)) {
        INTRIN_CRC32_8(ret, *data, ret);
    }

    return ~ret;
}
#ifdef __cplusplus
}
#endif
