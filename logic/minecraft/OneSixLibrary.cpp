/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QJsonArray>

#include "OneSixLibrary.h"
#include "OneSixRule.h"
#include "OpSys.h"
#include "logic/net/URLConstants.h"
#include <pathutils.h>
#include <JlCompress.h>
#include "logger/QsLog.h"

OneSixLibrary::OneSixLibrary(RawLibraryPtr base)
{
	m_name = base->m_name;
	m_base_url = base->m_base_url;
	m_hint = base->m_hint;
	m_absolute_url = base->m_absolute_url;
	extract_excludes = base->extract_excludes;
	m_native_classifiers = base->m_native_classifiers;
	m_rules = base->m_rules;
	finalize();
}

OneSixLibraryPtr OneSixLibrary::fromRawLibrary(RawLibraryPtr lib)
{
	return OneSixLibraryPtr(new OneSixLibrary(lib));
}

void OneSixLibrary::finalize()
{
	QString relative;
	
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
	if (isNative())
	{
		GradleSpecifier nativeSpec = m_name;
		m_is_active = m_is_active && m_native_classifiers.contains(currentSystem);
		m_decenttype = "Native";
		if(m_native_classifiers.contains(currentSystem))
		{
			nativeSpec.setClassifier(m_native_classifiers[currentSystem]);
		}
		else
		{
			nativeSpec.setClassifier("INVALID");
		}
		relative = nativeSpec.toPath();
	}
	else
	{
		relative = m_name.toPath();
		m_decenttype = "Java";
	}

	m_decentname = m_name.artifactId();
	m_decentversion = minVersion = m_name.version();
	m_storage_path = relative;
	if(m_base_url.isEmpty())
		m_download_url = QString("https://" + URLConstants::LIBRARY_BASE) + relative;
	else
		m_download_url = m_base_url + relative;
}

void OneSixLibrary::setName(const QString &name)
{
	m_name = name;
}
void OneSixLibrary::setBaseUrl(const QString &base_url)
{
	m_base_url = base_url;
}
void OneSixLibrary::addNative(OpSys os, const QString &suffix)
{
	m_native_classifiers[os] = suffix;
}
void OneSixLibrary::clearSuffixes()
{
	m_native_classifiers.clear();
}
void OneSixLibrary::setRules(QList<std::shared_ptr<Rule>> rules)
{
	m_rules = rules;
}
bool OneSixLibrary::isActive() const
{
	return m_is_active;
}
QString OneSixLibrary::downloadUrl() const
{
	if (m_absolute_url.size())
		return m_absolute_url;
	return m_download_url;
}
QString OneSixLibrary::storagePath() const
{
	return m_storage_path;
}

void OneSixLibrary::setAbsoluteUrl(const QString &absolute_url)
{
	m_absolute_url = absolute_url;
}

QString OneSixLibrary::absoluteUrl() const
{
	return m_absolute_url;
}

void OneSixLibrary::setHint(const QString &hint)
{
	m_hint = hint;
}

QString OneSixLibrary::hint() const
{
	return m_hint;
}

QStringList OneSixLibrary::files()
{
	QStringList retval;
	QString storage = storagePath();
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

bool OneSixLibrary::filesExist(const QDir &base)
{
	auto libFiles = files();
	for(auto file: libFiles)
	{
		QFileInfo info(base, file);
		QLOG_WARN() << info.absoluteFilePath() << "doesn't exist";
		if (!info.exists())
			return false;
	}
	return true;
}

bool OneSixLibrary::extractTo(QString target_dir)
{
	QString storage = storagePath();
	if (storage.contains("${arch}"))
	{
		QString cooked_storage = storage;
		cooked_storage.replace("${arch}", "32");
		QString origin = PathCombine("libraries", cooked_storage);
		QString target_dir_cooked = PathCombine(target_dir, "32");
		if (!ensureFolderPathExists(target_dir_cooked))
		{
			QLOG_ERROR() << "Couldn't create folder " + target_dir_cooked;
			return false;
		}
		if (JlCompress::extractWithExceptions(origin, target_dir_cooked, extract_excludes)
				.isEmpty())
		{
			QLOG_ERROR() << "Couldn't extract " + origin;
			return false;
		}
		cooked_storage = storage;
		cooked_storage.replace("${arch}", "64");
		origin = PathCombine("libraries", cooked_storage);
		target_dir_cooked = PathCombine(target_dir, "64");
		if (!ensureFolderPathExists(target_dir_cooked))
		{
			QLOG_ERROR() << "Couldn't create folder " + target_dir_cooked;
			return false;
		}
		if (JlCompress::extractWithExceptions(origin, target_dir_cooked, extract_excludes)
				.isEmpty())
		{
			QLOG_ERROR() << "Couldn't extract " + origin;
			return false;
		}
	}
	else
	{
		if (!ensureFolderPathExists(target_dir))
		{
			QLOG_ERROR() << "Couldn't create folder " + target_dir;
			return false;
		}
		QString path = PathCombine("libraries", storage);
		if (JlCompress::extractWithExceptions(path, target_dir, extract_excludes).isEmpty())
		{
			QLOG_ERROR() << "Couldn't extract " + path;
			return false;
		}
	}
	return true;
}
