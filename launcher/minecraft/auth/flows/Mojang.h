#pragma once
#include "AuthFlow.h"

class MojangRefresh : public AuthFlow {
    Q_OBJECT
   public:
    explicit MojangRefresh(AccountData* data, QObject* parent = 0);
};

class MojangLogin : public AuthFlow {
    Q_OBJECT
   public:
    explicit MojangLogin(AccountData* data, QString password, QObject* parent = 0);

   private:
    QString m_password;
};
