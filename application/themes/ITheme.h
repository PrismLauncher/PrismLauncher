#pragma once
#include <QString>
#include <QPalette>

class QStyle;

class ITheme
{
public:
    virtual ~ITheme() {}
    virtual void apply(bool initial);
    virtual QString id() = 0;
    virtual QString name() = 0;
    virtual bool hasStyleSheet() = 0;
    virtual QString appStyleSheet() = 0;
    virtual QString qtTheme() = 0;
    virtual bool hasColorScheme() = 0;
    virtual QPalette colorScheme() = 0;
    virtual QColor fadeColor() = 0;
    virtual double fadeAmount() = 0;
    virtual QStringList searchPaths()
    {
        return {};
    }

    static QPalette fadeInactive(QPalette in, qreal bias, QColor color);
};
