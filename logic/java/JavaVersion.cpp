#include "JavaVersion.h"
#include <MMCStrings.h>

#include <QRegularExpression>
#include <QString>

JavaVersion & JavaVersion::operator=(const QString & javaVersionString)
{
	string = javaVersionString;

	auto getCapturedInteger = [](const QRegularExpressionMatch & match, const QString &what) -> int
	{
		auto str = match.captured(what);
		if(str.isEmpty())
		{
			return 0;
		}
		return str.toInt();
	};

	QRegularExpression pattern;
	if(javaVersionString.startsWith("1."))
	{
		pattern = QRegularExpression ("1[.](?<major>[0-9]+)([.](?<minor>[0-9]+))?(_(?<security>[0-9]+)?)?(-(?<prerelease>[a-zA-Z0-9]+))?");
	}
	else
	{
		pattern = QRegularExpression("(?<major>[0-9]+)([.](?<minor>[0-9]+))?([.](?<security>[0-9]+))?(-(?<prerelease>[a-zA-Z0-9]+))?");
	}

	auto match = pattern.match(string);
	parseable = match.hasMatch();
	major = getCapturedInteger(match, "major");
	minor = getCapturedInteger(match, "minor");
	security = getCapturedInteger(match, "security");
	prerelease = match.captured("prerelease");
	return *this;
}

JavaVersion::JavaVersion(const QString &rhs)
{
	operator=(rhs);
}

QString JavaVersion::toString()
{
	return string;
}

bool JavaVersion::requiresPermGen()
{
	if(parseable)
	{
		return major < 8;
	}
	return true;
}

bool JavaVersion::operator<(const JavaVersion &rhs)
{
	if(parseable && rhs.parseable)
	{
		if(major < rhs.major)
			return true;
		if(major > rhs.major)
			return false;
		if(minor < rhs.minor)
			return true;
		if(minor > rhs.minor)
			return false;
		if(security < rhs.security)
			return true;
		if(security > rhs.security)
			return false;

		// everything else being equal, consider prerelease status
		bool thisPre = !prerelease.isEmpty();
		bool rhsPre = !rhs.prerelease.isEmpty();
		if(thisPre && !rhsPre)
		{
			// this is a prerelease and the other one isn't -> lesser
			return true;
		}
		else if(!thisPre && rhsPre)
		{
			// this isn't a prerelease and the other one is -> greater
			return false;
		}
		else if(thisPre && rhsPre)
		{
			// both are prereleases - use natural compare...
			return Strings::naturalCompare(prerelease, rhs.prerelease, Qt::CaseSensitive) < 0;
		}
		// neither is prerelease, so they are the same -> this cannot be less than rhs
		return false;
	}
	else return Strings::naturalCompare(string, rhs.string, Qt::CaseSensitive) < 0;
}

bool JavaVersion::operator==(const JavaVersion &rhs)
{
	if(parseable && rhs.parseable)
	{
		return major == rhs.major && minor == rhs.minor && security == rhs.security && prerelease == rhs.prerelease;
	}
	return string == rhs.string;
}

bool JavaVersion::operator>(const JavaVersion &rhs)
{
	return (!operator<(rhs)) && (!operator==(rhs));
}
