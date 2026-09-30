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

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

#include <limits>
#include <optional>

#include "ApiError.h"

/**
 * Typed accessors for RPC parameters. Every accessor validates the JSON type and throws
 * ApiError(INVALID_PARAMS) otherwise, so handlers never act on malformed input.
 */
namespace api::params {

inline QJsonValue field(const QJsonObject& obj, const QString& key)
{
    return obj.value(key);
}

inline bool isMissing(const QJsonValue& v)
{
    return v.isUndefined() || v.isNull();
}

inline QString requireString(const QJsonObject& obj, const QString& key, qsizetype maxLength = 4096)
{
    const auto v = field(obj, key);
    if (!v.isString()) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must be a string").arg(key));
    }
    auto s = v.toString();
    if (s.size() > maxLength) {
        throw ApiError::invalidParams(QStringLiteral("'%1' is too long").arg(key));
    }
    return s;
}

inline QString requireNonEmpty(const QJsonObject& obj, const QString& key, qsizetype maxLength = 4096)
{
    auto s = requireString(obj, key, maxLength).trimmed();
    if (s.isEmpty()) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must not be empty").arg(key));
    }
    return s;
}

inline std::optional<QString> optionalString(const QJsonObject& obj, const QString& key, qsizetype maxLength = 4096)
{
    if (isMissing(field(obj, key))) {
        return std::nullopt;
    }
    return requireString(obj, key, maxLength);
}

inline bool requireBool(const QJsonObject& obj, const QString& key)
{
    const auto v = field(obj, key);
    if (!v.isBool()) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must be a boolean").arg(key));
    }
    return v.toBool();
}

inline bool optionalBool(const QJsonObject& obj, const QString& key, bool fallback)
{
    return isMissing(field(obj, key)) ? fallback : requireBool(obj, key);
}

inline qint64 requireInt(const QJsonObject& obj,
                         const QString& key,
                         qint64 min = std::numeric_limits<int>::min(),
                         qint64 max = std::numeric_limits<int>::max())
{
    const auto v = field(obj, key);
    if (!v.isDouble()) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must be a number").arg(key));
    }
    const double d = v.toDouble();
    if (d != static_cast<double>(static_cast<qint64>(d)) || d < static_cast<double>(min) || d > static_cast<double>(max)) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must be an integer in [%2, %3]").arg(key).arg(min).arg(max));
    }
    return static_cast<qint64>(d);
}

inline qint64 optionalInt(const QJsonObject& obj,
                          const QString& key,
                          qint64 fallback,
                          qint64 min = std::numeric_limits<int>::min(),
                          qint64 max = std::numeric_limits<int>::max())
{
    return isMissing(field(obj, key)) ? fallback : requireInt(obj, key, min, max);
}

inline QJsonObject requireObject(const QJsonObject& obj, const QString& key)
{
    const auto v = field(obj, key);
    if (!v.isObject()) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must be an object").arg(key));
    }
    return v.toObject();
}

inline QStringList requireStringList(const QJsonObject& obj, const QString& key, qsizetype maxItems = 10000)
{
    const auto v = field(obj, key);
    if (!v.isArray()) {
        throw ApiError::invalidParams(QStringLiteral("'%1' must be an array").arg(key));
    }
    const auto arr = v.toArray();
    if (arr.size() > maxItems) {
        throw ApiError::invalidParams(QStringLiteral("'%1' has too many items").arg(key));
    }
    QStringList out;
    out.reserve(arr.size());
    for (const auto& item : arr) {
        if (!item.isString()) {
            throw ApiError::invalidParams(QStringLiteral("'%1' must contain strings only").arg(key));
        }
        out << item.toString();
    }
    return out;
}

/** A plain file name: no directory separators, no `..`, not empty. Used for every name that maps to a file. */
inline QString requireFileName(const QJsonObject& obj, const QString& key)
{
    auto name = requireNonEmpty(obj, key, 255);
    if (name.contains('/') || name.contains('\\') || name == QLatin1String(".") || name == QLatin1String("..") || name.contains(QChar(0))) {
        throw ApiError::invalidParams(QStringLiteral("'%1' is not a valid file name").arg(key));
    }
    return name;
}

template <typename Enum, size_t N>
Enum requireEnum(const QJsonObject& obj, const QString& key, const std::pair<const char*, Enum> (&values)[N])
{
    const auto s = requireString(obj, key, 64);
    for (const auto& [name, value] : values) {
        if (s == QLatin1String(name)) {
            return value;
        }
    }
    throw ApiError::invalidParams(QStringLiteral("'%1' has an unsupported value '%2'").arg(key, s));
}

}  // namespace api::params
