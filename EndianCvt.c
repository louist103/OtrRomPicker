#include <stdlib.h>
#include <stdint.h>

#ifndef _MSC_VER
#include <byteswap.h>
#define BSWAP32 bswap_32
#define BSWAP16 bswap_16
#else  //Windows
#define BSWAP32 _byteswap_ulong
#define BSWAP16 _byteswap_ushort
#endif

void RomToBigEndian(void* rom, size_t romSize) {
	uint8_t firstbyte = ((uint8_t*)rom)[0];

	switch (firstbyte) {
	case 0x80: //BE
		return; //Already BE, no need to swap
	case 0x37:
		for (size_t pos = 0; pos < (romSize / 2); pos++) {
			((uint16_t*)rom)[pos] = BSWAP16(((uint16_t*)rom)[pos]);
		}
		return;
	case 0x40:
		for (size_t pos = 0; pos < (romSize / 4); pos++) {
			((uint32_t*)rom)[pos] = BSWAP32(((uint32_t*)rom)[pos]);
		}
	}

}