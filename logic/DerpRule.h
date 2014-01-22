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

#include "logic/DerpLibrary.h"

enum RuleAction
{
	Allow,
	Disallow,
	Defer
};

RuleAction RuleAction_fromString(QString);
QList<std::shared_ptr<Rule>> rulesFromJsonV4(const QJsonObject &objectWithRules);

class Rule
{
protected:
	RuleAction m_result;
	virtual bool applies(DerpLibrary *parent) = 0;

public:
	Rule(RuleAction result) : m_result(result)
	{
	}
	virtual ~Rule() {};
	virtual QJsonObject toJson() = 0;
	RuleAction apply(DerpLibrary *parent)
	{
		if (applies(parent))
			return m_result;
		else
			return Defer;
	}
	;
};

class OsRule : public Rule
{
private:
	// the OS
	OpSys m_system;
	// the OS version regexp
	QString m_version_regexp;

protected:
	virtual bool applies(DerpLibrary *)
	{
		return (m_system == currentSystem);
	}
	OsRule(RuleAction result, OpSys system, QString version_regexp)
		: Rule(result), m_system(system), m_version_regexp(version_regexp)
	{
	}

public:
	virtual QJsonObject toJson();
	static std::shared_ptr<OsRule> create(RuleAction result, OpSys system,
										  QString version_regexp)
	{
		return std::shared_ptr<OsRule>(new OsRule(result, system, version_regexp));
	}
};

class ImplicitRule : public Rule
{
protected:
	virtual bool applies(DerpLibrary *)
	{
		return true;
	}
	ImplicitRule(RuleAction result) : Rule(result)
	{
	}

public:
	virtual QJsonObject toJson();
	static std::shared_ptr<ImplicitRule> create(RuleAction result)
	{
		return std::shared_ptr<ImplicitRule>(new ImplicitRule(result));
	}
};
