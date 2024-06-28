// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Leia uwu <leia@tutamail.com>
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
 */
#pragma once

#include <LoggedProcess.h>
#include <java/JavaChecker.h>
#include <launch/LaunchStep.h>
#include <QHostInfo>

class PrintServers : public LaunchStep {
    Q_OBJECT
   public:
    PrintServers(LaunchTask* parent, const QStringList& servers);

    virtual void executeTask();
    virtual bool canAbort() const;

   private:
    void resolveServer(const QHostInfo& host_info);
    QMap<QString, QString> m_server_to_address;
    QStringList m_servers;
};
