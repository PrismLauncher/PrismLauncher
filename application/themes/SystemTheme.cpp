#include "SystemTheme.h"
#include <QApplication>
#include <QStyle>

SystemTheme::SystemTheme()
{
	systemPalette = QApplication::style()->standardPalette();
}

QString SystemTheme::id()
{
	return "system";
}

QString SystemTheme::name()
{
	return QObject::tr("System");
}

QPalette SystemTheme::colorScheme()
{
	return systemPalette;
}

QString SystemTheme::appStyleSheet()
{
	return QString();
}
