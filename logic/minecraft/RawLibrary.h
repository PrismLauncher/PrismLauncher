#pragma once
#include <QString>
#include <QPair>
#include <memory>

#include "OneSixRule.h"

class RawLibrary;
typedef std::shared_ptr<RawLibrary> RawLibraryPtr;

class RawLibrary
{
public: /* methods */
	static RawLibraryPtr fromJson(const QJsonObject &libObj, const QString &filename);
	
public: /* data */
	QString name;
	QString url;
	QString hint;
	QString absoluteUrl;

	bool applyExcludes = false;
	QStringList excludes;

	bool applyNatives = false;
	QList<QPair<OpSys, QString>> natives;

	bool applyRules = false;
	QList<std::shared_ptr<Rule>> rules;

	// user for '+' libraries
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
