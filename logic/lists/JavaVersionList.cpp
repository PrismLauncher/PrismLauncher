/* Copyright 2013 MultiMC Contributors
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

#include "JavaVersionList.h"
#include "MultiMC.h"

#include <QtNetwork>
#include <QtXml>
#include <QRegExp>

#include "logger/QsLog.h"
#include <logic/JavaUtils.h>

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

int JavaVersionList::columnCount(const QModelIndex &parent) const
{
	return 4;
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
	case Qt::DisplayRole:
		switch (index.column())
		{
		case 0:
			return version->id;

		case 1:
			return version->arch;

		case 2:
			return version->path;

		case 3:
			return version->recommended ? tr("Yes") : tr("No");

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		return version->descriptor();

	case VersionPointerRole:
		return qVariantFromValue(m_vlist[index.row()]);

	default:
		return QVariant();
	}
}

QVariant JavaVersionList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case 0:
			return "Version";

		case 1:
			return "Arch";

		case 2:
			return "Path";

		case 3:
			return "Recommended";

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case 0:
			return "The name of the version.";

		case 1:
			return "The architecture this version is for.";

		case 2:
			return "Path to this Java version.";

		case 3:
			return "Whether the version is recommended or not.";

		default:
			return QVariant();
		}

	default:
		return QVariant();
	}
}

BaseVersionPtr JavaVersionList::getTopRecommended() const
{
	for (int i = 0; i < m_vlist.length(); i++)
	{
		auto ver = std::dynamic_pointer_cast<JavaVersion>(m_vlist.at(i));
		if (ver->recommended)
		{
			return m_vlist.at(i);
		}
	}
	return BaseVersionPtr();
}

void JavaVersionList::updateListData(QList<BaseVersionPtr> versions)
{
	beginResetModel();
	m_vlist = versions;
	m_loaded = true;
	endResetModel();
	// NOW SORT!!
	// sort();
}

void JavaVersionList::sort()
{
	// NO-OP for now
}

JavaListLoadTask::JavaListLoadTask(JavaVersionList *vlist)
{
	m_list = vlist;
	m_currentRecommended = NULL;
}

JavaListLoadTask::~JavaListLoadTask()
{
}

void JavaListLoadTask::executeTask()
{
	setStatus("Detecting Java installations...");

	JavaUtils ju;
	QList<JavaVersionPtr> javas = ju.FindJavaPaths();

	QList<BaseVersionPtr> javas_bvp;
	for (int i = 0; i < javas.length(); i++)
	{
		BaseVersionPtr java = std::dynamic_pointer_cast<BaseVersion>(javas.at(i));

		if (java)
		{
			javas_bvp.append(java);
		}
	}

	m_list->updateListData(javas_bvp);

	emitSucceeded();
}
