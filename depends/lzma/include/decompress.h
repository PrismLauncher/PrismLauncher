/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * easylzma/decompress.h - The API for LZMA decompression using easylzma
 */

#pragma once

#include "include/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** an opaque handle to an lzma decompressor */
typedef struct _elzma_decompress_handle *elzma_decompress_handle;

/**
 * Allocate a handle to an LZMA decompressor object.
 */
elzma_decompress_handle EASYLZMA_API elzma_decompress_alloc();

/**
 * set allocation routines (optional, if not called malloc & free will
 * be used)
 */
void EASYLZMA_API
elzma_decompress_set_allocation_callbacks(elzma_decompress_handle hand, elzma_malloc mallocFunc,
										  void *mallocFuncContext, elzma_free freeFunc,
										  void *freeFuncContext);

/**
 * Free all data associated with an LZMA decompressor object.
 */
void EASYLZMA_API elzma_decompress_free(elzma_decompress_handle *hand);

/**
 * Perform decompression
 *
 * XXX: should the library automatically detect format by reading stream?
 *      currently it's based on data external to stream (such as extension
 *      or convention)
 */
int EASYLZMA_API elzma_decompress_run(elzma_decompress_handle hand,
									  elzma_read_callback inputStream, void *inputContext,
									  elzma_write_callback outputStream, void *outputContext,
									  elzma_file_format format);

#ifdef __cplusplus
}
;
#endif
