#include "SystemTheme.h"
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QDebug>

SystemTheme::SystemTheme()
{
    qDebug() << "Determining System Theme...";
    const auto & style = QApplication::style();
    systemPalette = style->standardPalette();
    QString lowerThemeName = style->objectName();
    qDebug() << "System theme seems to be:" << lowerThemeName;
    QStringList styles = QStyleFactory::keys();
    for(auto &st: styles)
    {
        qDebug() << "Considering theme from theme factory:" << st.toLower();
        if(st.toLower() == lowerThemeName)
        {
            systemTheme = st;
            qDebug() << "System theme has been determined to be:" << systemTheme;
            return;
        }
    }
    // fall back to fusion if we can't find the current theme.
    systemTheme = "Fusion";
    qDebug() << "System theme not found, defaulted to Fusion";
}

void SystemTheme::apply(bool initial)
{
    // if we are applying the system theme as the first theme, just don't touch anything. it's for the better...
    if(initial)
    {
        return;
    }
    ITheme::apply(initial);
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
    return true;
}
