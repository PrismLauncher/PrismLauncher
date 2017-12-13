/* Copyright 2015-2017 MultiMC Contributors
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

#include "VersionList.h"

#include <QDateTime>

#include "Version.h"
#include "JsonFormat.h"

namespace Meta
{
VersionList::VersionList(const QString &uid, QObject *parent)
	: BaseVersionList(parent), m_uid(uid)
{
	setObjectName("Version list: " + uid);
}

shared_qobject_ptr<Task> VersionList::getLoadTask()
{
	load(Net::Mode::Online);
	return getCurrentTask();
}

bool VersionList::isLoaded()
{
	return BaseEntity::isLoaded();
}

const BaseVersionPtr VersionList::at(int i) const
{
	return m_versions.at(i);
}
int VersionList::count() const
{
	return m_versions.size();
}

void VersionList::sortVersions()
{
	beginResetModel();
	std::sort(m_versions.begin(), m_versions.end(), [](const VersionPtr &a, const VersionPtr &b)
	{
		return *a.get() < *b.get();
	});
	endResetModel();
}

QVariant VersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_versions.size() || index.parent().isValid())
	{
		return QVariant();
	}

	VersionPtr version = m_versions.at(index.row());

	switch (role)
	{
	case VersionPointerRole: return QVariant::fromValue(std::dynamic_pointer_cast<BaseVersion>(version));
	case VersionRole:
	case VersionIdRole:
		return version->version();
	case ParentVersionRole:
	{
		auto parentUid = this->parentUid();
		if(parentUid.isEmpty())
		{
			return QVariant();
		}
		auto & reqs = version->requires();
		auto iter = std::find_if(reqs.begin(), reqs.end(), [&parentUid](const Require & req)
		{
			return req.uid == parentUid;
		});
		if (iter != reqs.end())
		{
			return (*iter).equalsVersion;
		}
	}
	case TypeRole: return version->type();

	case UidRole: return version->uid();
	case TimeRole: return version->time();
	case RequiresRole: return QVariant::fromValue(version->requires());
	case SortRole: return version->rawTime();
	case VersionPtrRole: return QVariant::fromValue(version);
	case RecommendedRole: return version->isRecommended();
	// FIXME: this should be determined in whatever view/proxy is used...
	// case LatestRole: return version == getLatestStable();
	default: return QVariant();
	}
}

BaseVersionList::RoleList VersionList::providesRoles() const
{
	return {VersionPointerRole, VersionRole, VersionIdRole, ParentVersionRole,
				TypeRole, UidRole, TimeRole, RequiresRole, SortRole,
				RecommendedRole, LatestRole, VersionPtrRole};
}

QHash<int, QByteArray> VersionList::roleNames() const
{
	QHash<int, QByteArray> roles = BaseVersionList::roleNames();
	roles.insert(UidRole, "uid");
	roles.insert(TimeRole, "time");
	roles.insert(SortRole, "sort");
	roles.insert(RequiresRole, "requires");
	return roles;
}

QString VersionList::localFilename() const
{
	return m_uid + "/index.json";
}

QString VersionList::humanReadable() const
{
	return m_name.isEmpty() ? m_uid : m_name;
}

VersionPtr VersionList::getVersion(const QString &version)
{
	VersionPtr out = m_lookup.value(version, nullptr);
	if(!out)
	{
		out = std::make_shared<Version>(m_uid, version);
		m_lookup[version] = out;
	}
	return out;
}

void VersionList::setName(const QString &name)
{
	m_name = name;
	emit nameChanged(name);
}

void VersionList::setVersions(const QVector<VersionPtr> &versions)
{
	beginResetModel();
	m_versions = versions;
	std::sort(m_versions.begin(), m_versions.end(), [](const VersionPtr &a, const VersionPtr &b)
	{
		return a->rawTime() > b->rawTime();
	});
	for (int i = 0; i < m_versions.size(); ++i)
	{
		m_lookup.insert(m_versions.at(i)->version(), m_versions.at(i));
		setupAddedVersion(i, m_versions.at(i));
	}

	// FIXME: this is dumb, we have 'recommended' as part of the metadata already...
	auto recommendedIt = std::find_if(m_versions.constBegin(), m_versions.constEnd(), [](const VersionPtr &ptr) { return ptr->type() == "release"; });
	m_recommended = recommendedIt == m_versions.constEnd() ? nullptr : *recommendedIt;
	endResetModel();
}

void VersionList::parse(const QJsonObject& obj)
{
	parseVersionList(obj, this);
}

// FIXME: this is dumb, we have 'recommended' as part of the metadata already...
static const Meta::VersionPtr &getBetterVersion(const Meta::VersionPtr &a, const Meta::VersionPtr &b)
{
	if(!a)
		return b;
	if(!b)
		return a;
	if(a->type() == b->type())
	{
		// newer of same type wins
		return (a->rawTime() > b->rawTime() ? a : b);
	}
	// 'release' type wins
	return (a->type() == "release" ? a : b);
}

void VersionList::merge(const BaseEntity::Ptr &other)
{
	const VersionListPtr list = std::dynamic_pointer_cast<VersionList>(other);
	if (m_name != list->m_name)
	{
		setName(list->m_name);
	}

	if(m_parentUid != list->m_parentUid)
	{
		setParentUid(list->m_parentUid);
	}

	// TODO: do not reset the whole model. maybe?
	beginResetModel();
	m_versions.clear();
	if(list->m_versions.isEmpty())
	{
		qWarning() << "Empty list loaded ...";
	}
	for (const VersionPtr &version : list->m_versions)
	{
		// we already have the version. merge the contents
		if (m_lookup.contains(version->version()))
		{
			m_lookup.value(version->version())->merge(version);
		}
		else
		{
			m_lookup.insert(version->uid(), version);
		}
		// connect it.
		setupAddedVersion(m_versions.size(), version);
		m_versions.append(version);
		m_recommended = getBetterVersion(m_recommended, version);
	}
	endResetModel();
}

void VersionList::setupAddedVersion(const int row, const VersionPtr &version)
{
	// FIXME: do not disconnect from everythin, disconnect only the lambdas here
	version->disconnect();
	connect(version.get(), &Version::requiresChanged, this, [this, row]() { emit dataChanged(index(row), index(row), QVector<int>() << RequiresRole); });
	connect(version.get(), &Version::timeChanged, this, [this, row]() { emit dataChanged(index(row), index(row), QVector<int>() << TimeRole << SortRole); });
	connect(version.get(), &Version::typeChanged, this, [this, row]() { emit dataChanged(index(row), index(row), QVector<int>() << TypeRole); });
}

BaseVersionPtr VersionList::getRecommended() const
{
	return m_recommended;
}

}

void Meta::VersionList::setParentUid(const QString& parentUid)
{
	m_parentUid = parentUid;
}
