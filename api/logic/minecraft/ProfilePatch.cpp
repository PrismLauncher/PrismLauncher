#include <meta/VersionList.h>
#include <meta/Index.h>
#include <Env.h>
#include "ProfilePatch.h"

#include "meta/Version.h"
#include "VersionFile.h"

ProfilePatch::ProfilePatch(std::shared_ptr<Meta::Version> version)
	:m_metaVersion(version)
{
}

ProfilePatch::ProfilePatch(std::shared_ptr<VersionFile> file, const QString& filename)
	:m_file(file), m_filename(filename)
{
}

void ProfilePatch::applyTo(MinecraftProfile* profile)
{
	auto vfile = getVersionFile();
	if(vfile)
	{
		vfile->applyTo(profile);
	}
}

std::shared_ptr<class VersionFile> ProfilePatch::getVersionFile()
{
	if(m_metaVersion)
	{
		if(!m_metaVersion->isLoaded())
		{
			m_metaVersion->load();
		}
		return m_metaVersion->data();
	}
	return m_file;
}

std::shared_ptr<class Meta::VersionList> ProfilePatch::getVersionList()
{
	if(m_metaVersion)
	{
		return ENV.metadataIndex()->get(m_metaVersion->uid());
	}
	return nullptr;
}

int ProfilePatch::getOrder()
{
	if(m_orderOverride)
		return m_order;

	auto vfile = getVersionFile();
	if(vfile)
	{
		return vfile->order;
	}
	return 0;
}
void ProfilePatch::setOrder(int order)
{
	m_orderOverride = true;
	m_order = order;
}
QString ProfilePatch::getID()
{
	if(m_metaVersion)
		return m_metaVersion->uid();
	return getVersionFile()->uid;
}
QString ProfilePatch::getName()
{
	if(m_metaVersion)
		return m_metaVersion->name();
	return getVersionFile()->name;
}
QString ProfilePatch::getVersion()
{
	if(m_metaVersion)
		return m_metaVersion->version();
	return getVersionFile()->version;
}
QString ProfilePatch::getFilename()
{
	return m_filename;
}
QDateTime ProfilePatch::getReleaseDateTime()
{
	if(m_metaVersion)
	{
		return m_metaVersion->time();
	}
	return getVersionFile()->releaseTime;
}

bool ProfilePatch::isCustom()
{
	return !m_isVanilla;
};

bool ProfilePatch::isCustomizable()
{
	if(m_metaVersion)
	{
		if(getVersionFile())
		{
			return true;
		}
	}
	return false;
}
bool ProfilePatch::isRemovable()
{
	return m_isRemovable;
}
bool ProfilePatch::isRevertible()
{
	return m_isRevertible;
}
bool ProfilePatch::isMoveable()
{
	return m_isMovable;
}
bool ProfilePatch::isVersionChangeable()
{
	auto list = getVersionList();
	if(list)
	{
		if(!list->isLoaded())
		{
			list->load();
		}
		return list->count() != 0;
	}
	return false;
}

void ProfilePatch::setVanilla (bool state)
{
	m_isVanilla = state;
}
void ProfilePatch::setRemovable (bool state)
{
	m_isRemovable = state;
}
void ProfilePatch::setRevertible (bool state)
{
	m_isRevertible = state;
}
void ProfilePatch::setMovable (bool state)
{
	m_isMovable = state;
}

ProblemSeverity ProfilePatch::getProblemSeverity()
{
	auto file = getVersionFile();
	if(file)
	{
		return file->getProblemSeverity();
	}
	return ProblemSeverity::Error;
}

const QList<PatchProblem> ProfilePatch::getProblems()
{
	auto file = getVersionFile();
	if(file)
	{
		return file->getProblems();
	}
	return {PatchProblem(ProblemSeverity::Error, QObject::tr("Patch is not loaded yet."))};
}
