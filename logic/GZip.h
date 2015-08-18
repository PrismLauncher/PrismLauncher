#pragma once
#include <QByteArray>

class GZip
{
public:
	static bool inflate(const QByteArray &compressedBytes, QByteArray &uncompressedBytes);
};

