// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QUrl>
#include <QDir>
#include <QUuid>
#include <QVariant>
#include <memory>

#include "Exception.h"

namespace Json
{
class MULTIMC_LOGIC_EXPORT JsonException : public ::Exception
{
public:
	JsonException(const QString &message) : Exception(message) {}
};

/// @throw FileSystemException
MULTIMC_LOGIC_EXPORT void write(const QJsonDocument &doc, const QString &filename);
/// @throw FileSystemException
MULTIMC_LOGIC_EXPORT void write(const QJsonObject &object, const QString &filename);
/// @throw FileSystemException
MULTIMC_LOGIC_EXPORT void write(const QJsonArray &array, const QString &filename);

MULTIMC_LOGIC_EXPORT QByteArray toBinary(const QJsonObject &obj);
MULTIMC_LOGIC_EXPORT QByteArray toBinary(const QJsonArray &array);
MULTIMC_LOGIC_EXPORT QByteArray toText(const QJsonObject &obj);
MULTIMC_LOGIC_EXPORT QByteArray toText(const QJsonArray &array);

/// @throw JsonException
MULTIMC_LOGIC_EXPORT QJsonDocument requireDocument(const QByteArray &data, const QString &what = "Document");
/// @throw JsonException
MULTIMC_LOGIC_EXPORT QJsonDocument requireDocument(const QString &filename, const QString &what = "Document");
/// @throw JsonException
MULTIMC_LOGIC_EXPORT QJsonObject requireObject(const QJsonDocument &doc, const QString &what = "Document");
/// @throw JsonException
MULTIMC_LOGIC_EXPORT QJsonArray requireArray(const QJsonDocument &doc, const QString &what = "Document");

/////////////////// WRITING ////////////////////

void writeString(QJsonObject & to, const QString &key, const QString &value);
void writeStringList(QJsonObject & to, const QString &key, const QStringList &values);

template<typename T>
QJsonValue toJson(const T &t)
{
	return QJsonValue(t);
}
template<>
QJsonValue toJson<QUrl>(const QUrl &url);
template<>
QJsonValue toJson<QByteArray>(const QByteArray &data);
template<>
QJsonValue toJson<QDateTime>(const QDateTime &datetime);
template<>
QJsonValue toJson<QDir>(const QDir &dir);
template<>
QJsonValue toJson<QUuid>(const QUuid &uuid);
template<>
QJsonValue toJson<QVariant>(const QVariant &variant);

template<typename T>
QJsonArray toJsonArray(const QList<T> &container)
{
	QJsonArray array;
	for (const T item : container)
	{
		array.append(toJson<T>(item));
	}
	return array;
}

////////////////// READING ////////////////////

/// @throw JsonException
template <typename T>
T requireIsType(const QJsonValue &value, const QString &what = "Value");

/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT double requireIsType<double>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT bool requireIsType<bool>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT int requireIsType<int>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QJsonObject requireIsType<QJsonObject>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QJsonArray requireIsType<QJsonArray>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QJsonValue requireIsType<QJsonValue>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QByteArray requireIsType<QByteArray>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QDateTime requireIsType<QDateTime>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QVariant requireIsType<QVariant>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QString requireIsType<QString>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QUuid requireIsType<QUuid>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QDir requireIsType<QDir>(const QJsonValue &value, const QString &what);
/// @throw JsonException
template<> MULTIMC_LOGIC_EXPORT QUrl requireIsType<QUrl>(const QJsonValue &value, const QString &what);

// the following functions are higher level functions, that make use of the above functions for
// type conversion
template <typename T>
T ensureIsType(const QJsonValue &value, const T default_ = T(), const QString &what = "Value")
{
	if (value.isUndefined() || value.isNull())
	{
		return default_;
	}
	try
	{
		return requireIsType<T>(value, what);
	}
	catch (const JsonException &)
	{
		return default_;
	}
}

/// @throw JsonException
template <typename T>
T requireIsType(const QJsonObject &parent, const QString &key, const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		throw JsonException(localWhat + "s parent does not contain " + localWhat);
	}
	return requireIsType<T>(parent.value(key), localWhat);
}

template <typename T>
T ensureIsType(const QJsonObject &parent, const QString &key, const T default_ = T(), const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		return default_;
	}
	return ensureIsType<T>(parent.value(key), default_, localWhat);
}

template <typename T>
QVector<T> requireIsArrayOf(const QJsonDocument &doc)
{
	const QJsonArray array = requireArray(doc);
	QVector<T> out;
	for (const QJsonValue val : array)
	{
		out.append(requireIsType<T>(val, "Document"));
	}
	return out;
}

template <typename T>
QVector<T> ensureIsArrayOf(const QJsonValue &value, const QString &what = "Value")
{
	const QJsonArray array = ensureIsType<QJsonArray>(value, QJsonArray(), what);
	QVector<T> out;
	for (const QJsonValue val : array)
	{
		out.append(requireIsType<T>(val, what));
	}
	return out;
}

template <typename T>
QVector<T> ensureIsArrayOf(const QJsonValue &value, const QVector<T> default_, const QString &what = "Value")
{
	if (value.isUndefined())
	{
		return default_;
	}
	return ensureIsArrayOf<T>(value, what);
}

/// @throw JsonException
template <typename T>
QVector<T> requireIsArrayOf(const QJsonObject &parent, const QString &key, const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		throw JsonException(localWhat + "s parent does not contain " + localWhat);
	}
	return ensureIsArrayOf<T>(parent.value(key), localWhat);
}

template <typename T>
QVector<T> ensureIsArrayOf(const QJsonObject &parent, const QString &key,
						 const QVector<T> &default_ = QVector<T>(), const QString &what = "__placeholder__")
{
	const QString localWhat = QString(what).replace("__placeholder__", '\'' + key + '\'');
	if (!parent.contains(key))
	{
		return default_;
	}
	return ensureIsArrayOf<T>(parent.value(key), default_, localWhat);
}

// this macro part could be replaced by variadic functions that just pass on their arguments, but that wouldn't work well with IDE helpers
#define JSON_HELPERFUNCTIONS(NAME, TYPE) \
	inline TYPE require##NAME(const QJsonValue &value, const QString &what = "Value") \
	{ \
		return requireIsType<TYPE>(value, what); \
	} \
	inline TYPE ensure##NAME(const QJsonValue &value, const TYPE default_ = TYPE(), const QString &what = "Value") \
	{ \
		return ensureIsType<TYPE>(value, default_, what); \
	} \
	inline TYPE require##NAME(const QJsonObject &parent, const QString &key, const QString &what = "__placeholder__") \
	{ \
		return requireIsType<TYPE>(parent, key, what); \
	} \
	inline TYPE ensure##NAME(const QJsonObject &parent, const QString &key, const TYPE default_ = TYPE(), const QString &what = "__placeholder") \
	{ \
		return ensureIsType<TYPE>(parent, key, default_, what); \
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

}
using JSONValidationError = Json::JsonException;
