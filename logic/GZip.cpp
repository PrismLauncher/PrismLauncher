#include "GZip.h"
#include <zlib.h>
#include <QByteArray>

// HACK: workaround for terrible macro crap on Windows
int wrap_inflate (z_streamp strm, int flush)
{
	return inflate(strm, flush);
}

#ifdef inflate
	#undef inflate
#endif

bool GZip::inflate(const QByteArray &compressedBytes, QByteArray &uncompressedBytes)
{
	if (compressedBytes.size() == 0)
	{
		uncompressedBytes = compressedBytes;
		return true;
	}

	unsigned uncompLength = compressedBytes.size();
	unsigned half_length = compressedBytes.size() / 2;
	uncompressedBytes.clear();
	uncompressedBytes.resize(uncompLength);

	z_stream strm;
	strm.next_in = (Bytef *)compressedBytes.data();
	strm.avail_in = compressedBytes.size();
	strm.total_out = 0;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;

	bool done = false;

	if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK)
	{
		return false;
	}

	while (!done)
	{
		// If our output buffer is too small
		if (strm.total_out >= uncompLength)
		{
			uncompressedBytes.resize(uncompLength + half_length);
			uncompLength += half_length;
		}

		strm.next_out = (Bytef *)(uncompressedBytes.data() + strm.total_out);
		strm.avail_out = uncompLength - strm.total_out;

		// Inflate another chunk.
		int err = wrap_inflate(&strm, Z_SYNC_FLUSH);
		if (err == Z_STREAM_END)
			done = true;
		else if (err != Z_OK)
		{
			break;
		}
	}

	if (inflateEnd(&strm) != Z_OK)
	{
		return false;
	}

	uncompressedBytes.resize(strm.total_out);
	return true;
}
