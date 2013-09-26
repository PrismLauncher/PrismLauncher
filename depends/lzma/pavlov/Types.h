/* Types.h -- Basic types
2008-11-23 : Igor Pavlov : Public domain */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define SZ_OK 0

#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12

#define SZ_ERROR_ARCHIVE 16
#define SZ_ERROR_NO_ARCHIVE 17

typedef int SRes;

#ifndef RINOK
#define RINOK(x)                                                                               \
	{                                                                                          \
		int __result__ = (x);                                                                  \
		if (__result__ != 0)                                                                   \
			return __result__;                                                                 \
	}
#endif

typedef int Bool;
#define True 1
#define False 0

#ifdef _MSC_VER

#if _MSC_VER >= 1300
#define MY_NO_INLINE __declspec(noinline)
#else
#define MY_NO_INLINE
#endif

#define MY_CDECL __cdecl
#define MY_STD_CALL __stdcall
#define MY_FAST_CALL MY_NO_INLINE __fastcall

#else

#define MY_CDECL
#define MY_STD_CALL
#define MY_FAST_CALL

#endif

/* The following interfaces use first parameter as pointer to structure */

typedef struct
{
	SRes (*Read)(void *p, void *buf, size_t *size);
	/* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
	   (output(*size) < input(*size)) is allowed */
} ISeqInStream;

typedef struct
{
	size_t (*Write)(void *p, const void *buf, size_t size);
	/* Returns: result - the number of actually written bytes.
	   (result < size) means error */
} ISeqOutStream;

typedef struct
{
	SRes (*Progress)(void *p, uint64_t inSize, uint64_t outSize);
	/* Returns: result. (result != SZ_OK) means break.
	   Value (uint64_t)(int64_t)-1 for size means unknown value. */
} ICompressProgress;
