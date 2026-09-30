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
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QPointer>

#include <functional>
#include <memory>

#include "ApiError.h"

namespace api {

class ApiRouter;

/**
 * Completion handle for one RPC call. Copyable; the first resolve()/reject() wins, later ones are ignored.
 * Safe to keep across asynchronous work: if the router is gone the reply is dropped.
 */
class ApiReply {
   public:
    ApiReply(ApiRouter* router, qint64 id);

    void resolve(const QJsonValue& result = QJsonObject{ { "ok", true } }) const;
    void reject(const ApiError& error) const;

    /** Runs `fn`; turns an ApiError or std::exception thrown by it into reject(). */
    template <typename Fn>
    void guard(Fn&& fn) const;

   private:
    struct State {
        QPointer<ApiRouter> router;
        qint64 id;
        bool done = false;
    };
    std::shared_ptr<State> m_state;
};

/**
 * Dispatches JSON RPC messages from the web UI to explicitly registered, named handlers
 * and pushes events back. There is intentionally no way to call an unregistered method.
 */
class ApiRouter : public QObject {
    Q_OBJECT
   public:
    /** Asynchronous handler: must eventually resolve or reject `reply` (it may throw ApiError synchronously). */
    using Handler = std::function<void(const QJsonObject& params, const ApiReply& reply)>;
    /** Synchronous handler: returns the result or throws ApiError. */
    using SyncHandler = std::function<QJsonValue(const QJsonObject& params)>;

    explicit ApiRouter(QObject* parent = nullptr);

    void add(const QString& method, Handler handler);
    void addSync(const QString& method, SyncHandler handler);

    QStringList methods() const;

    /** Entry point for raw messages from the page. */
    void handleMessage(const QByteArray& message);

    /** Pushes `{ kind: "event", name, payload }` to the page. */
    void emitEvent(const QString& name, const QJsonValue& payload = QJsonObject{});

   signals:
    /** Serialised message that must be delivered to the page (connected to WebView::postMessage). */
    void outgoing(const QByteArray& json);

   private:
    friend class ApiReply;
    void sendResult(qint64 id, const QJsonValue& result);
    void sendError(qint64 id, const ApiError& error);

    QHash<QString, Handler> m_handlers;
};

template <typename Fn>
void ApiReply::guard(Fn&& fn) const
{
    try {
        fn();
    } catch (const ApiError& e) {
        reject(e);
    } catch (const std::exception& e) {
        reject(ApiError::internal(QString::fromUtf8(e.what())));
    }
}

}  // namespace api
