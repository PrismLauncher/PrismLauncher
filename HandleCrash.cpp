/* Copyright 2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// This is the Unix implementation of MultiMC's crash handling system.
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <MultiMC.h>

#if defined Q_OS_UNIX
#include <sys/utsname.h>
#include <execinfo.h>
#elif defined Q_OS_WIN32
#include <windows.h>
#include <dbghelp.h>
#include <WinBacktrace.h>
#endif

#include "BuildConfig.h"

#include "HandleCrash.h"

// The maximum number of frames to include in the backtrace.
#define BT_SIZE 20


#define DUMPF_NAME_FMT "mmc-crash-%X.bm" // Black magic? Bowel movement? Dump?
//                      1234567890  1234
// The maximum number of digits in a unix timestamp when encoded in hexadecimal is about 17.
// Our format string is ~14 characters long.
// The maximum length of the dump file's filename should be well over both of these. 42 is a good number.
#define DUMPF_NAME_LEN 42

// {{{ Platform hackery

#if defined Q_OS_UNIX

struct CrashData
{
	int signal = 0;
};

// This has to be declared here, after the CrashData struct, but before the function that uses it.
void handleCrash(CrashData);

void handler(int sig)
{
	CrashData cData;
	cData.signal = sig;
	handleCrash(cData);
}

#elif defined Q_OS_WIN32

// Struct for storing platform specific crash information.
// This gets passed into the generic handler, which will use
// it to access platform specific information.
struct CrashData
{
	EXCEPTION_RECORD* exceptionInfo;
	CONTEXT* context;
};

void handleCrash(CrashData);

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* eInfo)
{
	CrashData cData;
	cData.exceptionInfo = eInfo->ExceptionRecord;
	cData.context = eInfo->ContextRecord;
	handleCrash(cData);
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif

// }}}

// {{{ Handling

#ifdef Q_OS_WIN32
// #ThanksMicrosoft
// Blame Microsoft for this atrocity.
void dprintf(int fd, const char* fmt...)
{
	va_list args;
	va_start(args, fmt);
	char buffer[10240];
	// Just sprintf to a really long string and hope it works...
	// This is a hack, but I can't think of a better way to do it easily.
	int len = vsnprintf(buffer, 10240, fmt, args);
	printf(buffer, fmt, args);
	write(fd, buffer, len);
	va_end(args);
}
#endif

void getVsnType(char* out);
void readFromTo(int from, int to);

void dumpErrorInfo(int dumpFile, CrashData crash)
{
#ifdef Q_OS_UNIX
	// TODO: Moar unix
	dprintf(dumpFile, "Signal: %d\n", crash.signal);
#elif defined Q_OS_WIN32
	EXCEPTION_RECORD* excInfo = crash.exceptionInfo;

	dprintf(dumpFile, "Exception Code: %d\n", excInfo->ExceptionCode);
	dprintf(dumpFile, "Exception Address: 0x%0X\n", excInfo->ExceptionAddress);
#endif
}

void dumpMiscInfo(int dumpFile)
{
	char vsnType[42]; // The version type. If it's more than 42 chars, the universe might implode...

	// Get MMC info.
	getVsnType(vsnType);

	// Get MMC info.
	getVsnType(vsnType);

	dprintf(dumpFile, "MultiMC Version: %s\n", BuildConfig.VERSION_CSTR);
	dprintf(dumpFile, "MultiMC Version Type: %s\n", vsnType);
}

void dumpBacktrace(int dumpFile, CrashData crash)
{
#ifdef Q_OS_UNIX
	// Variables for storing crash info.
	void* trace[BT_SIZE]; 	// Backtrace frames
	size_t size; 			// The backtrace size

	// Get the backtrace.
	size = backtrace(trace, BT_SIZE);

	// Dump the backtrace
	dprintf(dumpFile, "---- BEGIN BACKTRACE ----\n");
	backtrace_symbols_fd(trace, size, dumpFile);
	dprintf(dumpFile, "---- END BACKTRACE ----\n");
#elif defined Q_OS_WIN32
	dprintf(dumpFile, "---- BEGIN BACKTRACE ----\n");

	StackFrame stack[BT_SIZE];
	size_t size;

	SYMBOL_INFO *symbol;
	HANDLE process;

	size = getBacktrace(stack, BT_SIZE, *crash.context);

	// FIXME: Accessing heap memory is supposedly "dangerous",
	// but I can't find another way of doing this.

	// Initialize
	process = GetCurrentProcess();
	if (!SymInitialize(process, NULL, true))
	{
		dprintf(dumpFile, "Failed to initialize symbol handler. Can't print stack trace.\n");
		dprintf(dumpFile, "Here's a list of addresses in the call stack instead:\n");
		for(int i = 0; i < size; i++)
		{
			dprintf(dumpFile, "0x%0X\n", (DWORD64)stack[i].address);
		}
	} else {
		// Allocate memory... ._.
		symbol = (SYMBOL_INFO *) calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		// Dump stacktrace
		for(int i = 0; i < size; i++)
		{
			DWORD64 addr = (DWORD64)stack[i].address;
			if (!SymFromAddr(process, (DWORD64)(addr), 0, symbol))
				dprintf(dumpFile, "?? - 0x%0X\n", addr);
			else
				dprintf(dumpFile, "%s - 0x%0X\n", symbol->Name, symbol->Address);
		}

		free(symbol);
	}

	dprintf(dumpFile, "---- END BACKTRACE ----\n");
#endif
}

void dumpSysInfo(int dumpFile)
{
#ifdef Q_OS_UNIX
	utsname sysinfo;			// System information

	// Dump system info
	if (uname(&sysinfo) >= 0)
	{
		dprintf(dumpFile, "OS System: %s\n", sysinfo.sysname);
		dprintf(dumpFile, "OS Machine: %s\n", sysinfo.machine);
		dprintf(dumpFile, "OS Release: %s\n", sysinfo.release);
		dprintf(dumpFile, "OS Version: %s\n", sysinfo.version);
	} else {
		dprintf(dumpFile, "OS System: Unknown Unix");
	}
#else
	// TODO: Get more information here.
	dprintf(dumpFile, "OS System: Windows");
#endif
}

void dumpLogs(int dumpFile)
{
	int otherFile;

	// Attempt to attach the log file if the logger was initialized.
	dprintf(dumpFile, "---- BEGIN LOGS ----\n");
	if (loggerInitialized)
	{
		otherFile = open("MultiMC-0.log", O_RDONLY);
		readFromTo(otherFile, dumpFile);
	} else {
		dprintf(dumpFile, "Logger not initialized.\n");
	}
	dprintf(dumpFile, "---- END LOGS ----\n");
}

// The signal handler. If this function is called, it means shit has probably collided with some sort of device one might use to keep oneself cool.
// This is the generic part of the code that will be called after platform specific handling is finished.
void handleCrash(CrashData crash)
{
#ifdef Q_OS_UNIX
	fprintf(stderr, "Fatal error! Received signal %d\n", crash.signal);
#endif

	time_t unixTime = 0;		// Unix timestamp. Used to give our crash dumps "unique" names.

	char dumpFileName[DUMPF_NAME_LEN]; // The name of the file we're dumping to.
	int dumpFile; // File descriptor for our dump file.

	// Determine what our dump file should be called.
	// We'll just call it "mmc-crash-<unixtime>.dump"
	// First, check the time.
	time(&unixTime);

	// Now we get to do some !!FUN!! hackery to ensure we don't use the stack when we convert
	// the timestamp from an int to a string. To do this, we just allocate a fixed size array
	// of chars on the stack, and sprintf into it. We know the timestamp won't ever be longer
	// than a certain number of digits, so this should work just fine.
	// sprintf doesn't support writing signed values as hex, so this breaks on negative timestamps.
	// It really shouldn't matter, though...
	sprintf(dumpFileName, DUMPF_NAME_FMT, unixTime);

	// Now, we need to open the file.
	// Fail if it already exists. This should never happen.
	dumpFile = open(dumpFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);


	if (dumpFile >= 0)
	{
		// If we opened the dump file successfully.
		// Dump everything we can and GTFO.
		fprintf(stderr, "Dumping crash report to %s\n", dumpFileName);

		// Dump misc info
		dprintf(dumpFile, "Unix Time: %d\n", unixTime);
		dumpErrorInfo(dumpFile, crash);
		dumpMiscInfo(dumpFile);

		dprintf(dumpFile, "\n");

		dumpSysInfo(dumpFile);

		dprintf(dumpFile, "\n");

		dumpBacktrace(dumpFile, crash);

		dprintf(dumpFile, "\n");

		// DIE DIE DIE!
		exit(1);
	}
	else
	{
		fprintf(stderr, "Failed to open dump file %s to write crash info (ERRNO: %d)\n", dumpFileName, errno);
		exit(2);
	}
}


// Reads data from the file descriptor on the first argument into the second argument.
void readFromTo(int from, int to)
{
	char buffer[1024];
	size_t lastread = 1;
	while (lastread > 0)
	{
		lastread = read(from, buffer, 1024);
		if (lastread > 0) write(to, buffer, lastread);
	}
}

// Writes the current version type to the given char buffer.
void getVsnType(char* out)
{
	switch (BuildConfig.versionTypeEnum)
	{
		case Config::Release:
			sprintf(out, "Release");
			break;
		case Config::ReleaseCandidate:
			sprintf(out, "ReleaseCandidate");
			break;
		case Config::Development:
			sprintf(out, "Development");
			break;
		default:
			sprintf(out, "Unknown");
			break;
	}
}

// }}}

// {{{ Misc

#if defined TEST_SEGV
// Causes a crash. For testing.
void testCrash()
{
	char* lol = (char*)MMC->settings().get();
	lol -= 8;

	// Throw shit at the fan.
	for (int i = 0; i < 8; i++)
		lol[i] = 'f';
}
#endif

// Initializes the Unix crash handler.
void initBlackMagic()
{
#ifdef Q_OS_UNIX
	// Register the handler.
	signal(SIGSEGV, handler);
	signal(SIGABRT, handler);
#elif defined Q_OS_WIN32
	// I hate Windows
	SetUnhandledExceptionFilter(ExceptionFilter);
#endif

#ifdef TEST_SEGV
	testCrash();
#endif
}

// }}}

