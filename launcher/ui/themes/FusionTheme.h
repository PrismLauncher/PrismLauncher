#pragma once

#include "ITheme.h"

class FusionTheme : public ITheme {
   public:
    virtual ~FusionTheme() = default;

    QString qtTheme() override;
};
