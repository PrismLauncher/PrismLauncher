#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>

#include "Exception.h"

struct ApplicationMessage {
    QString command;
    QHash<QString, QString> args;

    QByteArray serialize() const;
    Result<> parse(const QByteArray& input);
};
