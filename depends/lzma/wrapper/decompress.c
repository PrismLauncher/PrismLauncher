/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 */

#include "include/decompress.h"
#include "pavlov/LzmaDec.h"
#include "pavlov/7zCrc.h"
#include "common_internal.h"
#include "lzma_header.h"
#include "lzip_header.h"

#include <string.h>
#include <assert.h>

#define ELZMA_DECOMPRESS_INPUT_BUFSIZE (1024 * 64)
#define ELZMA_DECOMPRESS_OUTPUT_BUFSIZE (1024 * 256)

/** an opaque handle to an lzma decompressor */
struct _elzma_decompress_handle
{
	char inbuf[ELZMA_DECOMPRESS_INPUT_BUFSIZE];
	char outbuf[ELZMA_DECOMPRESS_OUTPUT_BUFSIZE];
	struct elzma_alloc_struct allocStruct;
};

elzma_decompress_handle elzma_decompress_alloc()
{
	elzma_decompress_handle hand = malloc(sizeof(struct _elzma_decompress_handle));
	memset((void *)hand, 0, sizeof(struct _elzma_decompress_handle));
	init_alloc_struct(&(hand->allocStruct), NULL, NULL, NULL, NULL);
	return hand;
}

void elzma_decompress_set_allocation_callbacks(elzma_decompress_handle hand,
											   elzma_malloc mallocFunc, void *mallocFuncContext,
											   elzma_free freeFunc, void *freeFuncContext)
{
	if (hand)
	{
		init_alloc_struct(&(hand->allocStruct), mallocFunc, mallocFuncContext, freeFunc,
						  freeFuncContext);
	}
}

void elzma_decompress_free(elzma_decompress_handle *hand)
{
	if (*hand)
		free(*hand);
	*hand = NULL;
}

int elzma_decompress_run(elzma_decompress_handle hand, elzma_read_callback inputStream,
						 void *inputContext, elzma_write_callback outputStream,
						 void *outputContext, elzma_file_format format)
{
	unsigned long long int totalRead = 0; /* total amount read from stream */
	unsigned int crc32 = CRC_INIT_VAL;	/* running crc32 (lzip case) */
	CLzmaDec dec;
	unsigned int errorCode = ELZMA_E_OK;
	struct elzma_format_handler formatHandler;
	struct elzma_file_header h;
	struct elzma_file_footer f;

	/* switch between supported formats */
	if (format == ELZMA_lzma)
	{
		initializeLZMAFormatHandler(&formatHandler);
	}
	else if (format == ELZMA_lzip)
	{
		CrcGenerateTable();
		initializeLZIPFormatHandler(&formatHandler);
	}
	else
	{
		return ELZMA_E_BAD_PARAMS;
	}

	/* initialize footer */
	f.crc32 = 0;
	f.uncompressedSize = 0;

	/* initialize decoder memory */
	memset((void *)&dec, 0, sizeof(dec));
	LzmaDec_Init(&dec);

	/* decode the header. */
	{
		unsigned char *hdr =
			hand->allocStruct.Alloc(&(hand->allocStruct), formatHandler.header_size);

		size_t sz = formatHandler.header_size;

		formatHandler.init_header(&h);

		if (inputStream(inputContext, hdr, &sz) != 0 || sz != formatHandler.header_size)
		{
			hand->allocStruct.Free(&(hand->allocStruct), hdr);
			return ELZMA_E_INPUT_ERROR;
		}

		if (0 != formatHandler.parse_header(hdr, &h))
		{
			hand->allocStruct.Free(&(hand->allocStruct), hdr);
			return ELZMA_E_CORRUPT_HEADER;
		}

		/* the LzmaDec_Allocate call requires 5 bytes which have
		 * compression properties encoded in them.  In the case of
		 * lzip, the header format does not already contain what
		 * LzmaDec_Allocate expects, so we must craft it, silly */
		{
			unsigned char propsBuf[13];
			const unsigned char *propsPtr = hdr;

			if (format == ELZMA_lzip)
			{
				struct elzma_format_handler lzmaHand;
				initializeLZMAFormatHandler(&lzmaHand);
				lzmaHand.serialize_header(propsBuf, &h);
				propsPtr = propsBuf;
			}

			/* now we're ready to allocate the decoder */
			LzmaDec_Allocate(&dec, propsPtr, 5);
		}

		hand->allocStruct.Free(&(hand->allocStruct), hdr);
	}

	/* perform the decoding */
	for (;;)
	{
		size_t dstLen = ELZMA_DECOMPRESS_OUTPUT_BUFSIZE;
		size_t srcLen = ELZMA_DECOMPRESS_INPUT_BUFSIZE;
		size_t amt = 0;
		size_t bufOff = 0;
		ELzmaStatus stat;

		if (0 != inputStream(inputContext, hand->inbuf, &srcLen))
		{
			errorCode = ELZMA_E_INPUT_ERROR;
			goto decompressEnd;
		}

		/* handle the case where the input prematurely finishes */
		if (srcLen == 0)
		{
			errorCode = ELZMA_E_INSUFFICIENT_INPUT;
			goto decompressEnd;
		}

		amt = srcLen;

		/* handle the case where a single read buffer of compressed bytes
		 * will translate into multiple buffers of uncompressed bytes,
		 * with this inner loop */
		stat = LZMA_STATUS_NOT_SPECIFIED;

		while (bufOff < srcLen)
		{
			SRes r = LzmaDec_DecodeToBuf(&dec, (uint8_t *)hand->outbuf, &dstLen,
										 ((uint8_t *)hand->inbuf + bufOff), &amt,
										 LZMA_FINISH_ANY, &stat);

			/* XXX deal with result code more granularly*/
			if (r != SZ_OK)
			{
				errorCode = ELZMA_E_DECOMPRESS_ERROR;
				goto decompressEnd;
			}

			/* write what we've read */
			{
				size_t wt;

				/* if decoding lzip, update our crc32 value */
				if (format == ELZMA_lzip && dstLen > 0)
				{
					crc32 = CrcUpdate(crc32, hand->outbuf, dstLen);
				}
				totalRead += dstLen;

				wt = outputStream(outputContext, hand->outbuf, dstLen);
				if (wt != dstLen)
				{
					errorCode = ELZMA_E_OUTPUT_ERROR;
					goto decompressEnd;
				}
			}

			/* do we have more data on the input buffer? */
			bufOff += amt;
			assert(bufOff <= srcLen);
			if (bufOff >= srcLen)
				break;
			amt = srcLen - bufOff;

			/* with lzip, we will have the footer left on the buffer! */
			if (stat == LZMA_STATUS_FINISHED_WITH_MARK)
			{
				break;
			}
		}

		/* now check status */
		if (stat == LZMA_STATUS_FINISHED_WITH_MARK)
		{
			/* read a footer if one is expected and
			 * present */
			if (formatHandler.footer_size > 0 && amt >= formatHandler.footer_size &&
				formatHandler.parse_footer != NULL)
			{
				formatHandler.parse_footer((unsigned char *)hand->inbuf + bufOff, &f);
			}

			break;
		}
		/* for LZMA utils,  we don't always have a finished mark */
		if (!h.isStreamed && totalRead >= h.uncompressedSize)
		{
			break;
		}
	}

	/* finish the calculated crc32 */
	crc32 ^= 0xFFFFFFFF;

	/* if we have a footer, check that the calculated crc32 matches
	 * the encoded crc32, and that the sizes match */
	if (formatHandler.footer_size)
	{
		if (f.crc32 != crc32)
		{
			errorCode = ELZMA_E_CRC32_MISMATCH;
		}
		else if (f.uncompressedSize != totalRead)
		{
			errorCode = ELZMA_E_SIZE_MISMATCH;
		}
	}
	else if (!h.isStreamed)
	{
		/* if the format does not support a footer and has an uncompressed
		 * size in the header, let's compare that with how much we actually
		 * read */
		if (h.uncompressedSize != totalRead)
		{
			errorCode = ELZMA_E_SIZE_MISMATCH;
		}
	}

decompressEnd:
	LzmaDec_Free(&dec);

	return errorCode;
}
