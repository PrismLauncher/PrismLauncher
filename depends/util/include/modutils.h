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
		explicit Section(const QString &fullString)
		{
			m_fullString = fullString;
			int cutoff = m_fullString.size();
			for(int i = 0; i < m_fullString.size(); i++)
			{
				if(!m_fullString[i].isDigit())
				{
					cutoff = i;
					break;
				}
			}
			auto numPart = m_fullString.leftRef(cutoff);
			if(numPart.size())
			{
				numValid = true;
				m_numPart = numPart.toInt();
			}
			auto stringPart = m_fullString.midRef(cutoff);
			if(stringPart.size())
			{
				m_stringPart = stringPart.toString();
			}
		}
		explicit Section() {}
		bool numValid = false;
		int m_numPart = 0;
		QString m_stringPart;
		QString m_fullString;

		inline bool operator!=(const Section &other) const
		{
			if(numValid && other.numValid)
			{
				return m_numPart != other.m_numPart || m_stringPart != other.m_stringPart;
			}
			else
			{
				return m_fullString != other.m_fullString;
			}
		}
		inline bool operator<(const Section &other) const
		{
			if(numValid && other.numValid)
			{
				if(m_numPart < other.m_numPart)
					return true;
				if(m_numPart == other.m_numPart && m_stringPart < other.m_stringPart)
					return true;
				return false;
			}
			else
			{
				return m_fullString < other.m_fullString;
			}
		}
		inline bool operator>(const Section &other) const
		{
			if(numValid && other.numValid)
			{
				if(m_numPart > other.m_numPart)
					return true;
				if(m_numPart == other.m_numPart && m_stringPart > other.m_stringPart)
					return true;
				return false;
			}
			else
			{
				return m_fullString > other.m_fullString;
			}
		}
	};
	QList<Section> m_sections;

	void parse();
};

LIBUTIL_EXPORT bool versionIsInInterval(const QString &version, const QString &interval);
LIBUTIL_EXPORT bool versionIsInInterval(const Version &version, const QString &interval);
}

