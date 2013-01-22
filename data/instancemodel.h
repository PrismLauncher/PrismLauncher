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

#ifndef INSTANCELIST_H
#define INSTANCELIST_H

#include <QList>
#include <QMap>
#include <QSet>
#include <qabstractitemmodel.h>

enum IMI_type
{
	IMI_Root,
	IMI_Group,
	IMI_Instance
};

class InstanceModel;
class InstanceGroup;
class InstanceBase;

class InstanceModelItem
{
	public:
	virtual IMI_type getModelItemType() const = 0;
	virtual InstanceModelItem * getParent() const = 0;
	virtual int numChildren() const = 0;
	virtual InstanceModelItem * getChild(int index) const = 0;
	virtual InstanceModel * getModel() const = 0;
	virtual QVariant data(int role) const = 0;
	virtual int getRow() const = 0;
};

class InstanceGroup : public InstanceModelItem
{
public:
	InstanceGroup(const QString& name, InstanceModel * model);
	~InstanceGroup();
	
	QString getName() const;
	void setName(const QString& name);

	bool isHidden() const;
	void setHidden(bool hidden);

	virtual IMI_type getModelItemType() const
	{
		return IMI_Group;
	}
	virtual InstanceModelItem* getParent() const;
	virtual InstanceModelItem* getChild ( int index ) const;
	virtual int numChildren() const;
	virtual InstanceModel * getModel() const
	{
		return model;
	};
	virtual QVariant data ( int column ) const;
	int getIndexOf(InstanceBase * inst)
	{
		return instances.indexOf(inst);
	};
	virtual int getRow() const;
	void addInstance ( InstanceBase* inst );
protected:
	QString name;
	InstanceModel * model;
	QVector<InstanceBase*> instances;
	bool hidden;
	int row;
};

class InstanceModel : public QAbstractItemModel, public InstanceModelItem
{
public:
	explicit InstanceModel(QObject *parent = 0);
	~InstanceModel();
	
	virtual int columnCount ( const QModelIndex& parent = QModelIndex() ) const;
	virtual QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
	virtual QModelIndex index ( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
	virtual QModelIndex parent ( const QModelIndex& child ) const;
	virtual int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
	
	void addInstance(InstanceBase *inst, const QString& groupName = "Ungrouped");
	void setInstanceGroup(InstanceBase *inst, const QString & groupName);
	InstanceGroup* getGroupByName(const QString & name) const;
	
	void initialLoad(QString dir);
	bool saveGroupInfo() const;
	
	virtual IMI_type getModelItemType() const
	{
		return IMI_Root;
	}
	virtual InstanceModelItem * getParent() const 
	{
		return nullptr;
	};
	virtual int numChildren() const;
	virtual InstanceModelItem* getChild ( int index ) const;
	virtual InstanceModel* getModel() const
	{
		return nullptr;
	};
	virtual QVariant data ( int column ) const;
	int getIndexOf(const InstanceGroup * grp) const
	{
		return groups.indexOf((InstanceGroup *) grp);
	};
	virtual int getRow() const
	{
		return 0;
	};
signals:
	
public slots:
	
private:
	QString groupFile;
	QVector<InstanceGroup*> groups;
	InstanceGroup * implicitGroup;
};

#endif // INSTANCELIST_H
