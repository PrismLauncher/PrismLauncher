// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "MMCIcon.h"
#include <QFileInfo>
#include <QIcon>

IconType operator--(IconType& t, int)
{
    IconType temp = t;
    switch (t) {
        case IconType::Builtin:
            t = IconType::ToBeDeleted;
            break;
        case IconType::Transient:
            t = IconType::Builtin;
            break;
        case IconType::FileBased:
            t = IconType::Transient;
            break;
        default:
            break;
    }
    return temp;
}

IconType MMCIcon::type() const
{
    return m_current_type;
}

QString MMCIcon::name() const
{
    if (m_name.size())
        return m_name;
    return m_key;
}

bool MMCIcon::has(IconType _type) const
{
    return m_images[_type].present();
}

QIcon MMCIcon::icon() const
{
    if (m_current_type == IconType::ToBeDeleted)
        return QIcon();
    auto& icon = m_images[m_current_type].icon;
    if (!icon.isNull())
        return icon;
    // FIXME: inject this.
    return QIcon::fromTheme(m_images[m_current_type].key);
}

void MMCIcon::remove(IconType rm_type)
{
    m_images[rm_type].filename = QString();
    m_images[rm_type].icon = QIcon();
    for (auto iter = rm_type; iter != IconType::ToBeDeleted; iter--) {
        if (m_images[iter].present()) {
            m_current_type = iter;
            return;
        }
    }
    m_current_type = IconType::ToBeDeleted;
}

void MMCIcon::replace(IconType new_type, QIcon icon, QString path)
{
    if (new_type > m_current_type || m_current_type == IconType::ToBeDeleted) {
        m_current_type = new_type;
    }
    m_images[new_type].icon = icon;
    m_images[new_type].filename = path;
    m_images[new_type].key = QString();
}

void MMCIcon::replace(IconType new_type, const QString& key)
{
    if (new_type > m_current_type || m_current_type == IconType::ToBeDeleted) {
        m_current_type = new_type;
    }
    m_images[new_type].icon = QIcon();
    m_images[new_type].filename = QString();
    m_images[new_type].key = key;
}

QString MMCIcon::getFilePath() const
{
    if (m_current_type == IconType::ToBeDeleted) {
        return QString();
    }
    return m_images[m_current_type].filename;
}

bool MMCIcon::isBuiltIn() const
{
    return m_current_type == IconType::Builtin;
}
