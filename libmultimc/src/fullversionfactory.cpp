#include "fullversionfactory.h"
#include "fullversion.h"

QSharedPointer<FullVersion> FullVersionFactory::parse4(QJsonObject root, QSharedPointer<FullVersion> product)
{
	product->id = root.value("id").toString();
	
	// if it's on our legacy list, it's legacy
	if(legacyWhitelist.contains(product->id))
		product->isLegacy = true;
	
	product->mainClass = root.value("mainClass").toString();
	auto procArgsValue = root.value("processArguments");
	if(procArgsValue.isString())
	{
		product->processArguments = procArgsValue.toString();
		QString toCompare = product->processArguments.toLower();
		if(toCompare == "legacy")
		{
			product->minecraftArguments = " ${auth_player_name} ${auth_session}";
			product->isLegacy = true;
		}
		else if(toCompare == "username_session")
		{
			product->minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if(toCompare == "username_session_version")
		{
			product->minecraftArguments = "--username ${auth_player_name} --session ${auth_session} --version ${profile_name}";
		}
	}
	
	auto minecraftArgsValue = root.value("minecraftArguments");
	if(minecraftArgsValue.isString())
	{
		product->minecraftArguments = minecraftArgsValue.toString();
	}
	
	product->releaseTime = root.value("releaseTime").toString();
	product->time = root.value("time").toString();
	
	// Iterate through the list.
	auto librariesValue = root.value("libraries");
	if(librariesValue.isArray())
	{
		QJsonArray libList = root.value("libraries").toArray();
		for (auto lib : libList)
		{
			if (!lib.isObject())
			{
				continue;
			}
			
			QJsonObject libObj = lib.toObject();
			
			QString crud = libObj.value("name").toString();
			product->libraries.append(crud);
			
			// TODO: improve!
			/*
			auto parts = crud.split(':');
			int zz = parts.size();
			*/
		}
	}
	return product;
}

QSharedPointer<FullVersion> FullVersionFactory::parse(QByteArray data)
{
	QSharedPointer<FullVersion> readVersion(new FullVersion());
	
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	
	if (jsonError.error != QJsonParseError::NoError)
	{
		error_string = QString( "Error reading version file :") + " " + jsonError.errorString();
		m_error = FullVersionFactory::ParseError;
		return QSharedPointer<FullVersion>();
	}
	
	if(!jsonDoc.isObject())
	{
		error_string = "Error reading version file.";
		m_error = FullVersionFactory::ParseError;
		return QSharedPointer<FullVersion>();
	}
	QJsonObject root = jsonDoc.object();
	
	readVersion->minimumLauncherVersion = root.value("minimumLauncherVersion").toDouble();
	switch(readVersion->minimumLauncherVersion)
	{
		case 1:
		case 2:
		case 3:
		case 4:
			return parse4(root, readVersion);
		// ADD MORE HERE :D
		default:
			error_string = "Version file was for an unrecognized launcher version. RIP";
			m_error = FullVersionFactory::UnsupportedVersion;
			return QSharedPointer<FullVersion>();
	}
}


FullVersionFactory::FullVersionFactory()
{
	m_error = FullVersionFactory::AllOK;
	legacyWhitelist.append("1.5.1");
	legacyWhitelist.append("1.5.2");
}
