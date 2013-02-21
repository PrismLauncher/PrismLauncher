#ifndef USERUTILS_H
#define USERUTILS_H

#include <QString>

namespace Util
{
    // Get the Directory representing the User's Desktop
    QString getDesktopDir();

    // Create a shortcut at *location*, pointing to *dest* called with the arguments *args*
    // call it *name* and assign it the icon *icon*
    // return true if operation succeeded
    bool createShortCut(QString location, QString dest, QStringList args, QString name, QString iconLocation);
}

#endif // USERUTILS_H
