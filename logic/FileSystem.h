// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "Exception.h"

#include "multimc_logic_export.h"

namespace FS
{

class MULTIMC_LOGIC_EXPORT FileSystemException : public ::Exception
{
public:
	FileSystemException(const QString &message) : Exception(message) {}
};

void MULTIMC_LOGIC_EXPORT write(const QString &filename, const QByteArray &data);
QByteArray MULTIMC_LOGIC_EXPORT read(const QString &filename);
}
