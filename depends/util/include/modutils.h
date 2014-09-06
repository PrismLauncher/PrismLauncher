#pragma once

#include <QString>
#include <QList>

#include "libutil_config.h"

class QUrl;

namespace Util
{
struct Version
{
	Version(const QString &str);
	Version() {}

	bool operator<(const Version &other) const;
	bool operator<=(const Version &other) const;
	bool operator>(const Version &other) const;
	bool operator>=(const Version &other) const;
	bool operator==(const Version &other) const;
	bool operator!=(const Version &other) const;

	QString toString() const
	{
		return m_string;
	}

private:
	QString m_string;
	struct Section
	{
		explicit Section(const QString &str, const int num) : numValid(true), number(num), string(str) {}
		explicit Section(const QString &str) : numValid(false), string(str) {}
		explicit Section() {}
		bool numValid;
		int number;
		QString string;

		inline bool operator!=(const Section &other) const
		{
			return (numValid && other.numValid) ? (number != other.number) : (string != other.string);
		}
		inline bool operator<(const Section &other) const
		{
			return (numValid && other.numValid) ? (number < other.number) : (string < other.string);
		}
		inline bool operator>(const Section &other) const
		{
			return (numValid && other.numValid) ? (number > other.number) : (string > other.string);
		}
	};
	QList<Section> m_sections;

	void parse();
};

LIBUTIL_EXPORT bool versionIsInInterval(const QString &version, const QString &interval);
LIBUTIL_EXPORT bool versionIsInInterval(const Version &version, const QString &interval);
}

