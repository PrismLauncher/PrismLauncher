#pragma once
#include <QString>
#include <QPair>
#include <QList>
#include <QStringList>
#include <QMap>
#include <memory>

#include "logic/minecraft/OneSixRule.h"
#include "logic/minecraft/OpSys.h"
#include "logic/net/URLConstants.h"

class RawLibrary;
typedef std::shared_ptr<RawLibrary> RawLibraryPtr;

class RawLibrary
{
public: /* methods */
	/// read and create a basic library
	static RawLibraryPtr fromJson(const QJsonObject &libObj, const QString &filename);
	/// read and create a MultiMC '+' library. Those have some extra fields.
	static RawLibraryPtr fromJsonPlus(const QJsonObject &libObj, const QString &filename);
	QJsonObject toJson();
	
	QString fullname();
	QString version();
	QString group();
	
public: /* data */
	QString m_name;
	QString m_base_url;

	/// type hint - modifies how the library is treated
	QString m_hint;
	/// DEPRECATED: absolute URL. takes precedence over m_download_path, if defined
	QString m_absolute_url;

	bool applyExcludes = false;
	QStringList extract_excludes;

	/// Returns true if the library is native
	bool isNative() const;
	/// native suffixes per OS
	QMap<OpSys, QString> m_native_suffixes;

	bool applyRules = false;
	QList<std::shared_ptr<Rule>> m_rules;

	// used for '+' libraries
	enum InsertType
	{
		Apply,
		Append,
		Prepend,
		Replace
	} insertType = Append;
	QString insertData;
	
	// soft or hard dependency? hard means 'needs equal', soft means 'needs equal or newer'
	enum DependType
	{
		Soft,
		Hard
	} dependType = Soft;
};
