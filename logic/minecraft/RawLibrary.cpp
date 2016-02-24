#include "Json.h"
using namespace Json;

#include "RawLibrary.h"
#include <FileSystem.h>

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
			qWarning() << key << "is not a string in" << filename << "(skipping)";
			return false;
		}

		variable = val.toString();
		return true;
	};

	QString urlStr;
	readString("url", urlStr);
	out->m_base_url = urlStr;
	readString("MMC-hint", out->m_hint);
	readString("MMC-absulute_url", out->m_absolute_url);
	readString("MMC-absoluteUrl", out->m_absolute_url);
	if (libObj.contains("extract"))
	{
		out->applyExcludes = true;
		auto extractObj = requireObject(libObj.value("extract"));
		for (auto excludeVal : requireArray(extractObj.value("exclude")))
		{
			out->extract_excludes.append(requireString(excludeVal));
		}
	}
	if (libObj.contains("natives"))
	{
		QJsonObject nativesObj = requireObject(libObj.value("natives"));
		for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it)
		{
			if (!it.value().isString())
			{
				qWarning() << filename << "contains an invalid native (skipping)";
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

QJsonObject RawLibrary::toJson() const
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

QStringList RawLibrary::files() const
{
	QStringList retval;
	QString storage = storageSuffix();
	if (storage.contains("${arch}"))
	{
		QString cooked_storage = storage;
		cooked_storage.replace("${arch}", "32");
		retval.append(cooked_storage);
		cooked_storage = storage;
		cooked_storage.replace("${arch}", "64");
		retval.append(cooked_storage);
	}
	else
		retval.append(storage);
	return retval;
}

bool RawLibrary::filesExist(const QDir &base) const
{
	auto libFiles = files();
	for(auto file: libFiles)
	{
		QFileInfo info(base, file);
		qWarning() << info.absoluteFilePath() << "doesn't exist";
		if (!info.exists())
			return false;
	}
	return true;
}

QString RawLibrary::url() const
{
	if (!m_absolute_url.isEmpty())
	{
		return m_absolute_url;
	}

	if (m_base_url.isEmpty())
	{
		return QString("https://" + URLConstants::LIBRARY_BASE) + storageSuffix();
	}

	if(m_base_url.endsWith('/'))
	{
		return m_base_url + storageSuffix();
	}
	else
	{
		return m_base_url + QChar('/') + storageSuffix();
	}
}

bool RawLibrary::isActive() const
{
	bool result = true;
	if (m_rules.empty())
	{
		result = true;
	}
	else
	{
		RuleAction ruleResult = Disallow;
		for (auto rule : m_rules)
		{
			RuleAction temp = rule->apply(this);
			if (temp != Defer)
				ruleResult = temp;
		}
		result = result && (ruleResult == Allow);
	}
	if (isNative())
	{
		result = result && m_native_classifiers.contains(currentSystem);
	}
	return result;
}

void RawLibrary::setStoragePrefix(QString prefix)
{
	m_storagePrefix = prefix;
}

QString RawLibrary::defaultStoragePrefix()
{
	return "libraries/";
}

QString RawLibrary::storagePrefix() const
{
	if(m_storagePrefix.isEmpty())
	{
		return defaultStoragePrefix();
	}
	return m_storagePrefix;
}

QString RawLibrary::storageSuffix() const
{
	// non-native? use only the gradle specifier
	if (!isNative())
	{
		return m_name.toPath();
	}

	// otherwise native, override classifiers. Mojang HACK!
	GradleSpecifier nativeSpec = m_name;
	if (m_native_classifiers.contains(currentSystem))
	{
		nativeSpec.setClassifier(m_native_classifiers[currentSystem]);
	}
	else
	{
		nativeSpec.setClassifier("INVALID");
	}
	return nativeSpec.toPath();
}

QString RawLibrary::storagePath() const
{
	return FS::PathCombine(storagePrefix(), storageSuffix());
}

bool RawLibrary::storagePathIsDefault() const
{
	return m_storagePrefix.isEmpty();
}
