#include "BrightTheme.h"

#include <QObject>

QString BrightTheme::id()
{
    return "bright";
}

QString BrightTheme::name()
{
    return QObject::tr("Bright");
}

bool BrightTheme::hasColorScheme()
{
    return true;
}

QPalette BrightTheme::colorScheme()
{
    QPalette brightPalette;
    brightPalette.setColor(QPalette::Window, QColor(239,240,241));
    brightPalette.setColor(QPalette::WindowText, QColor(49,54,59));
    brightPalette.setColor(QPalette::Base, QColor(252,252,252));
    brightPalette.setColor(QPalette::AlternateBase, QColor(239,240,241));
    brightPalette.setColor(QPalette::ToolTipBase, QColor(49,54,59));
    brightPalette.setColor(QPalette::ToolTipText, QColor(239,240,241));
    brightPalette.setColor(QPalette::Text,  QColor(49,54,59));
    brightPalette.setColor(QPalette::Button, QColor(239,240,241));
    brightPalette.setColor(QPalette::ButtonText, QColor(49,54,59));
    brightPalette.setColor(QPalette::BrightText, Qt::red);
    brightPalette.setColor(QPalette::Link, QColor(41, 128, 185));
    brightPalette.setColor(QPalette::Highlight, QColor(61, 174, 233));
    brightPalette.setColor(QPalette::HighlightedText, QColor(239,240,241));
    return fadeInactive(brightPalette, fadeAmount(), fadeColor());
}

double BrightTheme::fadeAmount()
{
    return 0.5;
}

QColor BrightTheme::fadeColor()
{
    return QColor(239,240,241);
}

bool BrightTheme::hasStyleSheet()
{
    return false;
}

QString BrightTheme::appStyleSheet()
{
    return QString();
}

