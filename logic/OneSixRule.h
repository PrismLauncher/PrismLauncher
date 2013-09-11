#pragma once
#include <QString>
#include <QSharedPointer>

class OneSixLibrary;
#include "OneSixLibrary.h"

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
	virtual bool applies(OneSixLibrary * parent) = 0;
public:
	Rule(RuleAction result)
		:m_result(result) {}
	virtual ~Rule(){};
	RuleAction apply(OneSixLibrary * parent)
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
	virtual bool applies ( OneSixLibrary* )
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
	virtual bool applies ( OneSixLibrary* )
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
