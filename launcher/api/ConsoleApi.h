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
#include <QJsonArray>
#include <QObject>
#include <QTimer>

#include "QObjectPtr.h"

class LaunchTask;
class LogModel;

namespace api {

class ApiRouter;
class InstanceApi;

/**
 * `console.get` / `console.launcherLog` and the `minecraft.log` event stream.
 *
 * Follows the LogModel of each instance's current LaunchTask. LogModel is a ring buffer, so every
 * line gets a monotonic number `n` that survives wrap-around and later launches; the UI fetches the backlog
 * once and then appends batched events (flushed every 50 ms or 500 lines) instead of polling.
 */
class ConsoleApi : public QObject {
    Q_OBJECT
   public:
    ConsoleApi(ApiRouter* router, InstanceApi* instances, QObject* parent = nullptr);

   private:
    struct Stream {
        shared_qobject_ptr<LogModel> model;
        qint64 appended = 0;  // number of the next line; numbering continues across LogModels of the same key
        QJsonArray pending;   // lines not yet pushed as an event
        bool live = true;
    };

    void attach(const QString& instanceId, LaunchTask* task);
    void attachModel(const QString& key, const shared_qobject_ptr<LogModel>& model, bool live);
    void flush();
    QJsonObject chunk(const QString& key, qint64 from, qint64 limit) const;

    ApiRouter* m_router;
    QHash<QString, Stream> m_streams;  // key: instance id, or LauncherKey
    QTimer m_flushTimer;
};

}  // namespace api
