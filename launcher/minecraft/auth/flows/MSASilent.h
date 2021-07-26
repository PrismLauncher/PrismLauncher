#pragma once
#include "AuthContext.h"

class MSASilent : public AuthContext
{
    Q_OBJECT
public:
    explicit MSASilent(AccountData * data, QObject *parent = 0);
    void executeTask() override;
};
