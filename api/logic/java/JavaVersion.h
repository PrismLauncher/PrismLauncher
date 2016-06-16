#pragma once

#include "multimc_logic_export.h"
#include <QString>

class MULTIMC_LOGIC_EXPORT JavaVersion
{
	friend class JavaVersionTest;
public:
	JavaVersion() {};
	JavaVersion(const QString & rhs);

	JavaVersion & operator=(const QString & rhs);

	bool operator<(const JavaVersion & rhs);
	bool operator==(const JavaVersion & rhs);
	bool operator>(const JavaVersion & rhs);

	bool requiresPermGen();

	QString toString();

	int major()
	{
		return m_major;
	}
	int minor()
	{
		return m_minor;
	}
	int security()
	{
		return m_security;
	}
private:
	QString m_string;
	int m_major = 0;
	int m_minor = 0;
	int m_security = 0;
	bool m_parseable = false;
	QString m_prerelease;
};
