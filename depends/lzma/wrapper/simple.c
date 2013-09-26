/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * simple.c - a wrapper around easylzma to compress/decompress to memory
 */

#include "simple.h"

#include <string.h>
#include <assert.h>

struct dataStream
{
	const unsigned char *inData;
	size_t inLen;

	unsigned char *outData;
	size_t outLen;
};

static int inputCallback(void *ctx, void *buf, size_t *size)
{
	size_t rd = 0;
	struct dataStream *ds = (struct dataStream *)ctx;
	assert(ds != NULL);

	rd = (ds->inLen < *size) ? ds->inLen : *size;

	if (rd > 0)
	{
		memcpy(buf, (void *)ds->inData, rd);
		ds->inData += rd;
		ds->inLen -= rd;
	}

	*size = rd;

	return 0;
}

static size_t outputCallback(void *ctx, const void *buf, size_t size)
{
	struct dataStream *ds = (struct dataStream *)ctx;
	assert(ds != NULL);

	if (size > 0)
	{
		ds->outData = realloc(ds->outData, ds->outLen + size);
		memcpy((void *)(ds->outData + ds->outLen), buf, size);
		ds->outLen += size;
	}

	return size;
}

int simpleCompress(elzma_file_format format, const unsigned char *inData, size_t inLen,
				   unsigned char **outData, size_t *outLen)
{
	int rc;
	elzma_compress_handle hand;

	/* allocate compression handle */
	hand = elzma_compress_alloc();
	assert(hand != NULL);

	rc = elzma_compress_config(hand, ELZMA_LC_DEFAULT, ELZMA_LP_DEFAULT, ELZMA_PB_DEFAULT, 5,
							   (1 << 20) /* 1mb */, format, inLen);

	if (rc != ELZMA_E_OK)
	{
		elzma_compress_free(&hand);
		return rc;
	}

	/* now run the compression */
	{
		struct dataStream ds;
		ds.inData = inData;
		ds.inLen = inLen;
		ds.outData = NULL;
		ds.outLen = 0;

		rc = elzma_compress_run(hand, inputCallback, (void *)&ds, outputCallback, (void *)&ds,
								NULL, NULL);

		if (rc != ELZMA_E_OK)
		{
			if (ds.outData != NULL)
				free(ds.outData);
			elzma_compress_free(&hand);
			return rc;
		}

		*outData = ds.outData;
		*outLen = ds.outLen;
	}

	return rc;
}

int simpleDecompress(elzma_file_format format, const unsigned char *inData, size_t inLen,
					 unsigned char **outData, size_t *outLen)
{
	int rc;
	elzma_decompress_handle hand;

	hand = elzma_decompress_alloc();

	/* now run the compression */
	{
		struct dataStream ds;
		ds.inData = inData;
		ds.inLen = inLen;
		ds.outData = NULL;
		ds.outLen = 0;

		rc = elzma_decompress_run(hand, inputCallback, (void *)&ds, outputCallback, (void *)&ds,
								  format);

		if (rc != ELZMA_E_OK)
		{
			if (ds.outData != NULL)
				free(ds.outData);
			elzma_decompress_free(&hand);
			return rc;
		}

		*outData = ds.outData;
		*outLen = ds.outLen;
	}

	return rc;
}
