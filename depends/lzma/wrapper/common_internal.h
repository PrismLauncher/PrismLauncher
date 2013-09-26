#ifndef __ELZMA_COMMON_INTERNAL_H__
#define __ELZMA_COMMON_INTERNAL_H__

#include "common.h"

/** a structure which may be cast and passed into Igor's allocate
 *  routines */
struct elzma_alloc_struct
{
	void *(*Alloc)(void *p, size_t size);
	void (*Free)(void *p, void *address); /* address can be 0 */

	elzma_malloc clientMallocFunc;
	void *clientMallocContext;

	elzma_free clientFreeFunc;
	void *clientFreeContext;
};

/* initialize an allocation structure, may be called safely multiple
 * times */
void init_alloc_struct(struct elzma_alloc_struct *allocStruct, elzma_malloc clientMallocFunc,
					   void *clientMallocContext, elzma_free clientFreeFunc,
					   void *clientFreeContext);

/** superset representation of a compressed file header */
struct elzma_file_header
{
	unsigned char pb;
	unsigned char lp;
	unsigned char lc;
	unsigned char isStreamed;
	long long unsigned int uncompressedSize;
	unsigned int dictSize;
};

/** superset representation of a compressed file footer */
struct elzma_file_footer
{
	unsigned int crc32;
	long long unsigned int uncompressedSize;
};

/** a structure which encapsulates information about the particular
 *  file header and footer in use (lzip vs lzma vs (eventually) xz.
 *  The intention of this structure is to simplify compression and
 *  decompression logic by abstracting the file format details a bit.  */
struct elzma_format_handler
{
	unsigned int header_size;
	void (*init_header)(struct elzma_file_header *hdr);
	int (*parse_header)(const unsigned char *hdrBuf, struct elzma_file_header *hdr);
	int (*serialize_header)(unsigned char *hdrBuf, const struct elzma_file_header *hdr);

	unsigned int footer_size;
	int (*serialize_footer)(struct elzma_file_footer *ftr, unsigned char *ftrBuf);
	int (*parse_footer)(const unsigned char *ftrBuf, struct elzma_file_footer *ftr);
};

#endif
