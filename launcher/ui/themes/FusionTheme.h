#pragma once

#include "ITheme.h"

class FusionTheme : public ITheme {
   public:
    ~FusionTheme() override = default;

    QString qtTheme() override;
};
