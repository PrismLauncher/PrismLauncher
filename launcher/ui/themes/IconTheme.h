#pragma once

#include <QString>

class IconTheme {
   public:
    IconTheme(const QString& id, const QString& path);

    bool load();
    QString id();
    QString path();
    QString name();

   private:
    QString m_id;
    QString m_path;
    QString m_name;
};