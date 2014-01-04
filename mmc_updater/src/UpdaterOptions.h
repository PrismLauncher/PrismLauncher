#pragma once

#include "UpdateInstaller.h"

/** Parses the command-line options to the updater binary. */
class UpdaterOptions
{
	public:
		UpdaterOptions();

		void parse(int argc, char** argv);

		UpdateInstaller::Mode mode;
		std::string installDir;
		std::string packageDir;
		std::string scriptPath;
		std::string finishCmd;
		PLATFORM_PID waitPid;
		std::string logFile;
		bool showVersion;
		bool dryRun;
		bool forceElevated;
		bool autoClose;
};

