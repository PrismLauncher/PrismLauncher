#pragma once
#include "AuthFlow.h"

class OfflineLogin : public AuthFlow {
    Q_OBJECT
   public:
    explicit OfflineLogin(AccountData* data, QObject* parent = 0);
};
