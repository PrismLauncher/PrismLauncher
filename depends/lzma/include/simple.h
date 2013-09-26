/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * simple.h - a wrapper around easylzma to compress/decompress to memory
 */

#pragma once

#include "include/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "include/compress.h"
#include "include/decompress.h"

/* compress a chunk of memory and return a dynamically allocated buffer
 * if successful.  return value is an easylzma error code */
int EASYLZMA_API simpleCompress(elzma_file_format format, const unsigned char *inData,
								size_t inLen, unsigned char **outData, size_t *outLen);

/* decompress a chunk of memory and return a dynamically allocated buffer
 * if successful.  return value is an easylzma error code */
int EASYLZMA_API simpleDecompress(elzma_file_format format, const unsigned char *inData,
								  size_t inLen, unsigned char **outData, size_t *outLen);

#ifdef __cplusplus
}
;
#endif