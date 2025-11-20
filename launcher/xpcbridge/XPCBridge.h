// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
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

#ifndef XPCBRIDGE_H
#define XPCBRIDGE_H

#include <sys/socket.h>

#include <QLocalServer>
#include <QSocketNotifier>

class XPCBridge : public QObject {
    Q_OBJECT
    int m_launcherSocket = -1;
    int m_gameSocket = -1;

    std::unique_ptr<QSocketNotifier> m_launcherNotifier = nullptr;
   private slots:
    void onReadyRead() const;

   public:
    XPCBridge(QObject* parent = nullptr);
    ~XPCBridge() override;
    [[nodiscard]] int getGameSocketDescriptor() const;
};

#endif  // XPCBRIDGE_H
