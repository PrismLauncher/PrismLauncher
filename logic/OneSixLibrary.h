#pragma once
#include <QString>
#include <QStringList>
#include <QMap>
#include <memory>
#include <QJsonObject>
#include "OpSys.h"

class Rule;

class OneSixLibrary
{
private:
	// basic values used internally (so far)
	QString m_name;
	QString m_base_url = "https://s3.amazonaws.com/Minecraft.Download/libraries/";
	QList<std::shared_ptr<Rule> > m_rules;

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
	void setRules(QList<std::shared_ptr<Rule> > rules);

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
};
