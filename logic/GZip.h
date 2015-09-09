#pragma once
#include <QByteArray>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT GZip
{
public:
	static bool decompress(const QByteArray &compressedBytes, QByteArray &uncompressedBytes);
};

