#include "logic/MMCJson.h"
using namespace MMCJson;

#include "RawLibrary.h"

RawLibraryPtr RawLibrary::fromJson(const QJsonObject &libObj, const QString &filename)
{
	RawLibraryPtr out(new RawLibrary());
	if (!libObj.contains("name"))
	{
		throw JSONValidationError(filename +
								  "contains a library that doesn't have a 'name' field");
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

	readString("url", out->url);
	readString("MMC-hint", out->hint);
	readString("MMC-absulute_url", out->absoluteUrl);
	readString("MMC-absoluteUrl", out->absoluteUrl);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = ensureObject(libObj.value("extract"));
		for (auto excludeVal : ensureArray(extractObj.value("exclude")))
		{
			out->excludes.append(ensureString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
		out->applyNatives = true;
		QJsonObject nativesObj = ensureObject(libObj.value("natives"));
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				QLOG_WARN() << filename << "contains an invalid native (skipping)";
			}
			OpSys opSys = OpSys_fromString(it.key());
			if (opSys != Os_Other)
			{
				out->natives.append(qMakePair(opSys, it.value().toString()));
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out->applyRules = true;
		out->rules = rulesFromJsonV4(libObj);
	}
	return out;
}
