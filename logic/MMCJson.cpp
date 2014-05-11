#include "MMCJson.h"
#include <QString>
#include <QStringList>
#include <math.h>

bool MMCJson::ensureBoolean(const QJsonValue val, const QString what)
{
	if (!val.isBool())
		throw JSONValidationError(what + " is not boolean");
	return val.isBool();
}

QJsonValue MMCJson::ensureExists(QJsonValue val, const QString what)
{
	if(val.isUndefined() || val.isUndefined())
		throw JSONValidationError(what + " does not exist");
	return val;
}

QJsonArray MMCJson::ensureArray(const QJsonValue val, const QString what)
{
	if (!val.isArray())
		throw JSONValidationError(what + " is not an array");
	return val.toArray();
}

double MMCJson::ensureDouble(const QJsonValue val, const QString what)
{
	if (!val.isDouble())
		throw JSONValidationError(what + " is not a number");
	return val.toDouble();
}

int MMCJson::ensureInteger(const QJsonValue val, const QString what)
{
	double ret = ensureDouble(val, what);
	if (fmod(ret, 1) != 0)
		throw JSONValidationError(what + " is not an integer");
	return ret;
}

QJsonObject MMCJson::ensureObject(const QJsonValue val, const QString what)
{
	if (!val.isObject())
		throw JSONValidationError(what + " is not an object");
	return val.toObject();
}

QJsonObject MMCJson::ensureObject(const QJsonDocument val, const QString what)
{
	if (!val.isObject())
		throw JSONValidationError(what + " is not an object");
	return val.object();
}

QString MMCJson::ensureString(const QJsonValue val, const QString what)
{
	if (!val.isString())
		throw JSONValidationError(what + " is not a string");
	return val.toString();
}

void MMCJson::writeString(QJsonObject &to, QString key, QString value)
{
	if(value.size())
	{
		to.insert(key, value);
	}
}

void MMCJson::writeStringList(QJsonObject &to, QString key, QStringList values)
{
	if(values.size())
	{
		QJsonArray array;
		for(auto value: values)
		{
			array.append(value);
		}
		to.insert(key, array);
	}
}

