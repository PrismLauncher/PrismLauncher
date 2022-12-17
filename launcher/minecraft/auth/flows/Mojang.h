#pragma once
#include "AuthFlow.h"

class MojangRefresh : public AuthFlow
{
    Q_OBJECT
public:
    explicit MojangRefresh(
        AccountData *data,
        QObject *parent = 0
    );
};

class MojangLogin : public AuthFlow
{
    Q_OBJECT
public:
    explicit MojangLogin(
        AccountData *data,
        QString password,
        QObject *parent = 0
    );

private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_password;
};
