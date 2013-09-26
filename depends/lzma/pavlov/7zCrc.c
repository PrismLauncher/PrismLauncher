/* 7zCrc.c -- CRC32 calculation
2008-08-05
Igor Pavlov
Public domain */

#include "7zCrc.h"

#define kCrcPoly 0xEDB88320
uint32_t g_CrcTable[256];

void MY_FAST_CALL CrcGenerateTable(void)
{
	uint32_t i;
	for (i = 0; i < 256; i++)
	{
		uint32_t r = i;
		int j;
		for (j = 0; j < 8; j++)
			r = (r >> 1) ^ (kCrcPoly & ~((r & 1) - 1));
		g_CrcTable[i] = r;
	}
}

uint32_t MY_FAST_CALL CrcUpdate(uint32_t v, const void *data, size_t size)
{
	const uint8_t *p = (const uint8_t *)data;
	for (; size > 0; size--, p++)
		v = CRC_UPDATE_BYTE(v, *p);
	return v;
}

uint32_t MY_FAST_CALL CrcCalc(const void *data, size_t size)
{
	return CrcUpdate(CRC_INIT_VAL, data, size) ^ 0xFFFFFFFF;
}
