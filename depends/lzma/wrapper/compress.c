/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 */

#include "compress.h"
#include "lzma_header.h"
#include "lzip_header.h"
#include "common_internal.h"

#include "pavlov/Types.h"
#include "pavlov/LzmaEnc.h"
#include "pavlov/7zCrc.h"

#include <string.h>

struct _elzma_compress_handle
{
	CLzmaEncProps props;
	CLzmaEncHandle encHand;
	unsigned long long uncompressedSize;
	elzma_file_format format;
	struct elzma_alloc_struct allocStruct;
	struct elzma_format_handler formatHandler;
};

elzma_compress_handle elzma_compress_alloc()
{
	elzma_compress_handle hand = malloc(sizeof(struct _elzma_compress_handle));
	memset((void *)hand, 0, sizeof(struct _elzma_compress_handle));

	/* "reasonable" defaults for props */
	LzmaEncProps_Init(&(hand->props));
	hand->props.lc = 3;
	hand->props.lp = 0;
	hand->props.pb = 2;
	hand->props.level = 5;
	hand->props.algo = 1;
	hand->props.fb = 32;
	hand->props.dictSize = 1 << 24;
	hand->props.btMode = 1;
	hand->props.numHashBytes = 4;
	hand->props.mc = 32;
	hand->props.numThreads = 1;
	hand->props.writeEndMark = 1;

	init_alloc_struct(&(hand->allocStruct), NULL, NULL, NULL, NULL);

	/* default format is LZMA-Alone */
	initializeLZMAFormatHandler(&(hand->formatHandler));

	return hand;
}

void elzma_compress_free(elzma_compress_handle *hand)
{
	if (hand && *hand)
	{
		if ((*hand)->encHand)
		{
			LzmaEnc_Destroy((*hand)->encHand);
		}
	}
	*hand = NULL;
}

int elzma_compress_config(elzma_compress_handle hand, unsigned char lc, unsigned char lp,
						  unsigned char pb, unsigned char level, unsigned int dictionarySize,
						  elzma_file_format format, unsigned long long uncompressedSize)
{
	/* XXX: validate arguments are in valid ranges */

	hand->props.lc = lc;
	hand->props.lp = lp;
	hand->props.pb = pb;
	hand->props.level = level;
	hand->props.dictSize = dictionarySize;
	hand->uncompressedSize = uncompressedSize;
	hand->format = format;

	/* default of LZMA-Alone is set at alloc time, and there are only
	 * two possible formats */
	if (format == ELZMA_lzip)
	{
		initializeLZIPFormatHandler(&(hand->formatHandler));
	}

	return ELZMA_E_OK;
}

/* use Igor's stream hooks for compression. */
struct elzmaInStream
{
	SRes (*ReadPtr)(void *p, void *buf, size_t *size);
	elzma_read_callback inputStream;
	void *inputContext;
	unsigned int crc32;
	unsigned int crc32a;
	unsigned int crc32b;
	unsigned int crc32c;
	int calculateCRC;
};

static SRes elzmaReadFunc(void *p, void *buf, size_t *size)
{
	int rv;
	struct elzmaInStream *is = (struct elzmaInStream *)p;
	rv = is->inputStream(is->inputContext, buf, size);
	if (rv == 0 && *size > 0 && is->calculateCRC)
	{
		is->crc32 = CrcUpdate(is->crc32, buf, *size);
	}
	return rv;
}

struct elzmaOutStream
{
	size_t (*WritePtr)(void *p, const void *buf, size_t size);
	elzma_write_callback outputStream;
	void *outputContext;
};

static size_t elzmaWriteFunc(void *p, const void *buf, size_t size)
{
	struct elzmaOutStream *os = (struct elzmaOutStream *)p;
	return os->outputStream(os->outputContext, buf, size);
}

/* use Igor's stream hooks for compression. */
struct elzmaProgressStruct
{
	SRes (*Progress)(void *p, uint64_t inSize, uint64_t outSize);
	long long unsigned int uncompressedSize;
	elzma_progress_callback progressCallback;
	void *progressContext;
};

#include <stdio.h>
static SRes elzmaProgress(void *p, uint64_t inSize, uint64_t outSize)
{
	struct elzmaProgressStruct *ps = (struct elzmaProgressStruct *)p;
	if (ps->progressCallback)
	{
		ps->progressCallback(ps->progressContext, inSize, ps->uncompressedSize);
	}
	return SZ_OK;
}

void elzma_compress_set_allocation_callbacks(elzma_compress_handle hand,
											 elzma_malloc mallocFunc, void *mallocFuncContext,
											 elzma_free freeFunc, void *freeFuncContext)
{
	if (hand)
	{
		init_alloc_struct(&(hand->allocStruct), mallocFunc, mallocFuncContext, freeFunc,
						  freeFuncContext);
	}
}

int elzma_compress_run(elzma_compress_handle hand, elzma_read_callback inputStream,
					   void *inputContext, elzma_write_callback outputStream,
					   void *outputContext, elzma_progress_callback progressCallback,
					   void *progressContext)
{
	struct elzmaInStream inStreamStruct;
	struct elzmaOutStream outStreamStruct;
	struct elzmaProgressStruct progressStruct;
	SRes r;

	CrcGenerateTable();

	if (hand == NULL || inputStream == NULL)
		return ELZMA_E_BAD_PARAMS;

	/* initialize stream structrures */
	inStreamStruct.ReadPtr = elzmaReadFunc;
	inStreamStruct.inputStream = inputStream;
	inStreamStruct.inputContext = inputContext;
	inStreamStruct.crc32 = CRC_INIT_VAL;
	inStreamStruct.calculateCRC = (hand->formatHandler.serialize_footer != NULL);

	outStreamStruct.WritePtr = elzmaWriteFunc;
	outStreamStruct.outputStream = outputStream;
	outStreamStruct.outputContext = outputContext;

	progressStruct.Progress = elzmaProgress;
	progressStruct.uncompressedSize = hand->uncompressedSize;
	progressStruct.progressCallback = progressCallback;
	progressStruct.progressContext = progressContext;

	/* create an encoding object */
	hand->encHand = LzmaEnc_Create();

	if (hand->encHand == NULL)
	{
		return ELZMA_E_COMPRESS_ERROR;
	}

	/* inintialize with compression parameters */
	if (SZ_OK != LzmaEnc_SetProps(hand->encHand, &(hand->props)))
	{
		return ELZMA_E_BAD_PARAMS;
	}

	/* verify format is sane */
	if (ELZMA_lzma != hand->format && ELZMA_lzip != hand->format)
	{
		return ELZMA_E_UNSUPPORTED_FORMAT;
	}

	/* now write the compression header header */
	{
		unsigned char *hdr =
			hand->allocStruct.Alloc(&(hand->allocStruct), hand->formatHandler.header_size);

		struct elzma_file_header h;
		size_t wt;

		hand->formatHandler.init_header(&h);
		h.pb = (unsigned char)hand->props.pb;
		h.lp = (unsigned char)hand->props.lp;
		h.lc = (unsigned char)hand->props.lc;
		h.dictSize = hand->props.dictSize;
		h.isStreamed = (unsigned char)(hand->uncompressedSize == 0);
		h.uncompressedSize = hand->uncompressedSize;

		hand->formatHandler.serialize_header(hdr, &h);

		wt = outputStream(outputContext, (void *)hdr, hand->formatHandler.header_size);

		hand->allocStruct.Free(&(hand->allocStruct), hdr);

		if (wt != hand->formatHandler.header_size)
		{
			return ELZMA_E_OUTPUT_ERROR;
		}
	}

	/* begin LZMA encoding */
	/* XXX: expose encoding progress */
	r = LzmaEnc_Encode(hand->encHand, (ISeqOutStream *)&outStreamStruct,
					   (ISeqInStream *)&inStreamStruct, (ICompressProgress *)&progressStruct);

	if (r != SZ_OK)
		return ELZMA_E_COMPRESS_ERROR;

	/* support a footer! (lzip) */
	if (hand->formatHandler.serialize_footer != NULL && hand->formatHandler.footer_size > 0)
	{
		size_t wt;
		unsigned char *ftrBuf =
			hand->allocStruct.Alloc(&(hand->allocStruct), hand->formatHandler.footer_size);
		struct elzma_file_footer ftr;
		ftr.crc32 = inStreamStruct.crc32 ^ 0xFFFFFFFF;
		ftr.uncompressedSize = hand->uncompressedSize;

		hand->formatHandler.serialize_footer(&ftr, ftrBuf);

		wt = outputStream(outputContext, (void *)ftrBuf, hand->formatHandler.footer_size);

		hand->allocStruct.Free(&(hand->allocStruct), ftrBuf);

		if (wt != hand->formatHandler.footer_size)
		{
			return ELZMA_E_OUTPUT_ERROR;
		}
	}

	return ELZMA_E_OK;
}

unsigned int elzma_get_dict_size(unsigned long long size)
{
	int i = 13; /* 16k dict is minimum */

	/* now we'll find the closes power of two with a max at 16< *
	 * if the size is greater than 8m, we'll divide by two, all of this
	 * is based on a quick set of emperical tests on hopefully
	 * representative sample data */
	if (size > (1 << 23))
		size >>= 1;

	while (size >> i)
		i++;

	if (i > 23)
		return 1 << 23;

	/* now 1 << i is greater than size, let's return either 1<<i or 1<<(i-1),
	 * whichever is closer to size */
	return 1 << ((((1 << i) - size) > (size - (1 << (i - 1)))) ? i - 1 : i);
}
