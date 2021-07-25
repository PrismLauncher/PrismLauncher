#pragma once

#include <QString>
#include <QStringList>

struct ModDetails
{
    QString mod_id;
    QString name;
    QString version;
    QString mcversion;
    QString homeurl;
    QString updateurl;
    QString description;
    QStringList authors;
    QString credits;
};
