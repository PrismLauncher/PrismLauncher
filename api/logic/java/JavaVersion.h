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

private:
	QString string;
	int major = 0;
	int minor = 0;
	int security = 0;
	bool parseable = false;
	QString prerelease;
};
