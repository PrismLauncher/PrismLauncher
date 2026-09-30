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

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace api {

/**
 * Structured error returned to the web UI: `{ code, message, details? }`.
 * Codes are part of the contract, see `ApiErrorCode` in frontend/src/types/common.ts.
 *
 * API handlers may `throw ApiError(...)`; the router converts it into an error reply.
 */
struct ApiError {
    QString code;
    QString message;
    QJsonValue details = QJsonValue::Undefined;

    ApiError() = default;
    ApiError(QString code, QString message, QJsonValue details = QJsonValue::Undefined)
        : code(std::move(code)), message(std::move(message)), details(std::move(details))
    {}

    QJsonObject toJson() const
    {
        QJsonObject obj{ { "code", code }, { "message", message } };
        if (!details.isUndefined()) {
            obj.insert("details", details);
        }
        return obj;
    }

    static ApiError invalidParams(const QString& message) { return { "INVALID_PARAMS", message }; }
    static ApiError notFound(const QString& message) { return { "NOT_FOUND", message }; }
    static ApiError instanceNotFound(const QString& id) { return { "INSTANCE_NOT_FOUND", QStringLiteral("Instance '%1' does not exist").arg(id) }; }
    static ApiError accountNotFound(const QString& id) { return { "ACCOUNT_NOT_FOUND", QStringLiteral("Account '%1' does not exist").arg(id) }; }
    static ApiError internal(const QString& message) { return { "INTERNAL_ERROR", message }; }
    static ApiError io(const QString& message) { return { "IO_ERROR", message }; }
    static ApiError unsupported(const QString& message) { return { "UNSUPPORTED", message }; }
    static ApiError permissionDenied(const QString& message) { return { "PERMISSION_DENIED", message }; }
    static ApiError cancelled(const QString& message = QStringLiteral("Cancelled")) { return { "CANCELLED", message }; }
};

}  // namespace api
