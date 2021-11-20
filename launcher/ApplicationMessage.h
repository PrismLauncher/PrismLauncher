#pragma once

#include <QString>
#include <QMap>
#include <QByteArray>

struct ApplicationMessage {
    QString command;
    QMap<QString, QString> args;

    QByteArray serialize();
    void parse(const QByteArray & input);
};
