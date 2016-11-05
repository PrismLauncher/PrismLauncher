#pragma once

#include "FusionTheme.h"

class DarkTheme: public FusionTheme
{
public:
	virtual ~DarkTheme() {}

	QString id() override;
	QString name() override;
	QString appStyleSheet() override;
	QPalette colorScheme() override;
};
