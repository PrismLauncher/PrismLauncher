#include "ITheme.h"
#include "rainbow.h"
#include <QStyleFactory>
#include <QDir>
#include "MultiMC.h"

void ITheme::apply(bool)
{
	QApplication::setStyle(QStyleFactory::create(qtTheme()));
	if(hasColorScheme())
	{
		QApplication::setPalette(colorScheme());
	}
	if(hasStyleSheet())
	{
		MMC->setStyleSheet(appStyleSheet());
	}
	else
	{
		MMC->setStyleSheet(QString());
	}
	QDir::setSearchPaths("theme", searchPaths());
}

QPalette ITheme::fadeInactive(QPalette in, qreal bias, QColor color)
{
	auto blend = [&in, bias, color](QPalette::ColorRole role)
	{
		QColor from = in.color(QPalette::Active, role);
		QColor blended = Rainbow::mix(from, color, bias);
		in.setColor(QPalette::Disabled, role, blended);
	};
	blend(QPalette::Window);
	blend(QPalette::WindowText);
	blend(QPalette::Base);
	blend(QPalette::AlternateBase);
	blend(QPalette::ToolTipBase);
	blend(QPalette::ToolTipText);
	blend(QPalette::Text);
	blend(QPalette::Button);
	blend(QPalette::ButtonText);
	blend(QPalette::BrightText);
	blend(QPalette::Link);
	blend(QPalette::Highlight);
	blend(QPalette::HighlightedText);
	return in;
}
