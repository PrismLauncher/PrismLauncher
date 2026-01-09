// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <memory>

#include "Exception.h"

namespace Json {
class JsonException : public ::Exception {
   public:
    JsonException(const QString& message) : Exception(message) {}
};

/// @throw FileSystemException
void write(const QJsonDocument& doc, const QString& filename);
/// @throw FileSystemException
void write(const QJsonObject& object, const QString& filename);
/// @throw FileSystemException
void write(const QJsonArray& array, const QString& filename);

QByteArray toText(const QJsonObject& obj);
QByteArray toText(const QJsonArray& array);

/// @throw JsonException
QJsonDocument requireDocument(const QByteArray& data, const QString& what = "Document");
/// @throw JsonException
QJsonDocument requireDocument(const QString& filename, const QString& what = "Document");
/// @throw JsonException
QJsonObject requireObject(const QJsonDocument& doc, const QString& what = "Document");
/// @throw JsonException
QJsonArray requireArray(const QJsonDocument& doc, const QString& what = "Document");

/////////////////// WRITING ////////////////////

void writeString(QJsonObject& to, const QString& key, const QString& value);
void writeStringList(QJsonObject& to, const QString& key, const QStringList& values);

template <typename T>
QJsonValue toJson(const T& t)
{
    return QJsonValue(t);
}
template <>
QJsonValue toJson<QUrl>(const QUrl& url);
template <>
QJsonValue toJson<QByteArray>(const QByteArray& data);
template <>
QJsonValue toJson<QDateTime>(const QDateTime& datetime);
template <>
QJsonValue toJson<QDir>(const QDir& dir);
template <>
QJsonValue toJson<QUuid>(const QUuid& uuid);
template <>
QJsonValue toJson<QVariant>(const QVariant& variant);

template <typename T>
QJsonArray toJsonArray(const QList<T>& container)
{
    QJsonArray array;
    for (const T& item : container) {
        array.append(toJson<T>(item));
    }
    return array;
}

////////////////// READING ////////////////////

// Attempt to parse JSON up until garbage is encountered
QJsonDocument parseUntilGarbage(const QByteArray& json, QJsonParseError* error = nullptr, QString* garbage = nullptr);

/// @throw JsonException
template <typename T>
T requireIsType(const QJsonValue& value, const QString& what = "Value");

/// @throw JsonException
template <>
double requireIsType<double>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
bool requireIsType<bool>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
int requireIsType<int>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QJsonObject requireIsType<QJsonObject>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QJsonArray requireIsType<QJsonArray>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QJsonValue requireIsType<QJsonValue>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QByteArray requireIsType<QByteArray>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QDateTime requireIsType<QDateTime>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QVariant requireIsType<QVariant>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QString requireIsType<QString>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QUuid requireIsType<QUuid>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QDir requireIsType<QDir>(const QJsonValue& value, const QString& what);
/// @throw JsonException
template <>
QUrl requireIsType<QUrl>(const QJsonValue& value, const QString& what);

// the following functions are higher level functions, that make use of the above functions for
// type conversion

/// @throw JsonException
template <typename T>
T requireIsType(const QJsonObject& parent, const QString& key, const QString& what = "__placeholder__")
{
    const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
    if (!parent.contains(key)) {
        throw JsonException(localWhat + "s parent does not contain " + localWhat);
    }
    return requireIsType<T>(parent.value(key), localWhat);
}

template <typename T>
QList<T> requireIsArrayOf(const QJsonDocument& doc)
{
    const QJsonArray array = requireArray(doc);
    QList<T> out;
    for (const QJsonValue val : array) {
        out.append(requireIsType<T>(val, "Document"));
    }
    return out;
}

/// @throw JsonException
template <typename T>
QList<T> requireIsArrayOf(const QJsonObject& parent, const QString& key, const QString& what = "__placeholder__")
{
    const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
    if (!parent.contains(key)) {
        throw JsonException(localWhat + "s parent does not contain " + localWhat);
    }

    const QJsonArray array = parent[key].toArray();
    QList<T> out;
    for (const QJsonValue val : array) {
        out.append(requireIsType<T>(val, "Document"));
    }
    return out;
}

// this macro part could be replaced by variadic functions that just pass on their arguments, but that wouldn't work well with IDE helpers
#define JSON_HELPERFUNCTIONS(NAME, TYPE)                                                                              \
    inline TYPE require##NAME(const QJsonValue& value, const QString& what = "Value")                                 \
    {                                                                                                                 \
        return requireIsType<TYPE>(value, what);                                                                      \
    }                                                                                                                 \
    inline TYPE require##NAME(const QJsonObject& parent, const QString& key, const QString& what = "__placeholder__") \
    {                                                                                                                 \
        return requireIsType<TYPE>(parent, key, what);                                                                \
    }

JSON_HELPERFUNCTIONS(Array, QJsonArray)
JSON_HELPERFUNCTIONS(Object, QJsonObject)
JSON_HELPERFUNCTIONS(JsonValue, QJsonValue)
JSON_HELPERFUNCTIONS(String, QString)
JSON_HELPERFUNCTIONS(Boolean, bool)
JSON_HELPERFUNCTIONS(Double, double)
JSON_HELPERFUNCTIONS(Integer, int)
JSON_HELPERFUNCTIONS(DateTime, QDateTime)
JSON_HELPERFUNCTIONS(Url, QUrl)
JSON_HELPERFUNCTIONS(ByteArray, QByteArray)
JSON_HELPERFUNCTIONS(Dir, QDir)
JSON_HELPERFUNCTIONS(Uuid, QUuid)
JSON_HELPERFUNCTIONS(Variant, QVariant)

#undef JSON_HELPERFUNCTIONS

// helper functions for settings
QStringList toStringList(const QString& jsonString);
QString fromStringList(const QStringList& list);

QVariantMap toMap(const QString& jsonString);
QString fromMap(const QVariantMap& map);

}  // namespace Json
using JSONValidationError = Json::JsonException;
