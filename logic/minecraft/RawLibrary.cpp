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
	out->m_name = libObj.value("name").toString();

	auto readString = [libObj, filename](const QString & key, QString & variable) -> bool
	{
		if (!libObj.contains(key))
			return false;
		QJsonValue val = libObj.value(key);

		if (!val.isString())
		{
			QLOG_WARN() << key << "is not a string in" << filename << "(skipping)";
			return false;
		}

		variable = val.toString();
		return true;
	};

	readString("url", out->m_base_url);
	readString("MMC-hint", out->m_hint);
	readString("MMC-absulute_url", out->m_absolute_url);
	readString("MMC-absoluteUrl", out->m_absolute_url);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = ensureObject(libObj.value("extract"));
		for (auto excludeVal : ensureArray(extractObj.value("exclude")))
		{
			out->extract_excludes.append(ensureString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
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
				out->m_native_classifiers[opSys] = it.value().toString();
			}
		}
	}
	if (libObj.contains("rules"))
	{
		out->applyRules = true;
		out->m_rules = rulesFromJsonV4(libObj);
	}
	return out;
}

RawLibraryPtr RawLibrary::fromJsonPlus(const QJsonObject &libObj, const QString &filename)
{
	auto lib = RawLibrary::fromJson(libObj, filename);
	if (libObj.contains("insert"))
	{
		QJsonValue insertVal = ensureExists(libObj.value("insert"), "library insert rule");
		QString insertString;
		{
			if (insertVal.isString())
			{
				insertString = insertVal.toString();
			}
			else if (insertVal.isObject())
			{
				QJsonObject insertObj = insertVal.toObject();
				if (insertObj.isEmpty())
				{
					throw JSONValidationError("One library has an empty insert object in " +
											  filename);
				}
				insertString = insertObj.keys().first();
				lib->insertData = insertObj.value(insertString).toString();
			}
		}
		if (insertString == "apply")
		{
			lib->insertType = RawLibrary::Apply;
		}
		else if (insertString == "prepend")
		{
			lib->insertType = RawLibrary::Prepend;
		}
		else if (insertString == "append")
		{
			lib->insertType = RawLibrary::Append;
		}
		else if (insertString == "replace")
		{
			lib->insertType = RawLibrary::Replace;
		}
		else
		{
			throw JSONValidationError("A '+' library in " + filename +
									  " contains an invalid insert type");
		}
	}
	if (libObj.contains("MMC-depend"))
	{
		const QString dependString = ensureString(libObj.value("MMC-depend"));
		if (dependString == "hard")
		{
			lib->dependType = RawLibrary::Hard;
		}
		else if (dependString == "soft")
		{
			lib->dependType = RawLibrary::Soft;
		}
		else
		{
			throw JSONValidationError("A '+' library in " + filename +
									  " contains an invalid depend type");
		}
	}
	return lib;
}

bool RawLibrary::isNative() const
{
	return m_native_classifiers.size() != 0;
}

QJsonObject RawLibrary::toJson()
{
	QJsonObject libRoot;
	libRoot.insert("name", (QString)m_name);
	if (m_absolute_url.size())
		libRoot.insert("MMC-absoluteUrl", m_absolute_url);
	if (m_hint.size())
		libRoot.insert("MMC-hint", m_hint);
	if (m_base_url != "http://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		m_base_url != "https://" + URLConstants::AWS_DOWNLOAD_LIBRARIES &&
		m_base_url != "https://" + URLConstants::LIBRARY_BASE && !m_base_url.isEmpty())
	{
		libRoot.insert("url", m_base_url);
	}
	if (isNative())
	{
		QJsonObject nativeList;
		auto iter = m_native_classifiers.begin();
		while (iter != m_native_classifiers.end())
		{
			nativeList.insert(OpSys_toString(iter.key()), iter.value());
			iter++;
		}
		libRoot.insert("natives", nativeList);
		if (extract_excludes.size())
		{
			QJsonArray excludes;
			QJsonObject extract;
			for (auto exclude : extract_excludes)
			{
				excludes.append(exclude);
			}
			extract.insert("exclude", excludes);
			libRoot.insert("extract", extract);
		}
	}
	if (m_rules.size())
	{
		QJsonArray allRules;
		for (auto &rule : m_rules)
		{
			QJsonObject ruleObj = rule->toJson();
			allRules.append(ruleObj);
		}
		libRoot.insert("rules", allRules);
	}
	return libRoot;
}

QString RawLibrary::fullname()
{
	return m_name.artifactPrefix();
}
