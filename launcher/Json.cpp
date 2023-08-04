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

#include <math.h>
#include "FileSystem.h"

namespace Json {
void write(const QJsonDocument& doc, const QString& filename)
{
    FS::write(filename, doc.toJson());
}
void write(const QJsonObject& object, const QString& filename)
{
    write(QJsonDocument(object), filename);
}
void write(const QJsonArray& array, const QString& filename)
{
    write(QJsonDocument(array), filename);
}

QByteArray toText(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
QByteArray toText(const QJsonArray& array)
{
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

static bool isBinaryJson(const QByteArray& data)
{
    decltype(QJsonDocument::BinaryFormatTag) tag = QJsonDocument::BinaryFormatTag;
    return memcmp(data.constData(), &tag, sizeof(QJsonDocument::BinaryFormatTag)) == 0;
}
QJsonDocument requireDocument(const QByteArray& data, const QString& what)
{
    if (isBinaryJson(data)) {
        // FIXME: Is this needed?
        throw JsonException(what + ": Invalid JSON. Binary JSON unsupported");
    } else {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            throw JsonException(what + ": Error parsing JSON: " + error.errorString());
        }
        return doc;
    }
}
QJsonDocument requireDocument(const QString& filename, const QString& what)
{
    return requireDocument(FS::read(filename), what);
}
QJsonObject requireObject(const QJsonDocument& doc, const QString& what)
{
    if (!doc.isObject()) {
        throw JsonException(what + " is not an object");
    }
    return doc.object();
}
QJsonArray requireArray(const QJsonDocument& doc, const QString& what)
{
    if (!doc.isArray()) {
        throw JsonException(what + " is not an array");
    }
    return doc.array();
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
        for (auto value : values) {
            array.append(value);
        }
        to.insert(key, array);
    }
}

template <>
QJsonValue toJson<QUrl>(const QUrl& url)
{
    return QJsonValue(url.toString(QUrl::FullyEncoded));
}
template <>
QJsonValue toJson<QByteArray>(const QByteArray& data)
{
    return QJsonValue(QString::fromLatin1(data.toHex()));
}
template <>
QJsonValue toJson<QDateTime>(const QDateTime& datetime)
{
    return QJsonValue(datetime.toString(Qt::ISODate));
}
template <>
QJsonValue toJson<QDir>(const QDir& dir)
{
    return QDir::current().relativeFilePath(dir.absolutePath());
}
template <>
QJsonValue toJson<QUuid>(const QUuid& uuid)
{
    return uuid.toString();
}
template <>
QJsonValue toJson<QVariant>(const QVariant& variant)
{
    return QJsonValue::fromVariant(variant);
}

template <>
QByteArray requireIsType<QByteArray>(const QJsonValue& value, const QString& what)
{
    const QString string = ensureIsType<QString>(value, what);
    // ensure that the string can be safely cast to Latin1
    if (string != QString::fromLatin1(string.toLatin1())) {
        throw JsonException(what + " is not encodable as Latin1");
    }
    return QByteArray::fromHex(string.toLatin1());
}

template <>
QJsonArray requireIsType<QJsonArray>(const QJsonValue& value, const QString& what)
{
    if (!value.isArray()) {
        throw JsonException(what + " is not an array");
    }
    return value.toArray();
}

template <>
QString requireIsType<QString>(const QJsonValue& value, const QString& what)
{
    if (!value.isString()) {
        throw JsonException(what + " is not a string");
    }
    return value.toString();
}

template <>
bool requireIsType<bool>(const QJsonValue& value, const QString& what)
{
    if (!value.isBool()) {
        throw JsonException(what + " is not a bool");
    }
    return value.toBool();
}

template <>
double requireIsType<double>(const QJsonValue& value, const QString& what)
{
    if (!value.isDouble()) {
        throw JsonException(what + " is not a double");
    }
    return value.toDouble();
}

template <>
int requireIsType<int>(const QJsonValue& value, const QString& what)
{
    const double doubl = requireIsType<double>(value, what);
    if (fmod(doubl, 1) != 0) {
        throw JsonException(what + " is not an integer");
    }
    return int(doubl);
}

template <>
QDateTime requireIsType<QDateTime>(const QJsonValue& value, const QString& what)
{
    const QString string = requireIsType<QString>(value, what);
    const QDateTime datetime = QDateTime::fromString(string, Qt::ISODate);
    if (!datetime.isValid()) {
        throw JsonException(what + " is not a ISO formatted date/time value");
    }
    return datetime;
}

template <>
QUrl requireIsType<QUrl>(const QJsonValue& value, const QString& what)
{
    const QString string = ensureIsType<QString>(value, what);
    if (string.isEmpty()) {
        return QUrl();
    }
    const QUrl url = QUrl(string, QUrl::StrictMode);
    if (!url.isValid()) {
        throw JsonException(what + " is not a correctly formatted URL");
    }
    return url;
}

template <>
QDir requireIsType<QDir>(const QJsonValue& value, const QString& what)
{
    const QString string = requireIsType<QString>(value, what);
    // FIXME: does not handle invalid characters!
    return QDir::current().absoluteFilePath(string);
}

template <>
QUuid requireIsType<QUuid>(const QJsonValue& value, const QString& what)
{
    const QString string = requireIsType<QString>(value, what);
    const QUuid uuid = QUuid(string);
    if (uuid.toString() != string)  // converts back => valid
    {
        throw JsonException(what + " is not a valid UUID");
    }
    return uuid;
}

template <>
QJsonObject requireIsType<QJsonObject>(const QJsonValue& value, const QString& what)
{
    if (!value.isObject()) {
        throw JsonException(what + " is not an object");
    }
    return value.toObject();
}

template <>
QVariant requireIsType<QVariant>(const QJsonValue& value, const QString& what)
{
    if (value.isNull() || value.isUndefined()) {
        throw JsonException(what + " is null or undefined");
    }
    return value.toVariant();
}

template <>
QJsonValue requireIsType<QJsonValue>(const QJsonValue& value, const QString& what)
{
    if (value.isNull() || value.isUndefined()) {
        throw JsonException(what + " is null or undefined");
    }
    return value;
}

}  // namespace Json
