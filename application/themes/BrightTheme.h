#pragma once

#include "ITheme.h"

class BrightTheme: public ITheme
{
public:
	virtual ~BrightTheme() {}

	QString qtTheme() override;
	QString id() override;
	QString name() override;
	QString appStyleSheet() override;
	QPalette colorScheme() override;
};

