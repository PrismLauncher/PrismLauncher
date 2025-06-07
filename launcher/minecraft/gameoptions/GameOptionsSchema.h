#pragma once

#include <QVariant>
#include <vector>

struct GameOption {
    QString key;
    QVariant value;
    QString description;
    QVariant min;
    QVariant max;
    QVariantList values;
};

extern const std::vector<GameOption> globalDelegateMap;
