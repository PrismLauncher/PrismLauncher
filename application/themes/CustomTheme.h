#pragma once

#include "ITheme.h"

class CustomTheme: public ITheme
{
public:
	CustomTheme(ITheme * baseTheme, QString folder);
	virtual ~CustomTheme() {}

	QString id() override;
	QString name() override;
	bool hasStyleSheet() override;
	QString appStyleSheet() override;
	bool hasColorScheme() override;
	QPalette colorScheme() override;
	double fadeAmount() override;
	QColor fadeColor() override;
	QString qtTheme() override;
	QStringList searchPaths() override;

private: /* data */
	QPalette m_palette;
	QColor m_fadeColor;
	double m_fadeAmount;
	QString m_styleSheet;
	QString m_name;
	QString m_id;
	QString m_widgets;
};

