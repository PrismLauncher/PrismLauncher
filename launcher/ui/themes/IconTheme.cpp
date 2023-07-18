#include "IconTheme.h"

#include <QFile>
#include <QSettings>

IconTheme::IconTheme(const QString& id, const QString& path) : m_id(id), m_path(path) {}

bool IconTheme::load()
{
    QString path = m_path + "/index.theme";

    if (!QFile::exists(path))
        return false;

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("Icon Theme");
    m_name = settings.value("Name").toString();
    settings.endGroup();
    return !m_name.isNull();
}

QString IconTheme::id()
{
    return m_id;
}

QString IconTheme::path()
{
    return m_path;
}

QString IconTheme::name()
{
    return m_name;
}