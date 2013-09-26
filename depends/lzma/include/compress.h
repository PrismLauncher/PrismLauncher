/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * compress.h - the API for LZMA compression using easylzma
 */

#pragma once

#include "common.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** suggested default values */
#define ELZMA_LC_DEFAULT 3
#define ELZMA_LP_DEFAULT 0
#define ELZMA_PB_DEFAULT 2
#define ELZMA_DICT_SIZE_DEFAULT_MAX (1 << 24)

/** an opaque handle to an lzma compressor */
typedef struct _elzma_compress_handle *elzma_compress_handle;

/**
 * Allocate a handle to an LZMA compressor object.
 */
elzma_compress_handle EASYLZMA_API elzma_compress_alloc();

/**
 * set allocation routines (optional, if not called malloc & free will
 * be used)
 */
void EASYLZMA_API
elzma_compress_set_allocation_callbacks(elzma_compress_handle hand, elzma_malloc mallocFunc,
										void *mallocFuncContext, elzma_free freeFunc,
										void *freeFuncContext);

/**
 * Free all data associated with an LZMA compressor object.
 */
void EASYLZMA_API elzma_compress_free(elzma_compress_handle *hand);

/**
 * Set configuration paramters for a compression run.  If not called,
 * reasonable defaults will be used.
 */
int EASYLZMA_API elzma_compress_config(elzma_compress_handle hand, unsigned char lc,
									   unsigned char lp, unsigned char pb, unsigned char level,
									   unsigned int dictionarySize, elzma_file_format format,
									   unsigned long long uncompressedSize);

/**
 * Run compression
 */
int EASYLZMA_API
elzma_compress_run(elzma_compress_handle hand, elzma_read_callback inputStream,
				   void *inputContext, elzma_write_callback outputStream, void *outputContext,
				   elzma_progress_callback progressCallback, void *progressContext);

/**
 * a heuristic utility routine to guess a dictionary size that gets near
 * optimal compression while reducing memory usage.
 * accepts a size in bytes, returns a proposed dictionary size
 */
unsigned int EASYLZMA_API elzma_get_dict_size(unsigned long long size);

#ifdef __cplusplus
}
;
#endif
