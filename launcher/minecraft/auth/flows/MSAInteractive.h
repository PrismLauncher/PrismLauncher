#pragma once
#include "AuthContext.h"

class MSAInteractive : public AuthContext
{
    Q_OBJECT
public:
    explicit MSAInteractive(AccountData * data, QObject *parent = 0);
    void executeTask() override;
};
