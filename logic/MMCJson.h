/**
 * Some de-bullshitting for Qt JSON failures.
 *
 * Simple exception-throwing
 */

#pragma once
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <memory>
#include "MMCError.h"

class JSONValidationError : public MMCError
{
public:
	JSONValidationError(QString cause) : MMCError(cause) {}
};
class FileOpenError : public MMCError
{
public:
	FileOpenError(const QFile &file) : MMCError(QObject::tr("Error opening %1: %2").arg(file.fileName(), file.errorString())) {}
};

namespace MMCJson
{
/// parses the data into a json document. throws if there's a parse error
QJsonDocument parseDocument(const QByteArray &data, const QString &what);

/// tries to open and then parses the specified file. throws if there's an error
QJsonDocument parseFile(const QString &filename, const QString &what);

/// make sure the value exists. throw otherwise.
QJsonValue ensureExists(QJsonValue val, const QString what = "value");

/// make sure the value is converted into an object. throw otherwise.
QJsonObject ensureObject(const QJsonValue val, const QString what = "value");

/// make sure the document is converted into an object. throw otherwise.
QJsonObject ensureObject(const QJsonDocument val, const QString what = "document");

/// make sure the value is converted into an array. throw otherwise.
QJsonArray ensureArray(const QJsonValue val, QString what = "value");

/// make sure the document is converted into an array. throw otherwise.
QJsonArray ensureArray(const QJsonDocument &val, const QString &what = "document");

/// make sure the value is converted into a string. throw otherwise.
QString ensureString(const QJsonValue val, QString what = "value");

/// make sure the value is converted into a string that's parseable as an url. throw otherwise.
QUrl ensureUrl(const QJsonValue &val, const QString &what = "value");

/// make sure the value is converted into a boolean. throw otherwise.
bool ensureBoolean(const QJsonValue val, QString what = "value");

/// make sure the value is converted into an integer. throw otherwise.
int ensureInteger(const QJsonValue val, QString what = "value");

/// make sure the value is converted into an integer. throw otherwise. this version will return the default value if the field is undefined.
int ensureInteger(const QJsonValue val, QString what, const int def);

/// make sure the value is converted into a double precision floating number. throw otherwise.
double ensureDouble(const QJsonValue val, QString what = "value");

QStringList ensureStringList(const QJsonValue val, QString what);

void writeString(QJsonObject & to, QString key, QString value);

void writeStringList(QJsonObject & to, QString key, QStringList values);

template <typename T>
void writeObjectList(QJsonObject & to, QString key, QList<std::shared_ptr<T>> values)
{
	if (!values.isEmpty())
	{
		QJsonArray array;
		for (auto value: values)
		{
			array.append(value->toJson());
		}
		to.insert(key, array);
	}
}
template <typename T>
void writeObjectList(QJsonObject & to, QString key, QList<T> values)
{
	if (!values.isEmpty())
	{
		QJsonArray array;
		for (auto value: values)
		{
			array.append(value.toJson());
		}
		to.insert(key, array);
	}
}
}

