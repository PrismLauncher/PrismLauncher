#include "DarkTheme.h"

QString DarkTheme::id()
{
	return "dark";
}

QString DarkTheme::name()
{
	return QObject::tr("Dark");
}

QPalette DarkTheme::colorScheme()
{
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(49,54,59));
	darkPalette.setColor(QPalette::WindowText, Qt::white);
	darkPalette.setColor(QPalette::Base, QColor(35,38,41));
	darkPalette.setColor(QPalette::AlternateBase, QColor(49,54,59));
	darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
	darkPalette.setColor(QPalette::ToolTipText, Qt::white);
	darkPalette.setColor(QPalette::Text, Qt::white);
	darkPalette.setColor(QPalette::Button, QColor(49,54,59));
	darkPalette.setColor(QPalette::ButtonText, Qt::white);
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::HighlightedText, Qt::black);
	return fadeInactive(darkPalette, fadeAmount(),  fadeColor());
}

double DarkTheme::fadeAmount()
{
	return 0.5;
}

QColor DarkTheme::fadeColor()
{
	return QColor(49,54,59);
}

QString DarkTheme::appStyleSheet()
{
	return "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }";
}
