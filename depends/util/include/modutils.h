#pragma once

#include <QString>
#include "libutil_config.h"

class QUrl;

namespace Util
{
struct Version
{
	Version(const QString &str);

	bool operator<(const Version &other) const;
	bool operator<=(const Version &other) const;
	bool operator>(const Version &other) const;
	bool operator==(const Version &other) const;
	bool operator!=(const Version &other) const;

	QString toString() const
	{
		return m_string;
	}

private:
	QString m_string;
};

LIBUTIL_EXPORT QUrl expandQMURL(const QString &in);
LIBUTIL_EXPORT bool versionIsInInterval(const QString &version, const QString &interval);
}

