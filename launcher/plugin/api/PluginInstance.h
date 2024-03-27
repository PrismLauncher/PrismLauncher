// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 */
#pragma once

#include <QFileInfo>
#include <QObject>
#include <QString>

class PluginInstance : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(PluginInstance)

   public:
    virtual ~PluginInstance() {}

    [[nodiscard]] QFileInfo fileinfo() const { return m_file_info; }
    [[nodiscard]] auto id() const -> QString { return m_id; }

    [[nodiscard]] virtual QString relativePath(QString path) const = 0;

   protected:
    explicit PluginInstance() {}

    QFileInfo m_file_info;
    QString m_id;
};
