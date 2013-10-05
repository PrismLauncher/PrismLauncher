#include "OneSixLibrary.h"
#include "OneSixRule.h"
#include "OpSys.h"
#include <QJsonArray>
void OneSixLibrary::finalize()
{
	QStringList parts = m_name.split(':');
	QString relative = parts[0];
	relative.replace('.', '/');
	relative += '/' + parts[1] + '/' + parts[2] + '/' + parts[1] + '-' + parts[2];

	if (!m_is_native)
		relative += ".jar";
	else
	{
		if (m_native_suffixes.contains(currentSystem))
		{
			relative += "-" + m_native_suffixes[currentSystem] + ".jar";
		}
		else
		{
			// really, bad.
			relative += ".jar";
		}
	}

	m_decentname = parts[1];
	m_decentversion = parts[2];
	m_storage_path = relative;
	m_download_url = m_base_url + relative;

	if (m_rules.empty())
	{
		m_is_active = true;
	}
	else
	{
		RuleAction result = Disallow;
		for (auto rule : m_rules)
		{
			RuleAction temp = rule->apply(this);
			if (temp != Defer)
				result = temp;
		}
		m_is_active = (result == Allow);
	}
	if (m_is_native)
	{
		m_is_active = m_is_active && m_native_suffixes.contains(currentSystem);
		m_decenttype = "Native";
	}
	else
	{
		m_decenttype = "Java";
	}
}

void OneSixLibrary::setName(QString name)
{
	m_name = name;
}
void OneSixLibrary::setBaseUrl(QString base_url)
{
	m_base_url = base_url;
}
void OneSixLibrary::setIsNative()
{
	m_is_native = true;
}
void OneSixLibrary::addNative(OpSys os, QString suffix)
{
	m_is_native = true;
	m_native_suffixes[os] = suffix;
}
void OneSixLibrary::setRules(QList<std::shared_ptr<Rule>> rules)
{
	m_rules = rules;
}
bool OneSixLibrary::isActive()
{
	return m_is_active;
}
bool OneSixLibrary::isNative()
{
	return m_is_native;
}
QString OneSixLibrary::downloadUrl()
{
	if(m_absolute_url.size())
		return m_absolute_url;
	return m_download_url;
}
QString OneSixLibrary::storagePath()
{
	return m_storage_path;
}

void OneSixLibrary::setAbsoluteUrl(QString absolute_url)
{
	m_absolute_url = absolute_url;
}

QString OneSixLibrary::absoluteUrl()
{
	return m_absolute_url;
}

void OneSixLibrary::setHint(QString hint)
{
	m_hint = hint;
}

QString OneSixLibrary::hint()
{
	return m_hint;
}

QJsonObject OneSixLibrary::toJson()
{
	QJsonObject libRoot;
	libRoot.insert("name", m_name);
	if(m_absolute_url.size())
		libRoot.insert("MMC-absoluteUrl", m_absolute_url);
	if(m_hint.size())
		libRoot.insert("MMC-hint", m_hint);
	if(m_base_url != "https://s3.amazonaws.com/Minecraft.Download/libraries/")
		libRoot.insert("url", m_base_url);
	if (isNative() && m_native_suffixes.size())
	{
		QJsonObject nativeList;
		auto iter = m_native_suffixes.begin();
		while (iter != m_native_suffixes.end())
		{
			nativeList.insert(OpSys_toString(iter.key()), iter.value());
			iter++;
		}
		libRoot.insert("natives", nativeList);
	}
	if (isNative() && extract_excludes.size())
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
