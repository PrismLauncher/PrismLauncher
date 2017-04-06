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
