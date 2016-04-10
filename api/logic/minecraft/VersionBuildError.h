#include "Exception.h"

class VersionBuildError : public Exception
{
public:
	explicit VersionBuildError(QString cause) : Exception(cause) {}
	virtual ~VersionBuildError() noexcept
	{
	}
};

/**
 * the base version file was meant for a newer version of the vanilla launcher than we support
 */
class LauncherVersionError : public VersionBuildError
{
public:
	LauncherVersionError(int actual, int supported)
		: VersionBuildError(QObject::tr(
			  "The base version file of this instance was meant for a newer (%1) "
			  "version of the vanilla launcher than this version of MultiMC supports (%2).")
								.arg(actual)
								.arg(supported)) {};
	virtual ~LauncherVersionError() noexcept
	{
	}
};

/**
 * some patch was intended for a different version of minecraft
 */
class MinecraftVersionMismatch : public VersionBuildError
{
public:
	MinecraftVersionMismatch(QString fileId, QString mcVersion, QString parentMcVersion)
		: VersionBuildError(QObject::tr("The patch %1 is for a different version of Minecraft "
										"(%2) than that of the instance (%3).")
								.arg(fileId)
								.arg(mcVersion)
								.arg(parentMcVersion)) {};
	virtual ~MinecraftVersionMismatch() noexcept
	{
	}
};

/**
 * files required for the version are not (yet?) present
 */
class VersionIncomplete : public VersionBuildError
{
public:
	VersionIncomplete(QString missingPatch)
		: VersionBuildError(QObject::tr("Version is incomplete: missing %1.")
								.arg(missingPatch)) {};
	virtual ~VersionIncomplete() noexcept
	{
	}
};
