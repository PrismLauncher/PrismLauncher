#pragma once

#include <QString>

struct LoggingSpec {
    QString argument;
    QString id;
    QString url;
    QString sha1;
    int size = 0;

    QString formatArgument(const QString& path) const { return QString(argument).replace("${path}", path); }
};
