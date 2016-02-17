#include "MinecraftVersion.h"
#include "MinecraftProfile.h"
#include "VersionBuildError.h"
#include "ProfileUtils.h"
#include "settings/SettingsObject.h"

bool MinecraftVersion::usesLegacyLauncher()
{
	return m_traits.contains("legacyLaunch") || m_traits.contains("aplhaLaunch");
}

QString MinecraftVersion::descriptor()
{
	return m_descriptor;
}

QString MinecraftVersion::name()
{
	return m_name;
}

QString MinecraftVersion::typeString() const
{
	if(m_type == "snapshot")
	{
		return QObject::tr("Snapshot");
	}
	else if (m_type == "release")
	{
		return QObject::tr("Regular release");
	}
	else if (m_type == "old_alpha")
	{
		return QObject::tr("Alpha");
	}
	else if (m_type == "old_beta")
	{
		return QObject::tr("Beta");
	}
	else
	{
		return QString();
	}
}

bool MinecraftVersion::hasJarMods()
{
	return false;
}

bool MinecraftVersion::isMinecraftVersion()
{
	return true;
}

void MinecraftVersion::applyFileTo(MinecraftProfile *version)
{
	if(m_versionSource == Local && getVersionFile())
	{
		getVersionFile()->applyTo(version);
	}
	else
	{
		throw VersionIncomplete(QObject::tr("Can't apply incomplete/builtin Minecraft version %1").arg(m_name));
	}
}

QJsonDocument MinecraftVersion::toJson(bool saveOrder)
{
	if(m_versionSource == Local && getVersionFile())
	{
		return getVersionFile()->toJson(saveOrder);
	}
	else
	{
		throw VersionIncomplete(QObject::tr("Can't write incomplete/builtin Minecraft version %1").arg(m_name));
	}
}

QString MinecraftVersion::getUrl() const
{
	if(m_versionFileURL.isEmpty())
	{
		return QString("http://") + URLConstants::AWS_DOWNLOAD_VERSIONS + m_descriptor + "/" + m_descriptor + ".json";
	}
	return m_versionFileURL;
}

VersionFilePtr MinecraftVersion::getVersionFile()
{
	QFileInfo versionFile(QString("versions/%1/%1.dat").arg(m_descriptor));

	auto loadedVersionFile = ProfileUtils::parseBinaryJsonFile(versionFile);
	loadedVersionFile->name = "Minecraft";
	//FIXME: possibly not the best place for this... but w/e
	loadedVersionFile->setCustomizable(true);
	return loadedVersionFile;
}

bool MinecraftVersion::isCustomizable()
{
	switch(m_versionSource)
	{
		case Local:
		case Remote:
			// locally cached file, or a remote file that we can acquire can be customized
			return true;
		case Builtin:
			// builtins do not follow the normal OneSix format. They are not customizable.
		default:
			// Everything else is undefined and therefore not customizable.
			return false;
	}
	return false;
}

void MinecraftVersion::applyTo(MinecraftProfile *version)
{
	// do we have this one cached?
	if (m_versionSource == Local)
	{
		applyFileTo(version);
		return;
	}
	// if not builtin, do not proceed any further.
	if (m_versionSource != Builtin)
	{
		throw VersionIncomplete(QObject::tr(
			"Minecraft version %1 could not be applied: version files are missing.").arg(m_descriptor));
	}
	if (!m_descriptor.isNull())
	{
		version->id = m_descriptor;
	}
	if (!m_mainClass.isNull())
	{
		version->mainClass = m_mainClass;
	}
	if (!m_appletClass.isNull())
	{
		version->appletClass = m_appletClass;
	}
	if (!m_processArguments.isNull())
	{
		version->vanillaProcessArguments = m_processArguments;
		version->processArguments = m_processArguments;
	}
	if (!m_type.isNull())
	{
		version->type = m_type;
	}
	if (!m_releaseTimeString.isNull())
	{
		version->m_releaseTimeString = m_releaseTimeString;
		version->m_releaseTime = m_releaseTime;
	}
	if (!m_updateTimeString.isNull())
	{
		version->m_updateTimeString = m_updateTimeString;
		version->m_updateTime = m_updateTime;
	}
	version->traits.unite(m_traits);
}

int MinecraftVersion::getOrder()
{
	return order;
}

void MinecraftVersion::setOrder(int order)
{
	this->order = order;
}

QList<JarmodPtr> MinecraftVersion::getJarMods()
{
	return QList<JarmodPtr>();
}

QString MinecraftVersion::getPatchName()
{
	return "Minecraft";
}
QString MinecraftVersion::getPatchVersion()
{
	return m_descriptor;
}
QString MinecraftVersion::getPatchID()
{
	return "net.minecraft";
}
QString MinecraftVersion::getPatchFilename()
{
	return QString();
}

bool MinecraftVersion::needsUpdate()
{
	return m_versionSource == Remote || hasUpdate();
}

bool MinecraftVersion::hasUpdate()
{
	return m_versionSource == Remote || (m_versionSource == Local && upstreamUpdate);
}

bool MinecraftVersion::isCustom()
{
	// if we add any other source types, this will evaluate to false for them.
	return m_versionSource != Builtin && m_versionSource != Local && m_versionSource != Remote;
}
