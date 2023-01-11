#pragma once

#include "ITheme.h"

class FusionTheme : public ITheme {
   public:
    ~FusionTheme() override = default;

    void apply(bool initial) override;
    QString qtTheme() override;

    static const QString USE_FUSION_QML_GLOBAL_THEME;
};
