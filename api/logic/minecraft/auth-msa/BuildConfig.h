#pragma once
#include <QString>

class Config
{
public:
    Config();
    QString CLIENT_ID;
};

extern const Config BuildConfig;
