// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "Exception.h"

namespace FS
{
DECLARE_EXCEPTION(FileSystem);

void write(const QString &filename, const QByteArray &data);
QByteArray read(const QString &filename);
}
