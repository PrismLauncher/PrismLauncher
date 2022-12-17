#pragma once

#include <QString>
#include "settings/SettingsObject.h"

class MCEditTool
{
public:
    MCEditTool(SettingsObjectPtr settings);
    void setPath(QString & path);
    QString path() const;
    bool check(const QString &toolPath, QString &error);
    QString getProgramPath();
private:
    SettingsObjectPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings;
};
