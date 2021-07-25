#include "JavaInstall.h"
#include <MMCStrings.h>

bool JavaInstall::operator<(const JavaInstall &rhs)
{
    auto archCompare = Strings::naturalCompare(arch, rhs.arch, Qt::CaseInsensitive);
    if(archCompare != 0)
        return archCompare < 0;
    if(id < rhs.id)
    {
        return true;
    }
    if(id > rhs.id)
    {
        return false;
    }
    return Strings::naturalCompare(path, rhs.path, Qt::CaseInsensitive) < 0;
}

bool JavaInstall::operator==(const JavaInstall &rhs)
{
    return arch == rhs.arch && id == rhs.id && path == rhs.path;
}

bool JavaInstall::operator>(const JavaInstall &rhs)
{
    return (!operator<(rhs)) && (!operator==(rhs));
}
