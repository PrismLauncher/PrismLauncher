#pragma once
#include "AuthContext.h"

class MojangRefresh : public AuthContext
{
    Q_OBJECT
public:
    explicit MojangRefresh(AccountData *data, QObject *parent = 0);
    void executeTask() override;
};
