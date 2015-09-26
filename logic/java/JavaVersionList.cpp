/* Copyright 2013-2015 MultiMC Contributors
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

#include <QtNetwork>
#include <QtXml>
#include <QRegExp>

#include <QDebug>

#include "java/JavaVersionList.h"
#include "java/JavaCheckerJob.h"
#include "java/JavaUtils.h"
#include "MMCStrings.h"
#include "minecraft/VersionFilterData.h"

JavaVersionList::JavaVersionList(QObject *parent) : BaseVersionList(parent)
{
}

Task *JavaVersionList::getLoadTask()
{
	return new JavaListLoadTask(this);
}

const BaseVersionPtr JavaVersionList::at(int i) const
{
	return m_vlist.at(i);
}

bool JavaVersionList::isLoaded()
{
	return m_loaded;
}

int JavaVersionList::count() const
{
	return m_vlist.count();
}

QVariant JavaVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	auto version = std::dynamic_pointer_cast<JavaVersion>(m_vlist[index.row()]);
	switch (role)
	{
		case VersionPointerRole:
			return qVariantFromValue(m_vlist[index.row()]);
		case VersionIdRole:
			return version->descriptor();
		case VersionRole:
			return version->id;
		case RecommendedRole:
			return version->recommended;
		case PathRole:
			return version->path;
		case ArchitectureRole:
			return version->arch;
		default:
			return QVariant();
	}
}

BaseVersionList::RoleList JavaVersionList::providesRoles()
{
	return {VersionPointerRole, VersionIdRole, VersionRole, RecommendedRole, PathRole, ArchitectureRole};
}


void JavaVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	// manual testing fakery
	/*
	m_vlist.push_back(std::make_shared<JavaVersion>("1.6.0_33", "64", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.6.0_44", "64", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.6.0_55", "64", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.7.0_44", "64", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.8.0_44", "64", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.6.0_33", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.6.0_44", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.6.0_55", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.7.0_44", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.8.0_44", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.9.0_1231", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.9.0_1", "32", "/foo/bar/baz"));
	m_vlist.push_back(std::make_shared<JavaVersion>("1.9.0_1", "64", "/foo/bar/baz"));
	*/
	sortVersions();
	if(m_vlist.size())
	{
		auto best = std::dynamic_pointer_cast<JavaVersion>(m_vlist[0]);
		best->recommended = true;
	}
	endResetModel();
}

bool sortJavas(BaseVersionPtr left, BaseVersionPtr right)
{
	auto rleft = std::dynamic_pointer_cast<JavaVersion>(left);
	auto rright = std::dynamic_pointer_cast<JavaVersion>(right);
	// prefer higher arch
	auto archCompare = Strings::naturalCompare(rleft->arch, rright->arch, Qt::CaseInsensitive);
	if(archCompare != 0)
		return archCompare > 0;
	// dirty hack - 1.9 and above is too new
	auto labove19 = Strings::naturalCompare(rleft->name(), g_VersionFilterData.discouragedJavaVersion, Qt::CaseInsensitive) >= 0;
	auto rabove19 = Strings::naturalCompare(rright->name(), g_VersionFilterData.discouragedJavaVersion, Qt::CaseInsensitive) >= 0;
	if(labove19 == rabove19)
	{
		// prefer higher versions in general
		auto nameCompare = Strings::naturalCompare(rleft->name(), rright->name(), Qt::CaseInsensitive);
		if(nameCompare != 0)
			return nameCompare > 0;
		// if all else is equal, sort by path
		return Strings::naturalCompare(rleft->path, rright->path, Qt::CaseInsensitive) < 0;
	}
	return labove19 < rabove19;
}

void JavaVersionList::sortVersions()
{
	beginResetModel();
	std::sort(m_vlist.begin(), m_vlist.end(), sortJavas);
	endResetModel();
}

JavaListLoadTask::JavaListLoadTask(JavaVersionList *vlist) : Task()
{
	m_list = vlist;
	m_currentRecommended = NULL;
}

JavaListLoadTask::~JavaListLoadTask()
{
}

void JavaListLoadTask::executeTask()
{
	setStatus(tr("Detecting Java installations..."));

	JavaUtils ju;
	QList<QString> candidate_paths = ju.FindJavaPaths();

	m_job = std::shared_ptr<JavaCheckerJob>(new JavaCheckerJob("Java detection"));
	connect(m_job.get(), SIGNAL(finished(QList<JavaCheckResult>)), this, SLOT(javaCheckerFinished(QList<JavaCheckResult>)));
	connect(m_job.get(), &Task::progress, this, &Task::setProgress);

	qDebug() << "Probing the following Java paths: ";
	int id = 0;
	for(QString candidate : candidate_paths)
	{
		qDebug() << " " << candidate;

		auto candidate_checker = new JavaChecker();
		candidate_checker->m_path = candidate;
		candidate_checker->m_id = id;
		m_job->addJavaCheckerAction(JavaCheckerPtr(candidate_checker));

		id++;
	}

	m_job->start();
}

void JavaListLoadTask::javaCheckerFinished(QList<JavaCheckResult> results)
{
	QList<JavaVersionPtr> candidates;

	qDebug() << "Found the following valid Java installations:";
	for(JavaCheckResult result : results)
	{
		if(result.valid)
		{
			JavaVersionPtr javaVersion(new JavaVersion());

			javaVersion->id = result.javaVersion;
			javaVersion->arch = result.mojangPlatform;
			javaVersion->path = result.path;
			candidates.append(javaVersion);

			qDebug() << " " << javaVersion->id << javaVersion->arch << javaVersion->path;
		}
	}

	QList<BaseVersionPtr> javas_bvp;
	for (auto java : candidates)
	{
		//qDebug() << java->id << java->arch << " at " << java->path;
		BaseVersionPtr bp_java = std::dynamic_pointer_cast<BaseVersion>(java);

		if (bp_java)
		{
			javas_bvp.append(java);
		}
	}

	m_list->updateListData(javas_bvp);
	emitSucceeded();
}
