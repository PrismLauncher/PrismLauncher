#pragma once

#include <QString>
#include <QHash>
#include <QByteArray>

struct ApplicationMessage {
    QString command;
    QHash<QString, QString> args;

    QByteArray serialize();
    void parse(const QByteArray & input);
};
