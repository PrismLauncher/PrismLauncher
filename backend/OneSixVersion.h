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
	/// is this lib actually active on the current OS?
	bool m_is_active;
	/// is the library a native?
	bool m_is_native;
	/// native suffixes per OS
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
	
	/// Set the library composite name
	void setName(QString name);
	/// Set the url base for downloads
	void setBaseUrl(QString base_url);
	/// Call this to mark the library as 'native' (it's a zip archive with DLLs)
	void setIsNative();
	/// Attach a name suffix to the specified OS native
	void addNative(OpSys os, QString suffix);
	/// Set the load rules
	void setRules(QList<QSharedPointer<Rule> > rules);

	/// Returns true if the library should be loaded (or extracted, in case of natives)
	bool isActive();
	/// Returns true if the library is native
	bool isNative();
	/// Get the URL to download the library from
	QString downloadPath();
	/// Get the relative path where the library should be saved
	QString storagePath();
};


class FullVersion
{
public:
	/// the ID - determines which jar to use! ACTUALLY IMPORTANT!
	QString id;
	/// Last updated time - as a string
	QString time;
	/// Release time - as a string
	QString releaseTime;
	/// Release type - "release" or "snapshot"
	QString type;
	/**
	 * DEPRECATED: Old versions of the new vanilla launcher used this
	 * ex: "username_session_version"
	 */
	QString processArguments; 
	/**
	 * arguments that should be used for launching minecraft
	 * 
	 * ex: "--username ${auth_player_name} --session ${auth_session}
	 *      --version ${version_name} --gameDir ${game_directory} --assetsDir ${game_assets}"
	 */
	QString minecraftArguments;
	/**
	 * the minimum launcher version required by this version ... current is 4 (at point of writing)
	 */
	int minimumLauncherVersion;
	/**
	 * The main class to load first
	 */
	QString mainClass;
	
	/// the list of libs - both active and inactive, native and java
	QList<QSharedPointer<Library> > libraries;
	
	/**
	 * is this actually a legacy version? if so, none of the other stuff here will be ever used.
	 * added by FullVersionFactory
	 */
	bool isLegacy;

	/*
	FIXME: add support for those rules here? Looks like a pile of quick hacks to me though.

	"rules": [
		{
		"action": "allow"
		},
		{
		"action": "disallow",
		"os": {
			"name": "osx",
			"version": "^10\\.5\\.\\d$"
		}
		}
	],
	"incompatibilityReason": "There is a bug in LWJGL which makes it incompatible with OSX 10.5.8. Please go to New Profile and use 1.5.2 for now. Sorry!"
	}
	*/
	// QList<Rule> rules;
	
public:
	FullVersion()
	{
		minimumLauncherVersion = 0xDEADBEEF;
		isLegacy = false;
	}
	
	QList<QSharedPointer<Library> > getActiveNormalLibs();
	QList<QSharedPointer<Library> > getActiveNativeLibs();
};