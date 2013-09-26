#include "lzip_header.h"

#include <string.h>

#define ELZMA_LZIP_HEADER_SIZE 6
#define ELZMA_LZIP_FOOTER_SIZE 12

static void initLzipHeader(struct elzma_file_header *hdr)
{
	memset((void *)hdr, 0, sizeof(struct elzma_file_header));
}

static int parseLzipHeader(const unsigned char *hdrBuf, struct elzma_file_header *hdr)
{
	if (0 != strncmp("LZIP", (char *)hdrBuf, 4))
		return 1;
	/* XXX: ignore version for now */
	hdr->pb = 2;
	hdr->lp = 0;
	hdr->lc = 3;
	/* unknown at this point */
	hdr->isStreamed = 1;
	hdr->uncompressedSize = 0;
	hdr->dictSize = 1 << (hdrBuf[5] & 0x1F);
	return 0;
}

static int serializeLzipHeader(unsigned char *hdrBuf, const struct elzma_file_header *hdr)
{
	hdrBuf[0] = 'L';
	hdrBuf[1] = 'Z';
	hdrBuf[2] = 'I';
	hdrBuf[3] = 'P';
	hdrBuf[4] = 0;
	{
		int r = 0;
		while ((hdr->dictSize >> r) != 0)
			r++;
		hdrBuf[5] = (unsigned char)(r - 1) & 0x1F;
	}
	return 0;
}

static int serializeLzipFooter(struct elzma_file_footer *ftr, unsigned char *ftrBuf)
{
	unsigned int i = 0;

	/* first crc32 */
	for (i = 0; i < 4; i++)
	{
		*(ftrBuf++) = (unsigned char)(ftr->crc32 >> (i * 8));
	}

	/* next data size */
	for (i = 0; i < 8; i++)
	{
		*(ftrBuf++) = (unsigned char)(ftr->uncompressedSize >> (i * 8));
	}

	/* write version 0 files, omit member length for now*/

	return 0;
}

static int parseLzipFooter(const unsigned char *ftrBuf, struct elzma_file_footer *ftr)
{
	unsigned int i = 0;
	ftr->crc32 = 0;
	ftr->uncompressedSize = 0;

	/* first crc32 */
	for (i = 0; i < 4; i++)
	{
		ftr->crc32 += ((unsigned int)*(ftrBuf++) << (i * 8));
	}

	/* next data size */
	for (i = 0; i < 8; i++)
	{
		ftr->uncompressedSize += (unsigned long long)*(ftrBuf++) << (i * 8);
	}
	/* read version 0 files, omit member length for now*/

	return 0;
}

void initializeLZIPFormatHandler(struct elzma_format_handler *hand)
{
	hand->header_size = ELZMA_LZIP_HEADER_SIZE;
	hand->init_header = initLzipHeader;
	hand->parse_header = parseLzipHeader;
	hand->serialize_header = serializeLzipHeader;
	hand->footer_size = ELZMA_LZIP_FOOTER_SIZE;
	hand->serialize_footer = serializeLzipFooter;
	hand->parse_footer = parseLzipFooter;
}
