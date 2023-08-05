#pragma once
#include "AuthFlow.h"

class MSAInteractive : public AuthFlow {
    Q_OBJECT
   public:
    explicit MSAInteractive(AccountData* data, QObject* parent = 0);
};

class MSASilent : public AuthFlow {
    Q_OBJECT
   public:
    explicit MSASilent(AccountData* data, QObject* parent = 0);
};
