#pragma once
#include <QString>
#include <QStringList>
#include <QMap>
#include <QSharedPointer>
#include "OpSys.h"

class Rule;

class OneSixLibrary
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
	OneSixLibrary(QString name)
	{
		m_is_native = false;
		m_is_active = false;
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
