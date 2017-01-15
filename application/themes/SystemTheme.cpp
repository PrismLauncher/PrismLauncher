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
	qWarning() << "System theme not found, defaulted to Fusion";
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

double SystemTheme::fadeAmount()
	{
		return 0.5;
	}

QColor SystemTheme::fadeColor()
	{
		return QColor(128,128,128);
	}

bool SystemTheme::hasStyleSheet()
{
	return false;
}

bool SystemTheme::hasColorScheme()
{
	// FIXME: horrible hack to work around Qt's sketchy theming APIs
#if defined(Q_OS_LINUX)
	return true;
#else
	return false;
#endif
}
