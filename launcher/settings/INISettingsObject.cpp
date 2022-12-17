/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "INISettingsObject.h"
#include "Setting.h"

#include <QDebug>
#include <QFile>

INISettingsObject::INISettingsObject(QStringList paths, QObject *parent)
    : SettingsObject(parent)
{
    auto first_path = paths.constFirst();
    for (auto path : paths) {
        if (!QFile::exists(path))
            continue;

        if (path != first_path && QFile::exists(path)) {
            // Copy the fallback to the preferred path.
            QFile::copy(path, first_path);
            qDebug() << "Copied settings from" << path << "to" << first_path;
            break;
        }
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filePath = first_path;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.loadFile(first_path);
}

INISettingsObject::INISettingsObject(QString path, QObject* parent)
    : SettingsObject(parent)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filePath = path;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.loadFile(path);
}

void INISettingsObject::setFilePath(const QString &filePath)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filePath = filePath;
}

bool INISettingsObject::reload()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.loadFile(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filePath) && SettingsObject::reload();
}

void INISettingsObject::suspendSave()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspendSave = true;
}

void INISettingsObject::resumeSave()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspendSave = false;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doSave)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.saveFile(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filePath);
    }
}

void INISettingsObject::changeSetting(const Setting &setting, QVariant value)
{
    if (contains(setting.id()))
    {
        // valid value -> set the main config, remove all the sysnonyms
        if (value.isValid())
        {
            auto list = setting.configKeys();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.set(list.takeFirst(), value);
            for(auto iter: list)
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.remove(iter);
        }
        // invalid -> remove all (just like resetSetting)
        else
        {
            for(auto iter: setting.configKeys())
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.remove(iter);
        }
        doSave();
    }
}

void INISettingsObject::doSave()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspendSave)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doSave = true;
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.saveFile(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filePath);
    }
}

void INISettingsObject::resetSetting(const Setting &setting)
{
    // if we have the setting, remove all the synonyms. ALL OF THEM
    if (contains(setting.id()))
    {
        for(auto iter: setting.configKeys())
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.remove(iter);
        doSave();
    }
}

QVariant INISettingsObject::retrieveValue(const Setting &setting)
{
    // if we have the setting, return value of the first matching synonym
    if (contains(setting.id()))
    {
        for(auto iter: setting.configKeys())
        {
            if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini.contains(iter))
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_ini[iter];
        }
    }
    return QVariant();
}
