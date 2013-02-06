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

#include "instancemodel.h"

#include <QString>
#include <QDir>
#include <QDirIterator>
#include "stdinstance.h"

#include "../util/pathutils.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>


#define GROUP_FILE_FORMAT_VERSION 1

InstanceModel::InstanceModel( QObject* parent ) :
	QAbstractItemModel()
{
}

InstanceModel::~InstanceModel()
{
	saveGroupInfo();
	for(int i = 0; i < groups.size(); i++)
	{
		delete groups[i];
	}
}

void InstanceModel::addInstance( InstanceBase* inst, const QString& groupName )
{
	auto group = getGroupByName(groupName);
	group->addInstance(inst);
}

void InstanceGroup::addInstance ( InstanceBase* inst )
{
	instances.append(inst);
	inst->setGroup(this);
	// TODO: notify model.
}


void InstanceModel::initialLoad(QString dir)
{
	groupFile = dir + "/instgroups.json";
	implicitGroup = new InstanceGroup("Ungrouped", this);
	groups.append(implicitGroup);
	
	// temporary map from instance ID to group name
	QMap<QString, QString> groupMap;
	
		using namespace boost::property_tree;
	ptree pt;

	try
	{
		read_json(groupFile.toStdString(), pt);

		if (pt.get_optional<int>("formatVersion") != GROUP_FILE_FORMAT_VERSION)
		{
			// TODO: Discard old formats.
		}

		BOOST_FOREACH(const ptree::value_type& vp, pt.get_child("groups"))
		{
			ptree gPt = vp.second;
			QString groupName = QString::fromUtf8(vp.first.c_str());

			InstanceGroup *group = new InstanceGroup(groupName, this);
			groups.push_back(group);

			if (gPt.get_child_optional("hidden"))
				group->setHidden(gPt.get<bool>("hidden"));

			QVector<QString> groupInstances;
			BOOST_FOREACH(const ptree::value_type& v, gPt.get_child("instances"))
			{
				QString key = QString::fromUtf8(v.second.data().c_str());
				groupMap[key] = groupName;
			}
		}
	}
	catch (json_parser_error e)
	{
		qDebug("Failed to read group list. JSON parser error.");
//		wxLogError(_(), 
//			e.line(), wxStr(e.message()).c_str());
	}
	catch (ptree_error e)
	{
		qDebug("Failed to read group list. Unknown ptree error.");
	}
	
	qDebug("Loading instances");
	QDir instDir(dir);
	QDirIterator iter(instDir);
	
	while (iter.hasNext())
	{
		QString subDir = iter.next();
		if (QFileInfo(PathCombine(subDir, "instance.cfg")).exists())
		{
			// TODO Differentiate between different instance types.
			InstanceBase* inst = new StdInstance(subDir);
			QString instID = inst->getInstID();
			auto iter = groupMap.find(instID);
			if(iter != groupMap.end())
			{
				addInstance(inst,iter.value());
			}
			else
			{
				addInstance(inst);
			}
		}
	}
}

int InstanceModel::columnCount ( const QModelIndex& parent ) const
{
	// for now...
	return 1;
}

QVariant InstanceModel::data ( const QModelIndex& index, int role ) const
{
	if (!index.isValid())
		return QVariant();

	InstanceModelItem *item = static_cast<InstanceModelItem*>(index.internalPointer());

	return item->data(role);
}

QModelIndex InstanceModel::index ( int row, int column, const QModelIndex& parent ) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();
	
	InstanceModelItem *parentItem;

	if (!parent.isValid())
		parentItem = (InstanceModelItem *) this;
	else
		parentItem = static_cast<InstanceModelItem*>(parent.internalPointer());

	InstanceModelItem *childItem = parentItem->getChild(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
	
}

QModelIndex InstanceModel::parent ( const QModelIndex& index ) const
{
	if (!index.isValid())
		return QModelIndex();

	InstanceModelItem *childItem = static_cast<InstanceModelItem*>(index.internalPointer());
	InstanceModelItem *parentItem = childItem->getParent();

	if (parentItem == this)
		return QModelIndex();

	return createIndex(parentItem->getRow(), 0, parentItem);
}

int InstanceModel::rowCount ( const QModelIndex& parent ) const
{
	InstanceModelItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = (InstanceModelItem*) this;
	else
		parentItem = static_cast<InstanceModelItem*>(parent.internalPointer());

	return parentItem->numChildren();
}

bool InstanceModel::saveGroupInfo() const
{
	/*
	using namespace boost::property_tree;
	ptree pt;

	pt.put<int>("formatVersion", GROUP_FILE_FORMAT_VERSION);

	try
	{
		typedef QMap<InstanceGroup *, QVector<InstanceBase*> > GroupListMap;

		GroupListMap groupLists;
		for (auto iter = instances.begin(); iter != instances.end(); iter++)
		{
			InstanceGroup *group = getInstanceGroup(*iter);
			
			if (group != nullptr)
				groupLists[group].push_back(*iter);
		}

		ptree groupsPtree;
		for (auto iter = groupLists.begin(); iter != groupLists.end(); iter++)
		{
			auto group = iter.key();
			auto & gList = iter.value();

			ptree groupTree;

			groupTree.put<bool>("hidden", group->isHidden());

			ptree instList;
			for (auto iter2 = gList.begin(); iter2 != gList.end(); iter2++)
			{
				std::string instID((*iter2)->getInstID().toUtf8());
				instList.push_back(std::make_pair("", ptree(instID)));
			}
			groupTree.put_child("instances", instList);

			groupsPtree.push_back(std::make_pair(std::string(group->getName().toUtf8()), groupTree));
		}
		pt.put_child("groups", groupsPtree);

		write_json(groupFile.toStdString(), pt);
	}
	catch (json_parser_error e)
	{
// 		wxLogError(_("Failed to read group list.\nJSON parser error at line %i: %s"), 
// 			e.line(), wxStr(e.message()).c_str());
		return false;
	}
	catch (ptree_error e)
	{
// 		wxLogError(_("Failed to save group list. Unknown ptree error."));
		return false;
	}

	return true;
	*/
	return false;
}

void InstanceModel::setInstanceGroup ( InstanceBase* inst, const QString& groupName )
{
	/*
	InstanceGroup *prevGroup = getInstanceGroup(inst);

	if (prevGroup != nullptr)
	{
		groupsMap.remove(inst);
	}

	if (!groupName.isEmpty())
	{
		InstanceGroup *newGroup = nullptr;

		for (auto iter = root->groups.begin(); iter != root->groups.end(); iter++)
		{
			if ((*iter)->getName() == groupName)
			{
				newGroup = *iter;
			}
		}

		if (newGroup == nullptr)
		{
			newGroup = new InstanceGroup(groupName, this);
			root->groups.push_back(newGroup);
		}

		groupsMap[inst] = newGroup;
	}

	// TODO: propagate change, reflect in model, etc.
	//InstanceGroupChanged(inst);
	*/
}

InstanceGroup* InstanceModel::getGroupByName ( const QString& name ) const
{
	for (auto iter = groups.begin(); iter != groups.end(); iter++)
	{
		if ((*iter)->getName() == name)
			return *iter;
	}
	return nullptr;
}
/*
void InstanceModel::setGroupFile ( QString filename )
{
	groupFile = filename;
}*/

int InstanceModel::numChildren() const
{
	return groups.count();
}

InstanceModelItem* InstanceModel::getChild ( int index ) const
{
	return groups[index];
}

QVariant InstanceModel::data ( int role ) const
{
	switch(role)
	{
		case Qt::DisplayRole:
			return "name";
	}
	return QVariant();
}


InstanceGroup::InstanceGroup(const QString& name, InstanceModel *parent)
{
	this->name = name;
	this->model = parent;
	this->hidden = false;
}

InstanceGroup::~InstanceGroup()
{
	for(int i = 0; i < instances.size(); i++)
	{
		delete instances[i];
	}
}


QString InstanceGroup::getName() const
{
	return name;
}

void InstanceGroup::setName(const QString& name)
{
	this->name = name;
	//TODO: propagate change
}

InstanceModelItem* InstanceGroup::getParent() const
{
	return model;
}

bool InstanceGroup::isHidden() const
{
	return hidden;
}

void InstanceGroup::setHidden(bool hidden)
{
	this->hidden = hidden;
	//TODO: propagate change
}

int InstanceGroup::getRow() const
{
	return model->getIndexOf( this);
}

InstanceModelItem* InstanceGroup::getChild ( int index ) const
{
	return instances[index];
}

int InstanceGroup::numChildren() const
{
	return instances.size();
}

QVariant InstanceGroup::data ( int role ) const
{
	switch(role)
	{
		case Qt::DisplayRole:
			return name;
		default:
			return QVariant();
	}
}