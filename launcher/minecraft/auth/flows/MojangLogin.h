#pragma once
#include "AuthContext.h"

class MojangLogin : public AuthContext
{
    Q_OBJECT
public:
    explicit MojangLogin(AccountData * data, QString password, QObject *parent = 0);
    void executeTask() override;

private:
    QString m_password;
};
