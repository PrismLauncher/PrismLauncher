/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "WindowsConsole.h"
#include <system_error>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <cstddef>
#include <iostream>

void RedirectHandle(DWORD handle, FILE* stream, const char* mode)
{
    HANDLE stdHandle = GetStdHandle(handle);
    if (stdHandle != INVALID_HANDLE_VALUE) {
        int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
        if (fileDescriptor != -1) {
            FILE* file = _fdopen(fileDescriptor, mode);
            if (file != NULL) {
                int dup2Result = _dup2(_fileno(file), _fileno(stream));
                if (dup2Result == 0) {
                    setvbuf(stream, NULL, _IONBF, 0);
                }
            }
        }
    }
}

// taken from https://stackoverflow.com/a/25927081
// getting a proper output to console with redirection support on windows is apparently hell
void BindCrtHandlesToStdHandles(bool bindStdIn, bool bindStdOut, bool bindStdErr)
{
    // Re-initialize the C runtime "FILE" handles with clean handles bound to "nul". We do this because it has been
    // observed that the file number of our standard handle file objects can be assigned internally to a value of -2
    // when not bound to a valid target, which represents some kind of unknown internal invalid state. In this state our
    // call to "_dup2" fails, as it specifically tests to ensure that the target file number isn't equal to this value
    // before allowing the operation to continue. We can resolve this issue by first "re-opening" the target files to
    // use the "nul" device, which will place them into a valid state, after which we can redirect them to our target
    // using the "_dup2" function.
    if (bindStdIn) {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "r", stdin);
    }
    if (bindStdOut) {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "w", stdout);
    }
    if (bindStdErr) {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "w", stderr);
    }

    // Redirect unbuffered stdin from the current standard input handle
    if (bindStdIn) {
        RedirectHandle(STD_INPUT_HANDLE, stdin, "r");
    }

    // Redirect unbuffered stdout to the current standard output handle
    if (bindStdOut) {
        RedirectHandle(STD_OUTPUT_HANDLE, stdout, "w");
    }

    // Redirect unbuffered stderr to the current standard error handle
    if (bindStdErr) {
        RedirectHandle(STD_ERROR_HANDLE, stderr, "w");
    }

    // Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
    // standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
    // versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
    // has been read from or written to the targets or not.
    if (bindStdIn) {
        std::wcin.clear();
        std::cin.clear();
    }
    if (bindStdOut) {
        std::wcout.clear();
        std::cout.clear();
    }
    if (bindStdErr) {
        std::wcerr.clear();
        std::cerr.clear();
    }
}

bool AttachWindowsConsole()
{
    auto stdinType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
    auto stdoutType = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
    auto stderrType = GetFileType(GetStdHandle(STD_ERROR_HANDLE));

    bool bindStdIn = false;
    bool bindStdOut = false;
    bool bindStdErr = false;

    if (stdinType == FILE_TYPE_CHAR || stdinType == FILE_TYPE_UNKNOWN) {
        bindStdIn = true;
    }
    if (stdoutType == FILE_TYPE_CHAR || stdoutType == FILE_TYPE_UNKNOWN) {
        bindStdOut = true;
    }
    if (stderrType == FILE_TYPE_CHAR || stderrType == FILE_TYPE_UNKNOWN) {
        bindStdErr = true;
    }

    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        BindCrtHandlesToStdHandles(bindStdIn, bindStdOut, bindStdErr);
        return true;
    }

    return false;
}

std::error_code EnableAnsiSupport()
{
    // ref: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
    // Using `CreateFileW("CONOUT$", ...)` to retrieve the console handle works correctly even if STDOUT and/or STDERR are redirected
    HANDLE console_handle = CreateFileW(L"CONOUT$", FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (console_handle == INVALID_HANDLE_VALUE) {
        return std::error_code(GetLastError(), std::system_category());
    }

    // ref: https://docs.microsoft.com/en-us/windows/console/getconsolemode
    DWORD console_mode;
    if (0 == GetConsoleMode(console_handle, &console_mode)) {
        return std::error_code(GetLastError(), std::system_category());
    }

    // VT processing not already enabled?
    if ((console_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        // https://docs.microsoft.com/en-us/windows/console/setconsolemode
        if (0 == SetConsoleMode(console_handle, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            return std::error_code(GetLastError(), std::system_category());
        }
    }

    return {};
}
