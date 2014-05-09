#include "MinecraftVersion.h"
#include "VersionFinal.h"

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
	if (is_latest && is_snapshot)
	{
		return QObject::tr("Latest snapshot");
	}
	else if (is_latest)
	{
		return QObject::tr("Latest release");
	}
	else if (is_snapshot)
	{
		return QObject::tr("Snapshot");
	}
	else
	{
		return QObject::tr("Regular release");
	}
}
bool MinecraftVersion::hasJarMods()
{
	return false;
}
bool MinecraftVersion::isVanilla()
{
	return true;
}

void MinecraftVersion::applyTo(VersionFinal *version)
{
	// FIXME: make this work.
	if(m_versionSource != Builtin)
	{
		return;
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
