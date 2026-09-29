// SPDX-FileCopyrightText: 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#pragma once

// crash-handler detection config
#include "crash_handler/config.h"

#include <cpptrace/basic.hpp>

#include <concepts>
#include <cpptrace/forward.hpp>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
#include <windows.h>
#include <errhandlingapi.h>
#include <winnt.h>
// clang-format on
#else
#include <unistd.h>
#endif

#ifdef CRASH_HANDLER_HAVE_SIGALTSTACK
#include <signal.h>
#endif

namespace CrashHandler {

// private impl detail
// NOLINTNEXTLINE(readability-identifier-naming)
namespace detail {

template <class T>
concept readable = requires(T r, char* in) {
    { !r.read(in, ::std::size_t()) } -> std::convertible_to<bool>;
};

template <class T>
concept writable = requires(T r, const char* out) {
    { !r.write(out, ::std::size_t()) } -> std::convertible_to<bool>;
};

struct PipedProcess {
#ifdef _MSC_VER
    PROCESS_INFORMATION pi{};
    STARTUPINFOW si{};
    SECURITY_ATTRIBUTES sa{};
    // NOLINTBEGIN(readability-identifier-naming)
    struct pipe_t {
        HANDLE read_end = nullptr;
        HANDLE write_end = nullptr;
    } inputPipe;
    // NOLINTEND(readability-identifier-naming)

#else
    // NOLINTBEGIN(readability-identifier-naming)
    struct pipe_t {
        std::array<int, 2> fd;
        int& read_end() { return fd.at(0); }
        int& write_end() { return fd.at(1); }
        const int& read_end() const { return fd.at(0); }
        const int& write_end() const { return fd.at(1); }
    } input_pipe;
    // NOLINTEND(readability-identifier-naming)
    pid_t pid;

#endif  // _MSC_VER

#ifdef _MSC_VER
    bool open(const ::std::wstring& startCmd);
#else
    bool open(const ::std::string& processPath, const ::std::string& args);
#endif

    void close() const;
    void wait() const;

    bool write(const char* buffer, ::std::size_t size) const;
};

enum class ObjectFrameTypeValue : ::std::uint8_t { Safe = 1, Normal };

template <typename T>
struct ObjectFrameType {};

template <>
struct ObjectFrameType<cpptrace::frame_ptr> {
    static constexpr ObjectFrameTypeValue Type = ObjectFrameTypeValue::Safe;
};
template <>
struct ObjectFrameType<cpptrace::safe_object_frame> {
    static constexpr ObjectFrameTypeValue Type = ObjectFrameTypeValue::Safe;
};

template <>
struct ObjectFrameType<cpptrace::object_frame> {
    static constexpr ObjectFrameTypeValue Type = ObjectFrameTypeValue::Normal;
};

template <typename T>
concept objectFrame = requires(T a, ObjectFrameTypeValue b) {
    { b = ObjectFrameType<T>::Type };
};

template <objectFrame T>
struct ObjectFrameHelper {
    static constexpr ObjectFrameTypeValue Type = ObjectFrameType<T>::Type;
    template <writable Out>
    static bool writeObjectFrame(Out& out, T&);
    template <readable In>
    static bool readObjectFrame(In& in, T&);
};

}  // namespace detail

struct CrashContext {
    std::string_view message;
    uint32_t skipFrames = 0;
    bool preferSafe = false;
#ifdef _MSC_VER
    EXCEPTION_POINTERS* exceptionPointers;
    int32_t exceptionCode = 0;
    uint32_t processId = 0;
    uint32_t threadId = 0;
#else
    pid_t processId = 0;
    pid_t threadId = 0;
#endif

    template <detail::writable Out>
    bool writeTraceHeader(Out& out) const;
};

struct TraceHeader {
    std::string message;
#ifdef _MSC_VER
    uint32_t processId;
    uint32_t threadId;
#else
    pid_t processId;
    pid_t threadId;
#endif

    template <detail::readable In>
    bool readTraceHeader(In& in);
};

// Config for a crash handler.  pass to `attach`
struct CrashConfig {
    // the flag passes to the program to signal an incoming crash trace
    ::std::string crashHandlerFlag = "--crash-handler";
    // full path to the executable, if not passed attempts to capture it in a platform dependant way
    ::std::optional<::std::string> processExePath = std::nullopt;
    // path to a directory where binary crash traces should be saved
    // if not set, traces are nto saved
    ::std::optional<::std::string> dataPath = std::nullopt;
    // base name of a crash trace full name will be "${base}${n?}.$[ext]".
    // typically "crash.trace" or "crash1.trace"
    ::std::string traceFileNameBase = "crash";
    // extention used for crash trace
    ::std::string traceFileNameExt = "trace";
};

// NOLINTNEXTLINE readability-identifier-naming
using signal_handler_t = void(int);

// the Crash handler. holds the saved signal handlers to restore on detach
// as well as config value to control where traces are sent
class CrashHandler {
   public:
    CrashHandler(CrashConfig&& cfg);
    ~CrashHandler();

   public:
    [[noreturn]] void captureTraceAndExit(CrashContext&);

    bool execSelfEnabled() const;
    const ::std::string& capturedExePath() const;
    void setDataPath(std::string&& path);

    // signal handlers
   private:
    /// POSIX signal handlers
    signal_handler_t* m_previousSigSegVHandler = nullptr;
    signal_handler_t* m_previousSigAbrtHandler = nullptr;
    signal_handler_t* m_previousSigIllHandler = nullptr;
    signal_handler_t* m_previousSigFpeHandler = nullptr;

    ::std::terminate_handler m_previousTerminate = nullptr;

#ifdef CRASH_HANDLER_HAVE_SIGALTSTACK
    // alt stack context to help with stack_overflow errors in signal handlers
    struct StackSave {
        ::std::optional<stack_t> old;
        stack_t new;
    };

    ::std::optional<StackSave> m_stackSave;
#endif  // CRASH_HANDLER_HAVE_SIGALTSTACK

#ifdef _MSC_VER
    // windows specific handlers that should be preferred to normal posix signals
    LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter = nullptr;
    _invalid_parameter_handler m_previousInvalidParamHandler = nullptr;
    _purecall_handler m_previousPureCallHandler = nullptr;
    PVOID m_vectoredExceptionHandler = nullptr;

#endif

    // in theory macos could have a mach port exceptions handler installed
    // but it's nigh impossible to get a trace form a mach port because
    // it's resolved by sending a message to a listening thread loop

   private:
    bool m_execSelfEnabled = false;

    // config values

    ::std::string m_exePath;
    ::std::string m_crashHandlerFlag;

    ::std::optional<::std::string> m_dataPath;
    ::std::string m_traceFileNameBase;
    ::std::string m_traceFileNameExt;

#ifdef _MSC_VER
    // pre computation of windows start cmd
    ::std::wstring m_cliCommand;
#endif  // _MSC_VER

   private:
    // attach the signal handlers
    void attachHandlers() noexcept;
    // restore saved handlers
    void restorePreviousHandlers() noexcept;

   private:
    // Trace functions:
    //
    // open a new process and pipe object trace
    // also save to configured dump file

    // for safe traces inside signal handlers.
    void doSignalSafeObjectTrace(CrashContext& ctx);
    // normal object trace
    void doObjectTrace(CrashContext& ctx);

   private:
    // save trace
    template <detail::objectFrame FrameType>
    void saveObjectTrace(const CrashContext& ctx, ::std::span<FrameType> trace);
    // send trace to crash handler
    template <detail::objectFrame FrameType>
    void sendObjectTraceToProcess(const CrashContext& ctx, detail::PipedProcess p, ::std::span<FrameType> trace);
};

// Attach the global handler
// Returns success if no previous handler was attached, otherwise does nothing and returns false.
bool attach(CrashConfig&& cfg);

// Detaches any global handler
void detach();

struct CrashTrace {
    bool valid{ false };
    TraceHeader header{};
    cpptrace::stacktrace stacktrace{};
};
// when running as a crash handler call this to read the crash trace frm stdin
CrashTrace readTraceFromStdin();

// set a data path for the current crash handler to save traces
void setDataPath(std::string&& path);
};  // namespace CrashHandler
