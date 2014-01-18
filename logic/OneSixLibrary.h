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

#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <memory>

#include "logic/net/URLConstants.h"
#include "OpSys.h"

class Rule;

class OneSixLibrary
{
private:
	// basic values used internally (so far)
	QString m_name;
	QString m_base_url = "https://" + URLConstants::LIBRARY_BASE;
	QList<std::shared_ptr<Rule>> m_rules;

	// custom values
	/// absolute URL. takes precedence over m_download_path, if defined
	QString m_absolute_url;
	/// download hint - how to actually get the library
	QString m_hint;

	// derived values used for real things
	/// a decent name fit for display
	QString m_decentname;
	/// a decent version fit for display
	QString m_decentversion;
	/// a decent type fit for display
	QString m_decenttype;
	/// where to store the lib locally
	QString m_storage_path;
	/// where to download the lib from
	QString m_download_url;
	/// is this lib actually active on the current OS?
	bool m_is_active = false;
	/// is the library a native?
	bool m_is_native = false;
	/// native suffixes per OS
	QMap<OpSys, QString> m_native_suffixes;

public:
	QStringList extract_excludes;

public:
	/// Constructor
	OneSixLibrary(QString name)
	{
		m_name = name;
	}

	/// Returns the raw name field
	QString rawName() const
	{
		return m_name;
	}

	QJsonObject toJson();

	/**
	 * finalize the library, processing the input values into derived values and state
	 *
	 * This SHALL be called after all the values are parsed or after any further change.
	 */
	void finalize();

	/// Set the library composite name
	void setName(QString name);
	/// get a decent-looking name
	QString name()
	{
		return m_decentname;
	}
	/// get a decent-looking version
	QString version()
	{
		return m_decentversion;
	}
	/// what kind of library is it? (for display)
	QString type()
	{
		return m_decenttype;
	}
	/// Set the url base for downloads
	void setBaseUrl(QString base_url);

	/// Call this to mark the library as 'native' (it's a zip archive with DLLs)
	void setIsNative();
	/// Attach a name suffix to the specified OS native
	void addNative(OpSys os, QString suffix);
	/// Set the load rules
	void setRules(QList<std::shared_ptr<Rule>> rules);

	/// Returns true if the library should be loaded (or extracted, in case of natives)
	bool isActive();
	/// Returns true if the library is native
	bool isNative();
	/// Get the URL to download the library from
	QString downloadUrl();
	/// Get the relative path where the library should be saved
	QString storagePath();

	/// set an absolute URL for the library. This is an MMC extension.
	void setAbsoluteUrl(QString absolute_url);
	QString absoluteUrl();

	/// set a hint about how to treat the library. This is an MMC extension.
	void setHint(QString hint);
	QString hint();

	bool extractTo(QString target_dir);
};
