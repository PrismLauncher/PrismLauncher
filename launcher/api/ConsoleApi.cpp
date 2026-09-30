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

#include "ConsoleApi.h"

#include "Application.h"
#include "MessageLevel.h"
#include "launch/LaunchTask.h"
#include "launch/LogModel.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "InstanceApi.h"

namespace api {

namespace {

const QString LauncherKey = QStringLiteral("\x01launcher");
constexpr int FlushIntervalMs = 50;
constexpr int MaxBatch = 500;
constexpr qint64 MaxChunk = 5000;

QString levelName(int level)
{
    switch (static_cast<MessageLevelValue>(level)) {
        case MessageLevelValue::StdOut:
            return "stdout";
        case MessageLevelValue::StdErr:
            return "stderr";
        case MessageLevelValue::Launcher:
            return "launcher";
        case MessageLevelValue::Trace:
            return "trace";
        case MessageLevelValue::Debug:
            return "debug";
        case MessageLevelValue::Info:
            return "info";
        case MessageLevelValue::Message:
            return "message";
        case MessageLevelValue::Warning:
            return "warning";
        case MessageLevelValue::Error:
            return "error";
        case MessageLevelValue::Fatal:
            return "fatal";
        default:
            return "unknown";
    }
}

QJsonObject line(const LogModel& model, int row, qint64 n)
{
    const auto index = model.index(row);
    auto text = model.data(index, Qt::DisplayRole).toString();
    if (text.endsWith('\n')) {
        text.chop(1);
    }
    return { { "n", static_cast<double>(n) }, { "level", levelName(model.data(index, LogModel::LevelRole).toInt()) }, { "text", text } };
}

}  // namespace

ConsoleApi::ConsoleApi(ApiRouter* router, InstanceApi* instances, QObject* parent) : QObject(parent), m_router(router)
{
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(FlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &ConsoleApi::flush);
    connect(instances, &InstanceApi::launchTaskChanged, this, &ConsoleApi::attach);

    if (APPLICATION->logModel) {
        // Not owned: the Application keeps its log model for its whole lifetime.
        attachModel(LauncherKey, shared_qobject_ptr<LogModel>(QSharedPointer<LogModel>(APPLICATION->logModel.get(), [](LogModel*) {})), false);
    }

    router->addSync("console.get", [this](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        return chunk(instance->id(), params::optionalInt(p, "from", 0, 0, std::numeric_limits<int>::max()),
                     params::optionalInt(p, "limit", MaxChunk, 1, MaxChunk));
    });
    router->addSync("console.launcherLog", [this](const QJsonObject& p) {
        auto out = chunk(LauncherKey, params::optionalInt(p, "from", 0, 0, std::numeric_limits<int>::max()),
                         params::optionalInt(p, "limit", MaxChunk, 1, MaxChunk));
        out.remove("instanceId");
        return out;
    });
}

void ConsoleApi::attach(const QString& instanceId, LaunchTask* task)
{
    if (!task) {
        return;
    }
    attachModel(instanceId, task->getLogModel(), true);
}

void ConsoleApi::attachModel(const QString& key, const shared_qobject_ptr<LogModel>& model, bool live)
{
    // Line numbers continue across launches, so the UI never mistakes a new launch's lines for old ones.
    qint64 previous = 0;
    if (auto it = m_streams.find(key); it != m_streams.end()) {
        previous = it->appended;
        if (it->model) {
            disconnect(it->model.get(), nullptr, this, nullptr);
        }
    }
    Stream stream;
    stream.model = model;
    stream.live = live;
    stream.appended = previous + (model ? model->rowCount() : 0);
    m_streams.insert(key, stream);
    if (!model) {
        return;
    }
    connect(model.get(), &QAbstractItemModel::rowsInserted, this, [this, key](const QModelIndex&, int first, int last) {
        auto it = m_streams.find(key);
        if (it == m_streams.end()) {
            return;
        }
        for (int row = first; row <= last; row++) {
            const auto n = it->appended++;
            if (it->live) {
                it->pending.append(line(*it->model, row, n));
            }
        }
        if (it->pending.size() >= MaxBatch) {
            flush();
        } else if (!it->pending.isEmpty() && !m_flushTimer.isActive()) {
            m_flushTimer.start();
        }
    });
    connect(model.get(), &QAbstractItemModel::modelReset, this, [this, key] {
        if (auto it = m_streams.find(key); it != m_streams.end()) {
            it->pending = QJsonArray();
        }
    });
}

void ConsoleApi::flush()
{
    for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
        if (it->pending.isEmpty()) {
            continue;
        }
        m_router->emitEvent("minecraft.log", QJsonObject{ { "instanceId", it.key() }, { "lines", it->pending } });
        it->pending = QJsonArray();
    }
}

QJsonObject ConsoleApi::chunk(const QString& key, qint64 from, qint64 limit) const
{
    const auto it = m_streams.constFind(key);
    if (it == m_streams.constEnd() || !it->model) {
        return { { "instanceId", key }, { "lines", QJsonArray() }, { "next", 0 }, { "available", false } };
    }
    const auto& model = *it->model;
    const qint64 rows = model.rowCount();
    const qint64 first = it->appended - rows;  // number of the oldest line still in the ring buffer
    const qint64 start = std::max(from, first);
    const qint64 end = std::min(it->appended, start + limit);
    QJsonArray lines;
    for (qint64 n = start; n < end; n++) {
        lines.append(line(model, static_cast<int>(n - first), n));
    }
    return { { "instanceId", key }, { "lines", lines }, { "next", static_cast<double>(std::max(end, start)) }, { "available", true } };
}

}  // namespace api
