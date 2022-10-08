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
    brightPalette.setColor(QPalette::Window, QColor(255,255,255));
    brightPalette.setColor(QPalette::WindowText, QColor(17,17,17));
    brightPalette.setColor(QPalette::Base, QColor(250,250,250));
    brightPalette.setColor(QPalette::AlternateBase, QColor(240,240,240));
    brightPalette.setColor(QPalette::ToolTipBase, QColor(17,17,17));
    brightPalette.setColor(QPalette::ToolTipText, QColor(255,255,255));
    brightPalette.setColor(QPalette::Text,  Qt::black);
    brightPalette.setColor(QPalette::Button, QColor(249,249,249));
    brightPalette.setColor(QPalette::ButtonText, Qt::black);
    brightPalette.setColor(QPalette::BrightText, Qt::red);
    brightPalette.setColor(QPalette::Link, QColor(37,137,164));
    brightPalette.setColor(QPalette::Highlight, QColor(137,207,84));
    brightPalette.setColor(QPalette::HighlightedText, Qt::black);
    return fadeInactive(brightPalette, fadeAmount(), fadeColor());
}

double BrightTheme::fadeAmount()
{
    return 0.5;
}

QColor BrightTheme::fadeColor()
{
    return QColor(255,255,255);
}

bool BrightTheme::hasStyleSheet()
{
    return false;
}

QString BrightTheme::appStyleSheet()
{
    return QString();
}

