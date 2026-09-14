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

#include "Json.h"

#include <QFile>
#include <expected>

#include <cmath>
#include "FileSystem.h"

namespace {
bool isBinaryJson(const QByteArray& data)
{
    decltype(QJsonDocument::BinaryFormatTag) tag = QJsonDocument::BinaryFormatTag;
    return memcmp(data.constData(), &tag, sizeof(QJsonDocument::BinaryFormatTag)) == 0;
}
}  // namespace
namespace Json {
Result<> write(const QJsonDocument& doc, const QString& filename)
{
    return FS::write(filename, doc.toJson());
}
Result<> write(const QJsonObject& object, const QString& filename)
{
    return write(QJsonDocument(object), filename);
}
Result<> write(const QJsonArray& array, const QString& filename)
{
    return write(QJsonDocument(array), filename);
}

QByteArray toText(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
QByteArray toText(const QJsonArray& array)
{
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

Result<QJsonDocument> requireDocument(const QByteArray& data, const QString& what)
{
    if (isBinaryJson(data)) {
        // FIXME: Is this needed?
        return std::unexpected(what + ": Invalid JSON. Binary JSON unsupported");
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return std::unexpected(what + ": Error parsing JSON at: " + QString::number(error.offset) + " reason: " + error.errorString());
    }
    return doc;
}
Result<QJsonDocument> requireDocument(const QString& filename, const QString& what)
{
    return FS::read(filename)
        .transform_error([what](const auto& v) { return what + ": Error reading file: " + v; })
        .and_then([what](const auto& v) { return requireDocument(v, what); });
}
Result<QJsonObject> requireObject(const QJsonDocument& doc, const QString& what)
{
    if (!doc.isObject()) {
        return std::unexpected(what + " is not an object");
    }
    return doc.object();
}
Result<QJsonObject> requireObject(const QByteArray& data, const QString& what)
{
    return requireDocument(data, what).and_then([what](const auto& v) { return requireObject(v, what); });
}
Result<QJsonObject> requireObject(const QString& filename, const QString& what)
{
    return requireDocument(filename, what).and_then([what](const auto& v) { return requireObject(v, what); });
}
Result<QJsonArray> requireArray(const QJsonDocument& doc, const QString& what)
{
    if (!doc.isArray()) {
        return std::unexpected(what + " is not an array");
    }
    return doc.array();
}
Result<QJsonArray> requireArray(const QByteArray& data, const QString& what)
{
    return requireDocument(data, what).and_then([what](const auto& v) { return requireArray(v, what); });
}

QJsonDocument parseUntilGarbage(const QByteArray& json, QJsonParseError* error, QString* garbage)
{
    auto doc = QJsonDocument::fromJson(json, error);
    if (error->error == QJsonParseError::GarbageAtEnd) {
        qsizetype offset = error->offset;
        QByteArray validJson = json.left(offset);
        doc = QJsonDocument::fromJson(validJson, error);

        if (garbage) {
            *garbage = json.right(json.size() - offset);
        }
    }

    return doc;
}

void writeString(QJsonObject& to, const QString& key, const QString& value)
{
    if (!value.isEmpty()) {
        to.insert(key, value);
    }
}

void writeStringList(QJsonObject& to, const QString& key, const QStringList& values)
{
    if (!values.isEmpty()) {
        QJsonArray array;
        for (const auto& value : values) {
            array.append(value);
        }
        to.insert(key, array);
    }
}

template <>
QJsonValue toJson<QUrl>(const QUrl& t)
{
    return { t.toString(QUrl::FullyEncoded) };
}
template <>
QJsonValue toJson<QByteArray>(const QByteArray& t)
{
    return { QString::fromLatin1(t.toHex()) };
}
template <>
QJsonValue toJson<QDateTime>(const QDateTime& t)
{
    return { t.toString(Qt::ISODate) };
}
template <>
QJsonValue toJson<QDir>(const QDir& t)
{
    return QDir::current().relativeFilePath(t.absolutePath());
}
template <>
QJsonValue toJson<QUuid>(const QUuid& t)
{
    return t.toString();
}
template <>
QJsonValue toJson<QVariant>(const QVariant& t)
{
    return QJsonValue::fromVariant(t);
}

template <>
Result<QByteArray> requireIsType<QByteArray>(const QJsonValue& value, const QString& what)
{
    const QString string = value.toString(what);
    // ensure that the string can be safely cast to Latin1
    if (string != QString::fromLatin1(string.toLatin1())) {
        return std::unexpected(what + " is not encodable as Latin1");
    }
    return QByteArray::fromHex(string.toLatin1());
}

template <>
Result<QJsonArray> requireIsType<QJsonArray>(const QJsonValue& value, const QString& what)
{
    if (!value.isArray()) {
        return std::unexpected(what + " is not an array");
    }
    return value.toArray();
}

template <>
Result<QString> requireIsType<QString>(const QJsonValue& value, const QString& what)
{
    if (!value.isString()) {
        return std::unexpected(what + " is not a string");
    }
    return value.toString();
}

template <>
Result<bool> requireIsType<bool>(const QJsonValue& value, const QString& what)
{
    if (!value.isBool()) {
        return std::unexpected(what + " is not a bool");
    }
    return value.toBool();
}

template <>
Result<double> requireIsType<double>(const QJsonValue& value, const QString& what)
{
    if (!value.isDouble()) {
        return std::unexpected(what + " is not a double");
    }
    return value.toDouble();
}

template <>
Result<int> requireIsType<int>(const QJsonValue& value, const QString& what)
{
    return requireIsType<double>(value, what).and_then([what](const auto& v) -> Result<int> {
        if (fmod(v, 1) != 0) {
            return std::unexpected(what + " is not an integer");
        }
        return int(v);
    });
}

template <>
Result<QDateTime> requireIsType<QDateTime>(const QJsonValue& value, const QString& what)
{
    return requireIsType<QString>(value, what).and_then([what](const auto& v) -> Result<QDateTime> {
        auto datetime = QDateTime::fromString(v, Qt::ISODate);
        if (!datetime.isValid()) {
            return std::unexpected(what + " is not a ISO formatted date/time value");
        }
        return datetime;
    });
}

template <>
Result<QUrl> requireIsType<QUrl>(const QJsonValue& value, const QString& what)
{
    return requireIsType<QString>(value, what).and_then([what](const auto& v) -> Result<QUrl> {
        if (v.isEmpty()) {
            return {};
        }
        auto url = QUrl(v, QUrl::StrictMode);
        if (!url.isValid()) {
            return std::unexpected(what + " is not a correctly formatted URL");
        }
        return url;
    });
}

template <>
Result<QDir> requireIsType<QDir>(const QJsonValue& value, const QString& what)
{
    return requireIsType<QString>(value, what).and_then([what](const auto& v) -> Result<QDir> {
        // FIXME: does not handle invalid characters!
        return QDir::current().absoluteFilePath(v);
    });
}

template <>
Result<QUuid> requireIsType<QUuid>(const QJsonValue& value, const QString& what)
{
    return requireIsType<QString>(value, what).and_then([what](const auto& v) -> Result<QUuid> {
        auto uuid = QUuid(v);
        if (uuid.toString() != v)  // converts back => valid
        {
            return std::unexpected(what + " is not a valid UUID");
        }
        return uuid;
    });
}

template <>
Result<QJsonObject> requireIsType<QJsonObject>(const QJsonValue& value, const QString& what)
{
    if (!value.isObject()) {
        return std::unexpected(what + " is not an object");
    }
    return value.toObject();
}

template <>
Result<QVariant> requireIsType<QVariant>(const QJsonValue& value, const QString& what)
{
    if (value.isNull() || value.isUndefined()) {
        return std::unexpected(what + " is null or undefined");
    }
    return value.toVariant();
}

template <>
Result<QJsonValue> requireIsType<QJsonValue>(const QJsonValue& value, const QString& what)
{
    if (value.isNull() || value.isUndefined()) {
        return std::unexpected(what + " is null or undefined");
    }
    return value;
}

QStringList toStringList(const QString& jsonString)
{
    return requireDocument(jsonString.toUtf8())
        .and_then([](const auto& v) { return requireIsArrayOf<QString>(v); })
        .value_or(QStringList());
}

QString fromStringList(const QStringList& list)
{
    QJsonArray array;
    for (const QString& str : list) {
        array.append(str);
    }

    QJsonDocument doc(toJsonArray(list));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

QVariantMap toMap(const QString& jsonString)
{
    auto doc = requireObject(jsonString.toUtf8()).value_or(QJsonObject());
    return doc.toVariantMap();
}

QString fromMap(const QVariantMap& map)
{
    QJsonObject obj = QJsonObject::fromVariantMap(map);
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

}  // namespace Json
