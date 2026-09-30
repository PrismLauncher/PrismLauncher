// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#include <QHash>
#include <QObject>
#include <QTimer>

#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/MinecraftAccount.h"

namespace api {

class ApiRouter;
class TaskTracker;

/**
 * `accounts.*`: list, Microsoft login (browser or device code flow), offline accounts, removal,
 * default account and refresh. The MSA flow runs the existing AuthFlow; the URL / device code it
 * produces are pushed as `account.login.*` events and shown by the web UI.
 */
class AccountApi : public QObject {
    Q_OBJECT
   public:
    AccountApi(ApiRouter* router, TaskTracker* tasks, QObject* parent = nullptr);

    /** Looks up an account by `MinecraftAccount::internalId()`. */
    static MinecraftAccountPtr findAccount(const QString& id);

   private:
    void registerMethods();
    QString startLogin(bool useDeviceCode);

    struct Flow {
        MinecraftAccountPtr account;
        shared_qobject_ptr<AuthFlow> task;
    };

    ApiRouter* m_router;
    TaskTracker* m_tasks;
    QTimer m_changedTimer;
    QHash<QString, Flow> m_flows;
    qint64 m_nextFlow = 1;
};

}  // namespace api
