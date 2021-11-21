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

#pragma once
#include <QString>
#include <QDateTime>
#include <QIcon>

enum IconType : unsigned
{
    Builtin,
    Transient,
    FileBased,
    ICONS_TOTAL,
    ToBeDeleted
};

struct MMCImage
{
    QIcon icon;
    QString key;
    QString filename;
    bool present() const
    {
        return !icon.isNull() || !key.isEmpty();
    }
};

struct MMCIcon
{
    QString m_key;
    QString m_name;
    MMCImage m_images[ICONS_TOTAL];
    IconType m_current_type = ToBeDeleted;

    IconType type() const;
    QString name() const;
    bool has(IconType _type) const;
    QIcon icon() const;
    void remove(IconType rm_type);
    void replace(IconType new_type, QIcon icon, QString path = QString());
    void replace(IconType new_type, const QString &key);
    bool isBuiltIn() const;
    QString getFilePath() const;
};
