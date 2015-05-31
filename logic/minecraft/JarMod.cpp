#include "JarMod.h"
#include "MMCJson.h"
using namespace MMCJson;

JarmodPtr Jarmod::fromJson(const QJsonObject &libObj, const QString &filename, const QString &originalName)
{
	JarmodPtr out(new Jarmod());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a jarmod that doesn't have a 'name' field");
	}
	out->name = libObj.value("name").toString();
	out->originalName = libObj.value("originalName").toString();
	return out;
}

QJsonObject Jarmod::toJson()
{
	QJsonObject out;
	writeString(out, "name", name);
	if(!originalName.isEmpty())
	{
		writeString(out, "originalName", originalName);
	}
	return out;
}
