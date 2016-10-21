#pragma once

#include "ITheme.h"

class SystemTheme: public ITheme
{
public:
	SystemTheme();
	virtual ~SystemTheme() {}

	QString id() override;
	QString name() override;
	QString qtTheme() override;
	QString appStyleSheet() override;
	QPalette colorScheme() override;
private:
	QPalette systemPalette;
	QString systemTheme;
};

