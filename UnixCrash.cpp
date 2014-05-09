// This is the Unix implementation of MultiMC's crash handling system.
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <MultiMC.h>

#include "BuildConfig.h"

#include "HandleCrash.h"

// The maximum number of frames to include in the backtrace.
#define BT_SIZE 20


#define DUMPF_NAME_FMT "mmc-crash-%X.dump\0"
//                      1234567890  123456
// The maximum number of digits in a unix timestamp when encoded in hexadecimal is about 17.
// Our format string is ~16 characters long.
// The maximum length of the dump file's filename should be well over both of these. 42 is a good number.
#define DUMPF_NAME_LEN 42

// {{{ Handling

void getVsnType(char* out);
void readFromTo(int from, int to);

// The signal handler. If this function is called, it means shit has probably collided with some sort of device one might use to keep oneself cool.
void handler(int sig)
{
	// Variables for storing crash info.
	void* trace[BT_SIZE]; 	// Backtrace frames
	size_t size; 			// The backtrace size

	bool gotSysInfo = false;	// True if system info check succeeded
	utsname sysinfo;			// System information

	time_t unixTime = 0;		// Unix timestamp. Used to give our crash dumps "unique" names.

	char dumpFileName[DUMPF_NAME_LEN]; // The name of the file we're dumping to.
	int dumpFile; // File descriptor for our dump file.

	char vsnType[42]; // The version type. If it's more than 42 chars, the universe might implode...

	int otherFile; // File descriptor for other things.


	fprintf(stderr, "Fatal error! Received signal %d\n", sig);


	// Get the backtrace.
	size = backtrace(trace, BT_SIZE);


	// Get system info.
	gotSysInfo = uname(&sysinfo) >= 0;


	// Get MMC info.
	getVsnType(vsnType);


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
		dprintf(dumpFile, "MultiMC Version: %s\n", BuildConfig.VERSION_CSTR);
		dprintf(dumpFile, "MultiMC Version Type: %s\n", vsnType);
		dprintf(dumpFile, "Signal: %d\n", sig);

		// Dump system info
		if (gotSysInfo)
		{
			dprintf(dumpFile, "OS System: %s\n", sysinfo.sysname);
			dprintf(dumpFile, "OS Machine: %s\n", sysinfo.machine);
			dprintf(dumpFile, "OS Release: %s\n", sysinfo.release);
			dprintf(dumpFile, "OS Version: %s\n", sysinfo.version);
		} else {
			dprintf(dumpFile, "OS System: Unknown Unix");
		}

		dprintf(dumpFile, "\n");

		// Dump the backtrace
		dprintf(dumpFile, "---- BEGIN BACKTRACE ----\n");
		backtrace_symbols_fd(trace, size, dumpFile);
		dprintf(dumpFile, "---- END BACKTRACE ----\n");

		dprintf(dumpFile, "\n");

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

		// DIE DIE DIE!
		exit(1);
	}
	else
	{
		fprintf(stderr, "Failed to open dump file %s to write crash info (%d). Here's a backtrace on stderr instead.\n", dumpFileName, errno);
		// If we can't dump to the file, dump a backtrace to stderr and give up.
		backtrace_symbols_fd(trace, size, STDERR_FILENO);
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
	// Register the handler.
	signal(SIGSEGV, handler);
	signal(SIGABRT, handler);

#ifdef TEST_SEGV
	testCrash();
#endif
}

// }}}

