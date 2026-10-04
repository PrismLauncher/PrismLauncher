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

#include "crash_handler/CrashHandler.h"

#include <array>
#include <bit>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <cpptrace/basic.hpp>

#ifdef _MSC_VER
#include <errhandlingapi.h>
#include <excpt.h>
#include <fcntl.h>
#include <fileapi.h>
#include <io.h>
#include <pplwin.h>
#include <processthreadsapi.h>
#include <winnt.h>
#else
#include <sys/wait.h>
#include <cstring>
#ifdef __linux__
#include <sys/types.h>
#elifdef __FreeBSD__
#include <sys/thr.h>
#elifdef __NetBSD__
#include <lwp.h>
#elifdef __OpenBSD__
// <unistd.h> included in header
#elifdef __APPLE__
#include <pthread.h>
#endif
#endif

#ifdef CRASH_HANDLER_HAVE_SIGALTSTACK
#include <mman.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#endif

#include <cpptrace/exceptions.hpp>
#include <cpptrace/formatting.hpp>
#include <cpptrace/from_current.hpp>
#include <cpptrace/utils.hpp>
#include <utility>

namespace CrashHandler {

// Anon namespace = static aka internal linkage
namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, readability-identifier-naming)
std::mutex CURRENT_EXCEPTION_HANDLER_GUARD;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, readability-identifier-naming)
std::optional<CrashHandler> CURRENT_EXCEPTION_HANDLER;

// size of the buffer used for safe signal traces
constexpr std::size_t g_SAFE_TRACE_SIZE = 200;

}  // namespace

#ifdef _MSC_VER
// useful windows string functions
namespace {

template <typename TChar, typename TStringGetterFunc>
std::basic_string<TChar> getStringFromWindowsApi(TStringGetterFunc stringGetter, int initialSize = 0)
{
    if (initialSize <= 0) {
        initialSize = MAX_PATH;
    }

    std::basic_string<TChar> result(initialSize, 0);
    for (;;) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        auto length = stringGetter(&result[0], result.length());
        if (length == 0) {
            return std::basic_string<TChar>();
        }

        if (length < result.length() - 1) {
            result.resize(length);
            result.shrink_to_fit();
            return result;
        }

        result.resize(result.length() * 2);
    }
}

std::string wideToMultiByte(const std::wstring& wide)
{
    if (wide.empty()) {
        return {};
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring multiByteToWide(const std::string& str)
{
    if (str.empty()) {
        return {};
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);
    return result;
}

}  // namespace
#endif

#ifdef _MSC_VER
namespace {

LONG WINAPI handleException(EXCEPTION_POINTERS* exceptionInfo)
{
    {
        std::cerr << "called handleException()\n";

        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);
        if (CURRENT_EXCEPTION_HANDLER.has_value()) {
            auto code = exceptionInfo->ExceptionRecord->ExceptionCode;

            CrashContext ctx{
                .message = "Caught unhandled SEH exception",
                .skipFrames = 7,  // get back to the actual exception frame through SEH path
                .exceptionPointers = exceptionInfo,
                .exceptionCode = static_cast<int32_t>(code),
                .processId = GetCurrentProcessId(),
                .threadId = GetCurrentThreadId(),
            };
            CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

LONG WINAPI handleVectoredException(EXCEPTION_POINTERS* exceptionInfo)
{
    if (exceptionInfo->ExceptionRecord->ExceptionCode == STATUS_HEAP_CORRUPTION) {
        std::cerr << "called handleVectoredException() -> Heap Corruption\n";
        {
            std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);
            if (CURRENT_EXCEPTION_HANDLER.has_value()) {
                auto code = exceptionInfo->ExceptionRecord->ExceptionCode;

                CrashContext ctx{
                    .message = "Caught Vectored SEH exception with STATUS_HEAP_CORRUPTION",
                    .skipFrames = 1,
                    .exceptionPointers = exceptionInfo,
                    .exceptionCode = static_cast<int32_t>(code),
                    .processId = GetCurrentProcessId(),
                    .threadId = GetCurrentThreadId(),
                };

                CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void handleInvalidParameter(const wchar_t* /*expression*/,
                            const wchar_t* /*function*/,
                            const wchar_t* /*file*/,
                            unsigned int /*line*/,
                            uintptr_t /*pReserved*/)
{
    std::cerr << "called handleInvalidParameter()\n";
    {
        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);

        if (CURRENT_EXCEPTION_HANDLER.has_value()) {
            CrashContext ctx{
                .message = "Caught syscall with invalid parameter",
                .skipFrames = 1,
                .exceptionPointers = nullptr,
                .exceptionCode = 1,
                .processId = GetCurrentProcessId(),
                .threadId = GetCurrentThreadId(),
            };
            CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
        }
    }
}

void handlePurecall()
{
    std::cerr << "called handlePurecall()\n";
    {
        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);

        if (CURRENT_EXCEPTION_HANDLER.has_value()) {
            CrashContext ctx{
                .message = "Caught virtual purecall",
                .skipFrames = 1,
                .exceptionPointers = nullptr,
                .exceptionCode = 0,
                .processId = GetCurrentProcessId(),
                .threadId = GetCurrentThreadId(),
            };
            CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
        }
    }
}
}  // namespace

#endif  // ifdef _MSC_VER

namespace {

#ifndef _MSC_VER

TheadIdT getThreadId()
{
#ifdef __linux__
    return gettid();
#elifdef __FreeBSD__
    long tid;
    thr_self(&tid);
    return tid;
#elifdef __NetBSD__
    return _lwp_self();
#elifdef __OpenBSD__
    return getthrid();
#elifdef __APPLE__
    uint64_t tid{ 0 };
    pthread_threadid_np(nullptr, &tid);
    return tid;
#endif
}

#endif

[[noreturn]] void signalHandler(int sig)
{
    std::cerr << "called signalHandler()\n";
    {
        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);

        if (CURRENT_EXCEPTION_HANDLER.has_value()) {
            std::string_view msg;
            switch (sig) {
                case SIGSEGV: {
                    msg = "Caught SIGSEGV";
                    break;
                }
                case SIGABRT: {
                    msg = "Caught SIGABRT";
                    break;
                }
                case SIGILL: {
                    msg = "Caught SIGILL";
                    break;
                }
                case SIGFPE: {
                    msg = "Caught SIGFPE";
                    break;
                }
                default: {
                    msg = "Caught Unknown Signal";
                }
            }
            CrashContext ctx{
                .message = msg,
                .skipFrames = 2,  // libc + this
                .preferSafe = true,
#ifdef _MSC_VER
                .exceptionPointers = nullptr,
                .exceptionCode = sig,
                .processId = GetCurrentProcessId(),
                .threadId = GetCurrentThreadId(),
#else
                .processId = getpid(),
                .threadId = getThreadId(),
#endif  // _MSC_VER
            };
            CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
        }

        // no handler, exit
        ::std::exit(EXIT_FAILURE);
    }
}

void terminateHandler()
{
    std::cerr << "called terminate_handler()\n";
    {
        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);

        if (CURRENT_EXCEPTION_HANDLER.has_value()) {
            std::string_view msg = "Terminate called after throwing";
            try {
                auto ptr = ::std::current_exception();

                if (ptr == nullptr) {
                    /// no exception, exit normally
                    return;
                }

                ::std::rethrow_exception(ptr);
            } catch (cpptrace::exception&) {
                CrashContext ctx{
                    .message = msg,
                    .skipFrames = 2,  // libc++ + this
#ifdef _MSC_VER
                    .exceptionPointers = nullptr,
                    .processId = GetCurrentProcessId(),
                    .threadId = GetCurrentThreadId(),
#else
                    .processId = getpid(),
                    .threadId = getThreadId(),
#endif  // _MSC_VER
                };
                CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
            } catch (std::exception&) {
                CrashContext ctx{
                    .message = msg,
                    .skipFrames = 2,
#ifdef _MSC_VER
                    .exceptionPointers = nullptr,
                    .processId = GetCurrentProcessId(),
                    .threadId = GetCurrentThreadId(),
#else
                    .processId = getpid(),
                    .threadId = getThreadId(),
#endif  // _MSC_VER
                };
                CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
            } catch (...) {
                CrashContext ctx{
                    .message = msg,
                    .skipFrames = 2,
#ifdef _MSC_VER
                    .exceptionPointers = nullptr,
                    .processId = GetCurrentProcessId(),
                    .threadId = GetCurrentThreadId(),
#else
                    .processId = getpid(),
                    .threadId = getThreadId(),
#endif  // _MSC_VER
                };
                CURRENT_EXCEPTION_HANDLER->captureTraceAndExit(ctx);
            }
        }
    }
}

#ifdef CRASH_HANDLER_HAVE_SIGALTSTACK

constexpr ::std::size_t getStackSize()
{
    if (SIGSTKSZ > 16 * 1024) {
        return SIGSTKSZ;
    } else {
        return 16 * 1024;
    }
}

std::optional<CrashHandler::StackPair> install_sigaltstack()
{
    constexpr const ::std::size_t SIG_STACK_SIZE = getStackSize();

    stack_t oldStack{};
    if (0 != sigaltstack(nullptr, &oldStack)) {
        std::cerr << "failed to learn about sigaltstack: " << std::strerror(errno);
        return {};
    }

    if (((oldStack.ss_flags & SS_DISABLE) == 0) && (oldStack.ss_size >= SIG_STACK_SIZE)) {
        // we're good
        return {};
    }

    // otherwise we need to allocate our own for STACK_OVERFLOW reasons
    ::std::size_t guardSize = sysconf(_SC_PAGESIZE);
    ::std::size_t allocSize = guardSize + SIG_STACK_SIZE;

    auto ptr = mmap(nullptr, allocSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "failed to map a sigaltstack, out of memory";
        return {};
    }

    // prep stack with read/write memory and register it
    if (0 != mprotect(ptr, SIG_STACK_SIZE, PROT_READ | PROT_WRITE)) {
        std::cerr << "mprotect to sigaltstack memory failed: " << std::strerror(errno);
        return {};
    }
    stack_t newStack{
        .ss_sp = ptr,
        .ss_flags = 0,
        .ss_size = SIG_STACK_SIZE,
    };

    if (0 != sigaltstack(&newStack, nullptr)) {
        std::cerr << "failed to register new sigaltstack: " << std::strerror(errno);
        return {};
    }

    return CrashHandler::StackSave{
        .old = (oldStack.ss_flags & SS_DISABLE) != 0 ? oldStack : std::nullopt,
        .new = newStack,
    };
}

void restore_sigaltstack(::std::optional<CrashHandler::StackSave>& ss)
{
    if (ss.has_value()) {
        stack_t curStack{};
        if (-1 == sigaltstack(nullptr, &curStack)) {
            return;
        }

        if (curStack.ss_sp = ss->new.ss_sp) {
            if (ss->old.has_value()) {
                // restore olf stack if there was one
                if (-1 == sigaltstack(&(ss->old), nullptr) {
                    return;
                }
            } else {
                // restore default alt stask
                stack_t disable{
                    .ss_flags = SS_DISABLE,
                };
                if (-1 == sigaltstack(&disable, nullptr)) {
                    return;
                }
            }
        }

        if (0 != munmap(ss->new.ss_sp, ss->new.ss_size)) {
            std::cerr << "munmap failed during thread shutdown";
            return;
        }

        ss = ::std::nullopt;
    }
}

#endif

}  // namespace

void CrashHandler::attachHandlers() noexcept
{
#ifdef CRASH_HANDLER_HAVE_SIGALTSTACK
    m_stackSave = install_sigaltstack();
#endif

    m_previousTerminate = std::set_terminate(terminateHandler);

#ifdef _MSC_VER
    m_previousFilter = ::SetUnhandledExceptionFilter(handleException);
    m_previousInvalidParamHandler = ::_set_invalid_parameter_handler(handleInvalidParameter);
    m_previousPureCallHandler = ::_set_purecall_handler(handlePurecall);
    m_vectoredExceptionHandler = ::AddVectoredExceptionHandler(1, handleVectoredException);
#else

    m_previousSigSegVHandler = ::std::signal(SIGSEGV, signalHandler);
    m_previousSigAbrtHandler = ::std::signal(SIGABRT, signalHandler);
    m_previousSigIllHandler = ::std::signal(SIGILL, signalHandler);
    m_previousSigFpeHandler = ::std::signal(SIGFPE, signalHandler);
#endif
}

void CrashHandler::restorePreviousHandlers() noexcept
{
#ifdef CRASH_HANDLER_HAVE_SIGALTSTACK
    restore_sigaltstack(m_stackSave);
#endif

    ::std::set_terminate(m_previousTerminate);

    ::std::signal(SIGSEGV, m_previousSigSegVHandler);
    ::std::signal(SIGABRT, m_previousSigAbrtHandler);
    ::std::signal(SIGABRT, m_previousSigAbrtHandler);
    ::std::signal(SIGABRT, m_previousSigFpeHandler);

#ifdef _MSC_VER
    ::SetUnhandledExceptionFilter(m_previousFilter);
    ::_set_invalid_parameter_handler(m_previousInvalidParamHandler);
    ::_set_purecall_handler(m_previousPureCallHandler);
    if (m_vectoredExceptionHandler != nullptr) {
        ::RemoveVectoredExceptionHandler(m_vectoredExceptionHandler);
    }
#endif
}

CrashHandler::CrashHandler(CrashConfig&& cfg)
    : m_exePath(cfg.processExePath.value_or("")), m_crashHandlerFlag(std::move(cfg.crashHandlerFlag))
{
    auto _ = std::move(cfg);  // finish moving the cfg to make clang-tidy happy

    m_execSelfEnabled = !m_exePath.empty();

#ifdef _MSC_VER

    std::wstring cmdline = multiByteToWide(m_exePath);
    cmdline += L" " + multiByteToWide(m_crashHandlerFlag);
    m_cliCommand = cmdline;
#endif  // _MSC_VER
    attachHandlers();
}

CrashHandler::~CrashHandler()
{
    restorePreviousHandlers();
};

bool CrashHandler::execSelfEnabled() const
{
    return m_execSelfEnabled;
}

#ifdef _MSC_VER
namespace {

std::string_view msgFromExceptionCode(int32_t code)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:
            return "BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT:
            return "FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION:
            return "FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW:
            return "FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK:
            return "FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW:
            return "FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:
            return "IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:
            return "INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:
            return "INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:
            return "PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:
            return "SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:
            return "STACK_OVERFLOW";
        default:
            return "UNKNOWN";
    }
}

}  // namespace
#endif

namespace detail {

template <>
template <writable Out>
bool ObjectFrameHelper<cpptrace::frame_ptr>::writeObjectFrame(Out& out, cpptrace::frame_ptr& ptr)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    cpptrace::safe_object_frame frame{};
    cpptrace::get_safe_object_frame(ptr, &frame);
    try {
        if (!out.write(reinterpret_cast<char*>(&frame), sizeof(frame))) {
            std::cerr << "failed to write frame\n";
            return false;
        };
    } catch (const ::std::exception& e) {
        std::cerr << "failed to write frame: " << e.what() << "\n";
        return false;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return true;
}

template <>
bool ObjectFrameHelper<cpptrace::safe_object_frame>::readObjectFrame(std::istream& in, cpptrace::safe_object_frame& frame)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    try {
        if (!in.read(reinterpret_cast<char*>(&frame), sizeof(frame))) {
            std::cerr << "failed to read frame\n";
            return false;
        };
    } catch (const ::std::exception& e) {
        std::cerr << "failed to read frame: " << e.what() << "\n";
        return false;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return true;
}

template <>
template <writable Out>
bool ObjectFrameHelper<cpptrace::object_frame>::writeObjectFrame(Out& out, cpptrace::object_frame& frame)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    try {
        if (!out.write(reinterpret_cast<char*>(&frame.raw_address), sizeof(frame.raw_address))) {
            std::cerr << "failed to write raw_address\n";
            return false;
        }
        if (!out.write(reinterpret_cast<char*>(&frame.object_address), sizeof(frame.object_address))) {
            std::cerr << "failed to write object_address\n";
            return false;
        }
        std::size_t pathlen = frame.object_path.size();
        if (!out.write(reinterpret_cast<char*>(&pathlen), sizeof(std::size_t))) {
            std::cerr << "failed to write object_path length\n";
            return false;
        }
        if (!out.write(frame.object_path.data(), pathlen)) {
            std::cerr << "failed to write object_path\n";
            return false;
        }
    } catch (const ::std::exception& e) {
        std::cerr << "failed to write frame: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "failed to write frame: <unknow>\n";
        return false;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return true;
}

template <>
bool ObjectFrameHelper<cpptrace::object_frame>::readObjectFrame(std::istream& in, cpptrace::object_frame& frame)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    try {
        if (!in.read(reinterpret_cast<char*>(&frame.raw_address), sizeof(frame.raw_address))) {
            std::cerr << "failed to read raw_address\n";
            return false;
        }
        if (!in.read(reinterpret_cast<char*>(&frame.object_address), sizeof(frame.object_address))) {
            std::cerr << "failed to read object_address\n";
            return false;
        }
        std::size_t pathlen = 0;
        if (!in.read(reinterpret_cast<char*>(&pathlen), sizeof(std::size_t))) {
            std::cerr << "failed to read object_path length\n";
            return false;
        }
        // resize
        frame.object_path.resize(pathlen);
        if (!in.read(frame.object_path.data(), static_cast<std::streamsize>(pathlen))) {
            std::cerr << "failed to read object_path\n";
            return false;
        }
    } catch (const ::std::exception& e) {
        std::cerr << "failed to read frame: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "failed to read frame: <unknow>\n";
        return false;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return true;
}
}  // namespace detail

template <detail::writable Out>
bool CrashContext::writeTraceHeader(Out& out) const
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    try {
        if (!out.write(reinterpret_cast<const char*>(&processId), sizeof(processId))) {
            std::cerr << "failed to write process id\n";
            return false;
        }
        if (!out.write(reinterpret_cast<const char*>(&threadId), sizeof(threadId))) {
            std::cerr << "failed to write threadId id\n";
            return false;
        }
        std::size_t messageLen = message.size();
        if (!out.write(reinterpret_cast<const char*>(&messageLen), sizeof(std::size_t))) {
            std::cerr << "failed to write message length\n";
            return false;
        }
        if (!out.write(message.data(), messageLen)) {
            std::cerr << "failed to write message\n";
            return false;
        }
    } catch (const ::std::exception& e) {
        std::cerr << "failed to write trace header: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "failed to write trace header: <unknow>\n";
        return false;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return true;
}

bool TraceHeader::readTraceHeader(std::istream& in)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    try {
        if (!in.read(reinterpret_cast<char*>(&processId), sizeof(processId))) {
            std::cerr << "failed to read process id\n";
            return false;
        }
        if (!in.read(reinterpret_cast<char*>(&threadId), sizeof(threadId))) {
            std::cerr << "failed to read threadId id\n";
            return false;
        }
        std::size_t messageLen{ 0 };
        if (!in.read(reinterpret_cast<char*>(&messageLen), sizeof(std::size_t))) {
            std::cerr << "failed to read message length\n";
            return false;
        }
        // resize
        message.resize(messageLen);
        if (!in.read(message.data(), static_cast<std::streamsize>(messageLen))) {
            std::cerr << "failed to read message string\n";
            return false;
        }
    } catch (const ::std::exception& e) {
        std::cerr << "failed to read trace header: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "failed to read trace header: <unknow>\n";
        return false;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return true;
}

[[noreturn]] void CrashHandler::captureTraceAndExit(CrashContext& ctx)
{
    ctx.skipFrames += 1;
    cpptrace::stacktrace trace;

#ifdef _MSC_VER
    if (ctx.exceptionPointers != nullptr) {
        // tell cpptrace to collect a trace form this
        cpptrace::detail::maybe_collect_trace(ctx.exceptionPointers, EXCEPTION_EXECUTE_HANDLER);
    }
#endif

    if (ctx.preferSafe && cpptrace::can_signal_safe_unwind() && cpptrace::can_get_safe_object_frame()) {
        doSignalSafeObjectTrace(ctx);
    } else {
        // capture raw trace, may do bad memory stuff but oh well
        doObjectTrace(ctx);
    }
    std::_Exit(EXIT_FAILURE);
}

void CrashHandler::doSignalSafeObjectTrace(CrashContext& ctx)
{
    ctx.skipFrames += 1;
    std::array<cpptrace::frame_ptr, g_SAFE_TRACE_SIZE> buffer{};
    std::size_t count = cpptrace::safe_generate_raw_trace(buffer.data(), g_SAFE_TRACE_SIZE, ctx.skipFrames);

    std::span trace(buffer.data(), count);

    detail::PipedProcess process{};

#ifdef _MSC_VER
    if (process.open(m_cliCommand)) {
#else
    if (process.open(m_exePath, m_crashHandlerFlag)) {
#endif
        sendObjectTraceToProcess<cpptrace::frame_ptr>(ctx, process, trace);
    }

    process.close();
}

void CrashHandler::doObjectTrace(CrashContext& ctx)
{
    ctx.skipFrames += 1;
    cpptrace::object_trace trace = cpptrace::generate_object_trace(ctx.skipFrames);

    detail::PipedProcess process{};

#ifdef _MSC_VER
    if (process.open(m_cliCommand)) {
#else
    if (process.open(m_exePath, m_crashHandlerFlag)) {
#endif
        sendObjectTraceToProcess<cpptrace::object_frame>(ctx, process, trace.frames);
    }

    process.close();
}

template <detail::objectFrame FrameType>
void CrashHandler::sendObjectTraceToProcess(const CrashContext& ctx, detail::PipedProcess p, ::std::span<FrameType> trace)
{
    std::cerr << "sending trace to process\n";
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    if (!ctx.writeTraceHeader(p)) {
        std::cerr << "failed to write trace header\n";
        return;
    }
    detail::ObjectFrameTypeValue frameType = detail::ObjectFrameHelper<FrameType>::Type;
    if (!p.write(reinterpret_cast<char*>(&frameType), sizeof(frameType))) {
        std::cerr << "failed to write trace type\n";
        return;
    };
    ::std::size_t count = trace.size();
    if (!p.write(reinterpret_cast<char*>(&count), sizeof(count))) {
        std::cerr << "failed to write trace frame count\n";
        return;
    };
    for (FrameType f : trace) {
        if (!detail::ObjectFrameHelper<FrameType>::writeObjectFrame(p, f)) {
            std::cerr << "writing frame failed\n";
            return;
        }
    }
    std::cerr << "done\n";
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

namespace {

void warmupCpptrace()
{
    std::array<cpptrace::frame_ptr, 10> buffer{};
    cpptrace::safe_generate_raw_trace(buffer.data(), 10);
    cpptrace::safe_object_frame frame{};
    cpptrace::get_safe_object_frame(buffer.at(0), &frame);
}

}  // namespace

bool attach(CrashConfig&& cfg)
{
    {
        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);
        if (CURRENT_EXCEPTION_HANDLER.has_value()) {
            return false;
        }

        if (cpptrace::can_signal_safe_unwind()) {
            warmupCpptrace();
        }

        CURRENT_EXCEPTION_HANDLER.emplace(std::move(cfg));
    }
    return true;
}

void detach()
{
    {
        std::scoped_lock lock(CURRENT_EXCEPTION_HANDLER_GUARD);

        CURRENT_EXCEPTION_HANDLER.reset();
    }
}

namespace detail {

#ifdef _MSC_VER
bool PipedProcess::open(const ::std::wstring& startCmd)
#else
bool PipedProcess::open(const ::std::string& processPath, const ::std::string& args)
#endif
{
#ifdef _MSC_VER
    // windows can't fork, spawn a whole new process with a cli arg instead;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = 1;

    // Create one-way pipe for child process STDIN
    if (::CreatePipe(&inputPipe.read_end, &inputPipe.write_end, &sa, 0) == 0) {
        std::cerr << "CreatePipe error: " << GetLastError() << "\n";
        return false;
    }

    // Ensure write handle to pipe for STDIN is not inherited
    if (::SetHandleInformation(inputPipe.write_end, HANDLE_FLAG_INHERIT, 0) == 0) {
        std::cerr << "SetHandleInformation error: " << GetLastError() << "\n";
        return false;
    }

    si.cb = sizeof(STARTUPINFOW);
    si.hStdError = nullptr;
    si.hStdOutput = nullptr;
    si.hStdInput = inputPipe.read_end;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto* cliargs = const_cast<wchar_t*>(startCmd.c_str());
    int result = ::CreateProcessW(nullptr, cliargs, nullptr, nullptr, 1, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);

    if (result == 0) {
        std::cerr << "Failed to CreateProcess: " << GetLastError() << "\n";
        return false;
    }
#else
    // Setup pipe and spawn child
    if (::pipe(input_pipe.fd.data()) == -1) {
        const char* forkFailureMessage = "pipe() failed\n";
        ::write(STDERR_FILENO, forkFailureMessage, ::strlen(forkFailureMessage));
        return false;
    }
    pid = ::fork();
    if (pid == -1) {
        const char* forkFailureMessage = "fork() failed\n";
        ::write(STDERR_FILENO, forkFailureMessage, ::strlen(forkFailureMessage));
        return false;
    }
    if (pid == 0) {  // child
        ::dup2(input_pipe.read_end(), STDIN_FILENO);
        ::close(input_pipe.read_end());
        ::close(input_pipe.write_end());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        std::array<char*, 3> argv = { const_cast<char*>(processPath.c_str()), const_cast<char*>(args.c_str()), nullptr };
        ::execv(argv.at(0), argv.data());  // doesn't return replaces self
        // so if we reach here it's a failure
        const char* execFailureMessage = "exec(self) failed\n";
        ::write(STDERR_FILENO, execFailureMessage, ::strlen(execFailureMessage));
        ::_exit(1);
    }
#endif
    return true;
}

void PipedProcess::wait() const
{
#ifdef _MSC_VER
    ::WaitForSingleObject(pi.hProcess, 1000);
#else
    ::waitpid(pid, nullptr, 1000);
#endif  // _MSC_VER
}

void PipedProcess::close() const
{
#ifdef _MSC_VER
    ::FlushFileBuffers(inputPipe.write_end);
    ::CloseHandle(inputPipe.write_end);
#else
    ::close(input_pipe.read_end());
    ::close(input_pipe.write_end());
#endif

    wait();

#ifdef _MSC_VER
    ::CloseHandle(inputPipe.read_end);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
#endif  // _MSC_VER
}

bool PipedProcess::write(const char* buffer, ::std::size_t size) const
{
#ifdef _MSC_VER
    DWORD written = 0;
    BOOL writeSuccess = ::WriteFile(inputPipe.write_end, buffer, static_cast<DWORD>(size), &written, nullptr);
    if ((writeSuccess == 0) || written != size) {
        // NOLINTNEXTLINE
        return false;
    }
#else
    ssize_t written = ::write(input_pipe.write_end(), buffer, size);
    if (std::cmp_not_equal(written, size)) {
        return false;
    }
#endif  // _MSC_VER

    return true;
}

}  // namespace detail

CrashTrace readTraceFromStdin()
{
#ifdef _MSC_VER
    _setmode(_fileno(stdin), _O_BINARY);
#endif  // _MSC_VER

    std::cerr << "reading trace header \n";
    CrashTrace trace{};

    if (!trace.header.readTraceHeader(std::cin)) {
        std::cerr << "Failed to read trace header!\n";
        return trace;
    }

    std::cerr << "reading trace type \n";
    detail::ObjectFrameTypeValue traceType = detail::ObjectFrameTypeValue::Safe;
    // NOLINTBEGIN)cppcoreguidelines-pro-type-reinterpret-cast
    if (!std::cin.read(reinterpret_cast<char*>(&traceType), sizeof(traceType))) {
        std::cerr << "Failed to read trace type!\n";
        return trace;
    }
    switch (traceType) {
        case detail::ObjectFrameTypeValue::Safe: {
            std::cerr << "detected a safe_object_trace\n";
            break;
        }
        case detail::ObjectFrameTypeValue::Normal: {
            std::cerr << "detected a normal_object_trace\n";
            break;
        }
        default: {
            std::cerr << "unknown trace type: " << static_cast<int>(traceType) << "\n";
            return trace;
        }
    }

    std::cerr << "reading trace type \n";
    std::size_t frameCount;
    if (!std::cin.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount))) {
        std::cerr << "Failed to read trace frame count!\n";
        return trace;
    }
    std::cerr << "attempting to read " << frameCount << " object frames from stdin\n";
    // NOLINTEND

    cpptrace::object_trace objectTrace{
        .frames = {},
    };

    switch (traceType) {
        case detail::ObjectFrameTypeValue::Safe: {
            for (std::size_t i = 0; i < frameCount; i++) {
                cpptrace::safe_object_frame frame{};

                if (!detail::ObjectFrameHelper<cpptrace::safe_object_frame>::readObjectFrame(std::cin, frame)) {
                    std::cerr << "Something went wrong while reading from the pipe\n";
                    break;
                }
                objectTrace.frames.push_back(frame.resolve());
            }
            break;
        }
        case detail::ObjectFrameTypeValue::Normal: {
            for (std::size_t i = 0; i < frameCount; i++) {
                cpptrace::object_frame frame{};

                if (!detail::ObjectFrameHelper<cpptrace::object_frame>::readObjectFrame(std::cin, frame)) {
                    std::cerr << "Something went wrong while reading from the pipe\n";
                    break;
                }
                objectTrace.frames.push_back(frame);
            }
            break;
        }
        default: {
            std::cerr << "got a bad trace type: " << static_cast<unsigned int>(traceType) << "\n";
        }
    }

    if (objectTrace.frames.size() != frameCount) {
        std::cerr << "expected " << frameCount << " frames but received " << objectTrace.frames.size() << "\n";
    }
    std::cerr << "resolving " << objectTrace.frames.size() << " frames from object trace\n";
    trace.stacktrace = objectTrace.resolve();
    trace.valid = true;

    return trace;
}

}  // namespace CrashHandler
