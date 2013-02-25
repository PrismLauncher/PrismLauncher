#ifndef USERUTILS_H
#define USERUTILS_H

#include <QString>

#include "libutil_config.h"

namespace Util
{
// Get the Directory representing the User's Desktop
LIBMMCUTIL_EXPORT QString getDesktopDir();

// Create a shortcut at *location*, pointing to *dest* called with the arguments *args*
// call it *name* and assign it the icon *icon*
// return true if operation succeeded
LIBMMCUTIL_EXPORT bool createShortCut(QString location, QString dest, QStringList args, QString name, QString iconLocation);
}

#endif // USERUTILS_H
