#include "GZip.h"
#include <zlib.h>
#include <QByteArray>

bool GZip::unzip(const QByteArray &compressedBytes, QByteArray &uncompressedBytes)
{
	if (compressedBytes.size() == 0)
	{
		uncompressedBytes = compressedBytes;
		return true;
	}

	unsigned uncompLength = compressedBytes.size();
	uncompressedBytes.clear();
	uncompressedBytes.resize(uncompLength);

	z_stream strm;
	memset(&strm, 0, sizeof(strm));
	strm.next_in = (Bytef *)compressedBytes.data();
	strm.avail_in = compressedBytes.size();

	bool done = false;

	if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK)
	{
		return false;
	}

	int err = Z_OK;

	while (!done)
	{
		// If our output buffer is too small
		if (strm.total_out >= uncompLength)
		{
			uncompressedBytes.resize(uncompLength * 2);
			uncompLength *= 2;
		}

		strm.next_out = (Bytef *)(uncompressedBytes.data() + strm.total_out);
		strm.avail_out = uncompLength - strm.total_out;

		// Inflate another chunk.
		err = inflate(&strm, Z_SYNC_FLUSH);
		if (err == Z_STREAM_END)
			done = true;
		else if (err != Z_OK)
		{
			break;
		}
	}

	if (inflateEnd(&strm) != Z_OK || !done)
	{
		return false;
	}

	uncompressedBytes.resize(strm.total_out);
	return true;
}

bool GZip::zip(const QByteArray &uncompressedBytes, QByteArray &compressedBytes)
{
	if (uncompressedBytes.size() == 0)
	{
		compressedBytes = uncompressedBytes;
		return true;
	}

	unsigned compLength = std::min(uncompressedBytes.size(), 16);
	compressedBytes.clear();
	compressedBytes.resize(compLength);

	z_stream zs;
	memset(&zs, 0, sizeof(zs));

	if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (16 + MAX_WBITS), 8, Z_DEFAULT_STRATEGY) != Z_OK)
	{
		return false;
	}

	zs.next_in = (Bytef*)uncompressedBytes.data();
	zs.avail_in = uncompressedBytes.size();

	int ret;
	compressedBytes.resize(uncompressedBytes.size());

	unsigned offset = 0;
	unsigned temp = 0;
	do
	{
		auto remaining = compressedBytes.size() - offset;
		if(remaining < 1)
		{
			compressedBytes.resize(compressedBytes.size() * 2);
		}
		zs.next_out = (Bytef *) (compressedBytes.data() + offset);
		temp = zs.avail_out = compressedBytes.size() - offset;
		ret = deflate(&zs, Z_FINISH);
		offset += temp - zs.avail_out;
	} while (ret == Z_OK);

	compressedBytes.resize(offset);

	if (deflateEnd(&zs) != Z_OK)
	{
		return false;
	}

	if (ret != Z_STREAM_END)
	{
		return false;
	}
	return true;
}