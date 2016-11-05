#pragma once

#include "ITheme.h"

class FusionTheme: public ITheme
{
public:
	virtual ~FusionTheme() {}

	QString qtTheme() override;

protected:
	QPalette fadeInactive(QPalette in, qreal bias, QColor color);
};
