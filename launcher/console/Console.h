#pragma once

#include <QString>

#include <ostream>
#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <cstdio>
#endif

namespace console {

inline bool isConsole()
{
#if defined Q_OS_WIN32
    DWORD procIDs[2];
    DWORD maxCount = 2;
    DWORD result = GetConsoleProcessList((LPDWORD)procIDs, maxCount);
    return result > 1;
#else
    if (isatty(fileno(stdout))) {
        return true;
    }
    return false;
#endif
}

}  // namespace console
