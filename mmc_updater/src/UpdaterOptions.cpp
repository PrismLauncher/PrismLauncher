#include "UpdaterOptions.h"

#include "Log.h"
#include "AnyOption/anyoption.h"
#include "FileUtils.h"
#include "Platform.h"
#include "StringUtils.h"

#include <cstdlib>
#include <iostream>

UpdaterOptions::UpdaterOptions()
: mode(UpdateInstaller::Setup)
, waitPid(0)
, showVersion(false)
, forceElevated(false)
, autoClose(false)
{
}

UpdateInstaller::Mode stringToMode(const std::string& modeStr)
{
	if (modeStr == "main")
	{
		return UpdateInstaller::Main;
	}
	else
	{
		if (!modeStr.empty())
		{
			LOG(Error,"Unknown mode " + modeStr);
		}
		return UpdateInstaller::Setup;
	}
}

void UpdaterOptions::parse(int argc, char** argv)
{
	AnyOption parser;
	parser.setOption("install-dir");
	parser.setOption("package-dir");
	parser.setOption("finish-cmd");
	parser.setOption("script");
	parser.setOption("wait");
	parser.setOption("mode");
	parser.setFlag("version");
	parser.setFlag("force-elevated");
	parser.setFlag("dry-run");
	parser.setFlag("auto-close");

	parser.processCommandArgs(argc,argv);

	if (parser.getValue("mode"))
	{
		mode = stringToMode(parser.getValue("mode"));
	}
	if (parser.getValue("install-dir"))
	{
		installDir = parser.getValue("install-dir");
	}
	if (parser.getValue("package-dir"))
	{
		packageDir = parser.getValue("package-dir");
	}
	if (parser.getValue("script"))
	{
		scriptPath = parser.getValue("script");
	}
	if (parser.getValue("wait"))
	{
		waitPid = static_cast<PLATFORM_PID>(atoll(parser.getValue("wait")));
	}
	if (parser.getValue("finish-cmd"))
	{
		finishCmd = parser.getValue("finish-cmd");
	}

	showVersion = parser.getFlag("version");
	forceElevated = parser.getFlag("force-elevated");
	dryRun = parser.getFlag("dry-run");
	autoClose = parser.getFlag("auto-close");
}
