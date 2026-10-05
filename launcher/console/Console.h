#pragma once

#include <QString>

#include <array>

#if defined Q_OS_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <cstdio>
#endif

namespace Console {

inline bool isConsole()
{
#ifdef Q_OS_WIN32
    std::array<DWORD, 2> procIDs{};
    DWORD result = GetConsoleProcessList(procIDs.data(), 2);
    return result > 1;
#else
    if (isatty(fileno(stdout))) {
        return true;
    }
    return false;
#endif
}

}  // namespace console
