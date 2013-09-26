/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * easylzma/common.h - definitions common to both compression and
 *                     decompression
 */

#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* msft dll export gunk.  To build a DLL on windows, you
 * must define WIN32, EASYLZMA_SHARED, and EASYLZMA_BUILD.  To use a
 * DLL, you must define EASYLZMA_SHARED and WIN32 */
#if defined(WIN32) && defined(EASYLZMA_SHARED)
#ifdef EASYLZMA_BUILD
#define EASYLZMA_API __declspec(dllexport)
#else
#define EASYLZMA_API __declspec(dllimport)
#endif
#else
#define EASYLZMA_API
#endif

/** error codes */

/** no error */
#define ELZMA_E_OK 0
/** bad parameters passed to an ELZMA function */
#define ELZMA_E_BAD_PARAMS 10
/** could not initialize the encode with configured parameters. */
#define ELZMA_E_ENCODING_PROPERTIES_ERROR 11
/** an error occured during compression (XXX: be more specific) */
#define ELZMA_E_COMPRESS_ERROR 12
/** currently unsupported lzma file format was specified*/
#define ELZMA_E_UNSUPPORTED_FORMAT 13
/** an error occured when reading input */
#define ELZMA_E_INPUT_ERROR 14
/** an error occured when writing output */
#define ELZMA_E_OUTPUT_ERROR 15
/** LZMA header couldn't be parsed */
#define ELZMA_E_CORRUPT_HEADER 16
/** an error occured during decompression (XXX: be more specific) */
#define ELZMA_E_DECOMPRESS_ERROR 17
/** the input stream returns EOF before the decompression could complete */
#define ELZMA_E_INSUFFICIENT_INPUT 18
/** for formats which have an emebedded crc, this error would indicated that
 *  what came out was not what went in, i.e. data corruption */
#define ELZMA_E_CRC32_MISMATCH 19
/** for formats which have an emebedded uncompressed content length,
 *  this error indicates that the amount we read was not what we expected */
#define ELZMA_E_SIZE_MISMATCH 20

/** Supported file formats */
typedef enum
{
	ELZMA_lzip, /**< the lzip format which includes a magic number and
				 *   CRC check */
	ELZMA_lzma /**< the LZMA-Alone format, originally designed by
				*   Igor Pavlov and in widespread use due to lzmautils,
				*   lacking both aforementioned features of lzip */
	/* XXX: future, potentially   ,
		ELZMA_xz
	*/
} elzma_file_format;

/**
 * A callback invoked during elzma_[de]compress_run when the [de]compression
 * process has generated [de]compressed output.
 *
 * the size parameter indicates how much data is in buf to be written.
 * it is required that the write callback consume all data, and a return
 * value not equal to input size indicates and error.
 */
typedef size_t (*elzma_write_callback)(void *ctx, const void *buf, size_t size);

/**
 * A callback invoked during elzma_[de]compress_run when the [de]compression
 * process requires more [un]compressed input.
 *
 * the size parameter is an in/out argument.  on input it indicates
 * the buffer size.  on output it indicates the amount of data read into
 * buf.  when *size is zero on output it indicates EOF.
 *
 * \returns the read callback should return nonzero on failure.
 */
typedef int (*elzma_read_callback)(void *ctx, void *buf, size_t *size);

/**
 * A callback invoked during elzma_[de]compress_run to report progress
 * on the [de]compression.
 *
 * \returns the read callback should return nonzero on failure.
 */
typedef void (*elzma_progress_callback)(void *ctx, size_t complete, size_t total);

/** pointer to a malloc function, supporting client overriding memory
 *  allocation routines */
typedef void *(*elzma_malloc)(void *ctx, unsigned int sz);

/** pointer to a free function, supporting client overriding memory
 *  allocation routines */
typedef void (*elzma_free)(void *ctx, void *ptr);

#ifdef __cplusplus
}
;
#endif
