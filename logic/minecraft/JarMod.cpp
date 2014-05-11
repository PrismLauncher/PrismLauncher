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

	auto readString = [libObj, filename](const QString & key, QString & variable)
	{
		if (libObj.contains(key))
		{
			QJsonValue val = libObj.value(key);
			if (!val.isString())
			{
				QLOG_WARN() << key << "is not a string in" << filename << "(skipping)";
			}
			else
			{
				variable = val.toString();
			}
		}
	};

	readString("url", out->baseurl);
	readString("MMC-hint", out->hint);
	readString("MMC-absoluteUrl", out->absoluteUrl);
	if(!out->baseurl.isEmpty() && out->absoluteUrl.isEmpty())
	{
		out->absoluteUrl = out->baseurl + out->name;
	}
	return out;
}

QJsonObject Jarmod::toJson()
{
	QJsonObject out;
	writeString(out, "name", name);
	writeString(out, "url", baseurl);
	writeString(out, "MMC-absoluteUrl", absoluteUrl);
	writeString(out, "MMC-hint", hint);
	return out;
}

QString Jarmod::url()
{
	if(!absoluteUrl.isEmpty())
		return absoluteUrl;
	else return baseurl + name;
}
