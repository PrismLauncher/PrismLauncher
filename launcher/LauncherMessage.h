#pragma once

#include <QString>
#include <QMap>
#include <QByteArray>

struct LauncherMessage {
    QString command;
    QMap<QString, QString> args;

    QByteArray serialize();
    void parse(const QByteArray & input);
};
