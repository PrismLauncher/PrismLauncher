#pragma once

#include <QString>

#include "multimc_util_export.h"

namespace Util
{
// Get the Directory representing the User's Desktop
MULTIMC_UTIL_EXPORT QString getDesktopDir();

// Create a shortcut at *location*, pointing to *dest* called with the arguments *args*
// call it *name* and assign it the icon *icon*
// return true if operation succeeded
MULTIMC_UTIL_EXPORT bool createShortCut(QString location, QString dest, QStringList args,
								   QString name, QString iconLocation);
}
