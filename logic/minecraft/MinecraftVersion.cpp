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
	m_problems.clear();
	m_problemSeverity = PROBLEM_NONE;
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
		case Builtin:
			// builtins do not follow the normal OneSix format. They are not customizable.
		default:
			// Everything else is undefined and therefore not customizable.
			return false;
	}
	return false;
}

const QList<PatchProblem> &MinecraftVersion::getProblems()
{
	if(m_versionSource != Builtin && getVersionFile())
	{
		return getVersionFile()->getProblems();
	}
	return ProfilePatch::getProblems();
}

ProblemSeverity MinecraftVersion::getProblemSeverity()
{
	if(m_versionSource != Builtin && getVersionFile())
	{
		return getVersionFile()->getProblemSeverity();
	}
	return ProfilePatch::getProblemSeverity();
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
	version->applyMinecraftVersion(m_descriptor);
	version->applyMainClass(m_mainClass);
	version->applyAppletClass(m_appletClass);
	version->applyMinecraftArguments(" ${auth_player_name} ${auth_session}"); // all builtin versions are legacy
	version->applyMinecraftVersionType(m_type);
	version->applyTraits(m_traits);
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
	return m_descriptor;
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
	return m_versionSource != Builtin && m_versionSource != Local && m_versionSource != Remote;
}
