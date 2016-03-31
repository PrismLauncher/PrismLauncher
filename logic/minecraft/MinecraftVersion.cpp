#include "MinecraftVersion.h"
#include "MinecraftProfile.h"
#include "VersionBuildError.h"
#include "ProfileUtils.h"
#include "settings/SettingsObject.h"
#include "minecraft/VersionFilterData.h"

bool MinecraftVersion::usesLegacyLauncher()
{
	return getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate;
}


QString MinecraftVersion::descriptor()
{
	return m_version;
}

QString MinecraftVersion::name()
{
	return m_version;
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

VersionSource MinecraftVersion::getVersionSource()
{
	return m_versionSource;
}

bool MinecraftVersion::hasJarMods()
{
	return false;
}

bool MinecraftVersion::isMinecraftVersion()
{
	return true;
}

void MinecraftVersion::applyFileTo(MinecraftProfile *profile)
{
	if(m_versionSource == Local && getVersionFile())
	{
		getVersionFile()->applyTo(profile);
	}
	else
	{
		throw VersionIncomplete(QObject::tr("Can't apply incomplete/builtin Minecraft version %1").arg(m_version));
	}
}

QString MinecraftVersion::getUrl() const
{
	// legacy fallback
	if(m_versionFileURL.isEmpty())
	{
		return QString("http://") + URLConstants::AWS_DOWNLOAD_VERSIONS + m_version + "/" + m_version + ".json";
	}
	// current
	return m_versionFileURL;
}

VersionFilePtr MinecraftVersion::getVersionFile()
{
	QFileInfo versionFile(QString("versions/%1/%1.dat").arg(m_version));
	m_problems.clear();
	if(!versionFile.exists())
	{
		if(m_loadedVersionFile)
		{
			m_loadedVersionFile.reset();
		}
		addProblem(PROBLEM_WARNING, QObject::tr("The patch file doesn't exist locally. It's possible it just needs to be downloaded."));
	}
	else
	{
		try
		{
			if(versionFile.lastModified() != m_loadedVersionFileTimestamp)
			{
				auto loadedVersionFile = ProfileUtils::parseBinaryJsonFile(versionFile);
				loadedVersionFile->name = "Minecraft";
				loadedVersionFile->setCustomizable(true);
				m_loadedVersionFileTimestamp = versionFile.lastModified();
				m_loadedVersionFile = loadedVersionFile;
			}
		}
		catch(Exception e)
		{
			m_loadedVersionFile.reset();
			addProblem(PROBLEM_ERROR, QObject::tr("The patch file couldn't be read:\n%1").arg(e.cause()));
		}
	}
	return m_loadedVersionFile;
}

bool MinecraftVersion::isCustomizable()
{
	switch(m_versionSource)
	{
		case Local:
		case Remote:
			// locally cached file, or a remote file that we can acquire can be customized
			return true;
		default:
			// Everything else is undefined and therefore not customizable.
			return false;
	}
	return false;
}

const QList<PatchProblem> &MinecraftVersion::getProblems()
{
	if(getVersionFile())
	{
		return getVersionFile()->getProblems();
	}
	return ProfilePatch::getProblems();
}

ProblemSeverity MinecraftVersion::getProblemSeverity()
{
	if(getVersionFile())
	{
		return getVersionFile()->getProblemSeverity();
	}
	return ProfilePatch::getProblemSeverity();
}

void MinecraftVersion::applyTo(MinecraftProfile *profile)
{
	// do we have this one cached?
	if (m_versionSource == Local)
	{
		applyFileTo(profile);
		return;
	}
	throw VersionIncomplete(QObject::tr("Minecraft version %1 could not be applied: version files are missing.").arg(m_version));
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

QString MinecraftVersion::getName()
{
	return "Minecraft";
}
QString MinecraftVersion::getVersion()
{
	return m_version;
}
QString MinecraftVersion::getID()
{
	return "net.minecraft";
}
QString MinecraftVersion::getFilename()
{
	return QString();
}
QDateTime MinecraftVersion::getReleaseDateTime()
{
	return m_releaseTime;
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
	return m_versionSource != Local && m_versionSource != Remote;
}
