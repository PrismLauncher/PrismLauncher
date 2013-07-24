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
public:
	OsRule(RuleAction result, OpSys system, QString version_regexp)
		: Rule(result), m_system(system), m_version_regexp(version_regexp) {}
};

class ImplicitRule : public Rule
{
protected:
	virtual bool applies ( Library* )
	{
		return true;
	}
public:
	ImplicitRule(RuleAction result)
		: Rule(result) {}
};

class Library
{
public:
	QString base_url;
	QString name;
	QList<QSharedPointer<Rule> > rules;
	QMap<OpSys, QString> natives;
	QStringList extract_excludes;
	
	void AddRule(RuleAction result)
	{
		rules.append(QSharedPointer<Rule>(new ImplicitRule(result)));
	}
	void AddRule(RuleAction result, OpSys system, QString version_regexp)
	{
		rules.append(QSharedPointer<Rule>(new OsRule(result, system, version_regexp)));
	}
	bool applies()
	{
		if(rules.empty())
			return true;
		RuleAction result = Disallow;
		for(auto rule: rules)
		{
			RuleAction temp = rule->apply( this );
			if(temp != Defer)
				result = temp;
		}
		return result == Allow;
	}
};
