#pragma once
#include "AuthFlow.h"

class CustomYggdrasilRefresh : public AuthFlow
{
    Q_OBJECT
public:
    explicit CustomYggdrasilRefresh(
        AccountData *data,
        QObject *parent = 0
    );
};

class CustomYggdrasilLogin : public AuthFlow
{
    Q_OBJECT
public:
    explicit CustomYggdrasilLogin(
        AccountData *data,
        QString password,
        QObject *parent = 0
    );

private:
    QString m_password;
};
