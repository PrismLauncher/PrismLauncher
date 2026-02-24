#pragma once

#include <QString>
#include "settings/SettingsObject.h"

class MCEditTool {
   public:
    MCEditTool(SettingsObject* settings);
    void setPath(QString& path);
    QString path() const;
    static bool check(const QString& toolPath, QString& error);
    QString getProgramPath() const;

   private:
    SettingsObject* m_settings;
};
