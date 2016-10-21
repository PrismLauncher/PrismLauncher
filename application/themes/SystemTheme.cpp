#include "SystemTheme.h"
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QDebug>

SystemTheme::SystemTheme()
{
	const auto & style = QApplication::style();
	systemPalette = style->standardPalette();
	QString lowerThemeName = style->objectName();
	qWarning() << systemTheme;
	QStringList styles = QStyleFactory::keys();
	for(auto &st: styles)
	{
		if(st.toLower() == lowerThemeName)
		{
			systemTheme = st;
			return;
		}
	}
	// fall back to fusion if we can't find the current theme.
	systemTheme = "Fusion";
}

QString SystemTheme::id()
{
	return "system";
}

QString SystemTheme::name()
{
	return QObject::tr("System");
}

QString SystemTheme::qtTheme()
{
	return systemTheme;
}

QPalette SystemTheme::colorScheme()
{
	return systemPalette;
}

QString SystemTheme::appStyleSheet()
{
	return QString();
}
