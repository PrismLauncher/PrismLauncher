#pragma once

#include "FusionTheme.h"

class BrightTheme: public FusionTheme
{
public:
    virtual ~BrightTheme() {}

    QString id() override;
    QString name() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    bool hasColorScheme() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
};

