#include "VersionFactory.h"
#include "OneSixVersion.h"
#include "OneSixRule.h"

// Library rules (if any)
QList<QSharedPointer<Rule> > FullVersionFactory::parse4rules(QJsonObject & baseObj)
{
	QList<QSharedPointer<Rule> > rules;
	auto rulesVal = baseObj.value("rules");
	if(rulesVal.isArray())
	{
		QJsonArray ruleList = rulesVal.toArray();
		for(auto ruleVal : ruleList)
		{
			QSharedPointer<Rule> rule;
			if(!ruleVal.isObject())
				continue;
			auto ruleObj = ruleVal.toObject();
			auto actionVal = ruleObj.value("action");
			if(!actionVal.isString())
				continue;
			auto action = RuleAction_fromString(actionVal.toString());
			if(action == Defer)
				continue;
			
			auto osVal = ruleObj.value("os");
			if(!osVal.isObject())
			{
				// add a new implicit action rule
				rules.append(ImplicitRule::create(action));
			}
			else
			{
				auto osObj = osVal.toObject();
				auto osNameVal = osObj.value("name");
				if(!osNameVal.isString())
					continue;
				OpSys requiredOs = OpSys_fromString(osNameVal.toString());
				QString versionRegex = osObj.value("version").toString();
				// add a new OS rule
				rules.append(OsRule::create(action, requiredOs, versionRegex));
			}
		}
	}
	return rules;
}


QSharedPointer<OneSixVersion> FullVersionFactory::parse4(QJsonObject root, QSharedPointer<OneSixVersion> fullVersion)
{
	fullVersion->id = root.value("id").toString();
	
	fullVersion->mainClass = root.value("mainClass").toString();
	auto procArgsValue = root.value("processArguments");
	if(procArgsValue.isString())
	{
		fullVersion->processArguments = procArgsValue.toString();
		QString toCompare = fullVersion->processArguments.toLower();
		if(toCompare == "legacy")
		{
			fullVersion->minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if(toCompare == "username_session")
		{
			fullVersion->minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if(toCompare == "username_session_version")
		{
			fullVersion->minecraftArguments = "--username ${auth_player_name} --session ${auth_session} --version ${profile_name}";
		}
	}
	
	auto minecraftArgsValue = root.value("minecraftArguments");
	if(minecraftArgsValue.isString())
	{
		fullVersion->minecraftArguments = minecraftArgsValue.toString();
	}
	
	auto minecraftTypeValue = root.value("type");
	if(minecraftTypeValue.isString())
	{
		fullVersion->type = minecraftTypeValue.toString();
	}
	
	fullVersion->releaseTime = root.value("releaseTime").toString();
	fullVersion->time = root.value("time").toString();
	
	// Iterate through the list, if it's a list.
	auto librariesValue = root.value("libraries");
	if(!librariesValue.isArray())
		return fullVersion;
	
	QJsonArray libList = root.value("libraries").toArray();
	for (auto libVal : libList)
	{
		if (!libVal.isObject())
		{
			continue;
		}
		
		QJsonObject libObj = libVal.toObject();
		
		// Library name
		auto nameVal = libObj.value("name");
		if(!nameVal.isString())
			continue;
		QSharedPointer<OneSixLibrary> library(new OneSixLibrary(nameVal.toString()));
		
		auto urlVal = libObj.value("url");
		if(urlVal.isString())
		{
			library->setBaseUrl(urlVal.toString());
		}
		
		// Extract excludes (if any)
		auto extractVal = libObj.value("extract");
		if(extractVal.isObject())
		{
			QStringList excludes;
			auto extractObj = extractVal.toObject();
			auto excludesVal = extractObj.value("exclude");
			if(!excludesVal.isArray())
				goto SKIP_EXTRACTS;
			auto excludesList = excludesVal.toArray();
			for(auto excludeVal : excludesList)
			{
				if(excludeVal.isString())
					excludes.append(excludeVal.toString());
			}
			library->extract_excludes = excludes;
		}
		SKIP_EXTRACTS:
		
		auto nativesVal = libObj.value("natives");
		if(nativesVal.isObject())
		{
			library->setIsNative();
			auto nativesObj = nativesVal.toObject();
			auto iter = nativesObj.begin();
			while(iter != nativesObj.end())
			{
				auto osType = OpSys_fromString(iter.key());
				if(osType == Os_Other)
					continue;
				if(!iter.value().isString())
					continue;
				library->addNative(osType, iter.value().toString());
				iter++;
			}
		}
		library->setRules(parse4rules(libObj));
		library->finalize();
		fullVersion->libraries.append(library);
	}
	return fullVersion;
}

QSharedPointer<OneSixVersion> FullVersionFactory::parse(QByteArray data)
{
	QSharedPointer<OneSixVersion> readVersion(new OneSixVersion());
	
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	
	if (jsonError.error != QJsonParseError::NoError)
	{
		error_string = QString( "Error reading version file :") + " " + jsonError.errorString();
		m_error = FullVersionFactory::ParseError;
		return QSharedPointer<OneSixVersion>();
	}
	
	if(!jsonDoc.isObject())
	{
		error_string = "Error reading version file.";
		m_error = FullVersionFactory::ParseError;
		return QSharedPointer<OneSixVersion>();
	}
	QJsonObject root = jsonDoc.object();
	
	int launcher_ver = readVersion->minimumLauncherVersion = root.value("minimumLauncherVersion").toDouble();
	// ADD MORE HERE :D
	if(launcher_ver > 0 && launcher_ver <= 7)
		return parse4(root, readVersion);
	else
	{
		error_string = "Version file was for an unrecognized launcher version. RIP";
		m_error = FullVersionFactory::UnsupportedVersion;
		return QSharedPointer<OneSixVersion>();
	}
}


FullVersionFactory::FullVersionFactory()
{
	m_error = FullVersionFactory::AllOK;
}
