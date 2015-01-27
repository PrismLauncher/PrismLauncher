#include "JarMod.h"
#include "logic/MMCJson.h"
using namespace MMCJson;

JarmodPtr Jarmod::fromJson(const QJsonObject &libObj, const QString &filename)
{
	JarmodPtr out(new Jarmod());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a jarmod that doesn't have a 'name' field");
	}
	out->name = libObj.value("name").toString();
	return out;
}

QJsonObject Jarmod::toJson()
{
	QJsonObject out;
	writeString(out, "name", name);
	return out;
}
