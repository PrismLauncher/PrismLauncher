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

#include <QtCore>

class Library;

enum OpSys
{
	Os_Windows,
	Os_Linux,
	Os_OSX,
	Os_Other
};

OpSys OpSys_fromString(QString);

#ifdef Q_OS_MAC
	#define currentSystem Os_OSX
#endif

#ifdef Q_OS_LINUX
	#define currentSystem Os_Linux
#endif

#ifdef Q_OS_WIN32
	#define currentSystem Os_Windows
#endif

#ifndef currentSystem
	#define currentSystem Os_Other
#endif


enum RuleAction
{
	Allow,
	Disallow,
	Defer
};

RuleAction RuleAction_fromString(QString);

class Rule
{
protected:
	RuleAction m_result;
	virtual bool applies(Library * parent) = 0;
public:
	Rule(RuleAction result)
		:m_result(result) {}
	virtual ~Rule(){};
	RuleAction apply(Library * parent)
	{
		if(applies(parent))
			return m_result;
		else
			return Defer;
	};
};

class OsRule : public Rule
{
private:
	// the OS
	OpSys m_system;
	// the OS version regexp
	QString m_version_regexp;
protected:
	virtual bool applies ( Library* )
	{
		return (m_system == currentSystem);
	}
	OsRule(RuleAction result, OpSys system, QString version_regexp)
		: Rule(result), m_system(system), m_version_regexp(version_regexp) {}
public:
	static QSharedPointer<OsRule> create(RuleAction result, OpSys system, QString version_regexp)
	{
		return QSharedPointer<OsRule> (new OsRule(result, system, version_regexp));
	}
};

class ImplicitRule : public Rule
{
protected:
	virtual bool applies ( Library* )
	{
		return true;
	}
	ImplicitRule(RuleAction result)
		: Rule(result) {}
public:
	static QSharedPointer<ImplicitRule> create(RuleAction result)
	{
		return QSharedPointer<ImplicitRule> (new ImplicitRule(result));
	}
};

class Library
{
private:
	// basic values used internally (so far)
	QString m_name;
	QString m_base_url;
	QList<QSharedPointer<Rule> > m_rules;
	
	// derived values used for real things
	/// where to store the lib locally
	QString m_storage_path;
	/// where to download the lib from
	QString m_download_path;
	/// is this lib actuall active on the current OS?
	bool m_is_active;
	
	// native lib?
	bool m_is_native;
	QMap<OpSys, QString> m_native_suffixes;
public:
	QStringList extract_excludes;
	
public:
	/// Constructor
	Library(QString name)
	{
		m_is_native = false;
		m_is_native = false;
		m_name = name;
		m_base_url = "https://s3.amazonaws.com/Minecraft.Download/libraries/";
	}
	
	/**
	 * finalize the library, processing the input values into derived values and state
	 * 
	 * This SHALL be called after all the values are parsed or after any further change.
	 */
	void finalize();
	
	
	/**
	 * Set the library composite name
	 */
	void setName(QString name)
	{
		m_name = name;
	}
	
	/**
	 * Set the url base for downloads
	 */
	void setBaseUrl(QString base_url)
	{
		m_base_url = base_url;
	}
	
	/**
	 * Call this to mark the library as 'native' (it's a zip archive with DLLs)
	 */
	void setIsNative()
	{
		m_is_native = true;
	}
	
	/**
	 * Attach a name suffix to the specified OS native
	 */
	void addNative(OpSys os, QString suffix)
	{
		m_is_native = true;
		m_native_suffixes[os] = suffix;
	}
	
	/**
	 * Set the load rules
	 */
	void setRules(QList<QSharedPointer<Rule> > rules)
	{
		m_rules = rules;
	}

	/**
	 * Returns true if the library should be loaded (or extracted, in case of natives)
	 */
	bool getIsActive()
	{
		return m_is_active;
	}
	/**
	 * Returns true if the library is native
	 */
	bool getIsNative()
	{
		return m_is_native;
	}
};
