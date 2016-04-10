#pragma once
#include <QByteArray>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT GZip
{
public:
	static bool unzip(const QByteArray &compressedBytes, QByteArray &uncompressedBytes);
	static bool zip(const QByteArray &uncompressedBytes, QByteArray &compressedBytes);
};

