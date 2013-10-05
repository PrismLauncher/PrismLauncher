#include "OneSixRule.h"
#include <QJsonObject>
#include <QJsonArray>

QList<std::shared_ptr<Rule>> rulesFromJsonV4(QJsonObject &objectWithRules)
{
	QList<std::shared_ptr<Rule>> rules;
	auto rulesVal = objectWithRules.value("rules");
	if (!rulesVal.isArray())
		return rules;

	QJsonArray ruleList = rulesVal.toArray();
	for (auto ruleVal : ruleList)
	{
		std::shared_ptr<Rule> rule;
		if (!ruleVal.isObject())
			continue;
		auto ruleObj = ruleVal.toObject();
		auto actionVal = ruleObj.value("action");
		if (!actionVal.isString())
			continue;
		auto action = RuleAction_fromString(actionVal.toString());
		if (action == Defer)
			continue;

		auto osVal = ruleObj.value("os");
		if (!osVal.isObject())
		{
			// add a new implicit action rule
			rules.append(ImplicitRule::create(action));
			continue;
		}

		auto osObj = osVal.toObject();
		auto osNameVal = osObj.value("name");
		if (!osNameVal.isString())
			continue;
		OpSys requiredOs = OpSys_fromString(osNameVal.toString());
		QString versionRegex = osObj.value("version").toString();
		// add a new OS rule
		rules.append(OsRule::create(action, requiredOs, versionRegex));
	}
}

QJsonObject ImplicitRule::toJson()
{
	QJsonObject ruleObj;
	ruleObj.insert("action", m_result == Allow ? QString("allow") : QString("disallow"));
	return ruleObj;
}

QJsonObject OsRule::toJson()
{
	QJsonObject ruleObj;
	ruleObj.insert("action", m_result == Allow ? QString("allow") : QString("disallow"));
	QJsonObject osObj;
	{
		osObj.insert("name", OpSys_toString(m_system));
		osObj.insert("version", m_version_regexp);
	}
	ruleObj.insert("os", osObj);
	return ruleObj;
}

RuleAction RuleAction_fromString(QString name)
{
	if (name == "allow")
		return Allow;
	if (name == "disallow")
		return Disallow;
	return Defer;
}