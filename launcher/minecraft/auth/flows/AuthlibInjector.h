#pragma once
#include "AuthFlow.h"

class AuthlibInjectorRefresh : public AuthFlow
{
    Q_OBJECT
public:
    explicit AuthlibInjectorRefresh(
        AccountData *data,
        QObject *parent = 0
    );
};

class AuthlibInjectorLogin : public AuthFlow
{
    Q_OBJECT
public:
    explicit AuthlibInjectorLogin(
        AccountData *data,
        QString password,
        QObject *parent = 0
    );

private:
    QString m_password;
};
