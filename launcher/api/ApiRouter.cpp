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

#include "ApiRouter.h"

#include <QDebug>
#include <QJsonDocument>
#include <QLoggingCategory>

namespace api {

namespace {
Q_LOGGING_CATEGORY(apiLog, "launcher.webui.api")

constexpr qsizetype MaxMessageSize = 16 * 1024 * 1024;
}  // namespace

ApiReply::ApiReply(ApiRouter* router, qint64 id) : m_state(std::make_shared<State>(State{ router, id }))
{}

void ApiReply::resolve(const QJsonValue& result) const
{
    if (m_state->done) {
        return;
    }
    m_state->done = true;
    if (m_state->router) {
        m_state->router->sendResult(m_state->id, result);
    }
}

void ApiReply::reject(const ApiError& error) const
{
    if (m_state->done) {
        return;
    }
    m_state->done = true;
    if (m_state->router) {
        m_state->router->sendError(m_state->id, error);
    }
}

ApiRouter::ApiRouter(QObject* parent) : QObject(parent) {}

void ApiRouter::add(const QString& method, Handler handler)
{
    Q_ASSERT_X(!m_handlers.contains(method), "ApiRouter::add", "method registered twice");
    m_handlers.insert(method, std::move(handler));
}

void ApiRouter::addSync(const QString& method, SyncHandler handler)
{
    add(method, [handler = std::move(handler)](const QJsonObject& params, const ApiReply& reply) {
        reply.guard([&] { reply.resolve(handler(params)); });
    });
}

QStringList ApiRouter::methods() const
{
    auto list = m_handlers.keys();
    list.sort();
    return list;
}

void ApiRouter::handleMessage(const QByteArray& message)
{
    if (message.size() > MaxMessageSize) {
        qCWarning(apiLog) << "Dropped oversized message from the web UI:" << message.size() << "bytes";
        return;
    }
    QJsonParseError parseError{};
    const auto doc = QJsonDocument::fromJson(message, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(apiLog) << "Dropped malformed message from the web UI:" << parseError.errorString();
        return;
    }
    const auto obj = doc.object();
    if (obj.value("kind").toString() != QLatin1String("call") || !obj.value("id").isDouble()) {
        qCWarning(apiLog) << "Dropped message with unknown kind from the web UI";
        return;
    }
    const auto id = static_cast<qint64>(obj.value("id").toDouble());
    const auto method = obj.value("method").toString();
    const auto paramsValue = obj.value("params");
    if (!paramsValue.isUndefined() && !paramsValue.isNull() && !paramsValue.isObject()) {
        sendError(id, ApiError::invalidParams("params must be an object"));
        return;
    }

    const auto it = m_handlers.constFind(method);
    if (it == m_handlers.constEnd()) {
        qCWarning(apiLog) << "Web UI called unknown method" << method;
        sendError(id, ApiError("UNKNOWN_METHOD", QStringLiteral("Unknown method '%1'").arg(method)));
        return;
    }

    qCDebug(apiLog) << "call" << id << method;
    const ApiReply reply(this, id);
    reply.guard([&] { (*it)(paramsValue.toObject(), reply); });
}

void ApiRouter::emitEvent(const QString& name, const QJsonValue& payload)
{
    const QJsonObject msg{ { "kind", "event" }, { "name", name }, { "payload", payload } };
    emit outgoing(QJsonDocument(msg).toJson(QJsonDocument::Compact));
}

void ApiRouter::sendResult(qint64 id, const QJsonValue& result)
{
    const QJsonObject msg{ { "kind", "result" }, { "id", id }, { "ok", true }, { "result", result } };
    emit outgoing(QJsonDocument(msg).toJson(QJsonDocument::Compact));
}

void ApiRouter::sendError(qint64 id, const ApiError& error)
{
    if (error.code == QLatin1String("INTERNAL_ERROR")) {
        qCWarning(apiLog) << "Web UI call" << id << "failed internally:" << error.message;
    } else {
        qCDebug(apiLog) << "call" << id << "rejected:" << error.code << error.message;
    }
    const QJsonObject msg{ { "kind", "result" }, { "id", id }, { "ok", false }, { "error", error.toJson() } };
    emit outgoing(QJsonDocument(msg).toJson(QJsonDocument::Compact));
}

}  // namespace api
